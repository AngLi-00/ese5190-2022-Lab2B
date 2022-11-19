#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ws2812.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

// Some logic to analyse:
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/structs/pwm.h"
#include "hardware/structs/pwm.h"

const uint CAPTURE_PIN_BASE = 21;
const uint CAPTURE_PIN_COUNT = 1;
const uint CAPTURE_N_SAMPLES = 1;
const uint BOOT_PIN = 21;

#define WS2812_PIN 12
#define WS2812_POWER_PIN 11
#define FREQ 800000
#define IS_RGBW true


#define record 'r'
#define replay 'p'
#define BOOT_PIN 21

uint32_t boot_pin_pressed;

static inline uint bits_packed_per_word(uint pin_count) {
    // If the number of pins to be sampled divides the shift register size, we
    // can use the full SR and FIFO width, and push when the input shift count
    // exactly reaches 32. If not, we have to push earlier, so we use the FIFO
    // a little less efficiently.
    const uint SHIFT_REG_WIDTH = 32;
    return SHIFT_REG_WIDTH - (SHIFT_REG_WIDTH % pin_count);
}

void logic_analyser_init(PIO pio, uint sm, uint pin_base, uint pin_count, float div) {
    // Load a program to capture n pins. This is just a single `in pins, n`
    // instruction with a wrap.
    uint16_t capture_prog_instr = pio_encode_in(pio_pins, pin_count);
    struct pio_program capture_prog = {
            .instructions = &capture_prog_instr,
            .length = 1,
            .origin = -1
    };
    uint offset = pio_add_program(pio, &capture_prog);

    // Configure state machine to loop over this `in` instruction forever,
    // with autopush enabled.
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_in_pins(&c, pin_base);
    sm_config_set_wrap(&c, offset, offset);
    sm_config_set_clkdiv(&c, div);
    // Note that we may push at a < 32 bit threshold if pin_count does not
    // divide 32. We are using shift-to-right, so the sample data ends up
    // left-justified in the FIFO in this case, with some zeroes at the LSBs.
    sm_config_set_in_shift(&c, true, true, bits_packed_per_word(pin_count));
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    pio_sm_init(pio, sm, offset, &c);
}

void logic_analyser_arm(PIO pio, uint sm, uint dma_chan, uint32_t *capture_buf, size_t capture_size_words,
                        uint trigger_pin, bool trigger_level) {
    pio_sm_set_enabled(pio, sm, false);
    // Need to clear _input shift counter_, as well as FIFO, because there may be
    // partial ISR contents left over from a previous run. sm_restart does this.
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);

    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

    dma_channel_configure(dma_chan, &c,
        capture_buf,        // Destination pointer
        &pio->rxf[sm],      // Source pointer
        capture_size_words, // Number of transfers
        true                // Start immediately
    );

    pio_sm_exec(pio, sm, pio_encode_wait_gpio(trigger_level, trigger_pin));
    pio_sm_set_enabled(pio, sm, true);
}

void print_capture_buf(const uint32_t *buf, uint pin_base, uint pin_count, uint32_t n_samples) {
    printf("Capture:\n");
    // Each FIFO record may be only partially filled with bits, depending on
    // whether pin_count is a factor of 32.
    uint record_size_bits = bits_packed_per_word(pin_count);
    for (int pin = 0; pin < pin_count; ++pin) {
        printf("%02d: ", pin + pin_base);
        for (int sample = 0; sample < n_samples; ++sample) {
            uint bit_index = pin + sample * pin_count;
            uint word_index = bit_index / record_size_bits;
            if (buf[word_index] != 0){
                boot_pin_pressed = 0;
            }
            else{
                boot_pin_pressed = 1;
            }

        }
    }
}

void turn_on_power(){
    const uint led_pin = WS2812_POWER_PIN;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin,1);
}

 void set_neopixel_color(uint32_t rgb) {
      //convert RGB to GRB
     uint32_t grb = ((rgb & 0xFF0000) >> 8) | ((rgb & 0x00FF00) << 8) | (rgb & 0x0000FF);
     pio_sm_put_blocking(pio0, 0, grb << 8u);
 }


int main() {

    stdio_init_all();
    PIO pio_0 = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio_0, &ws2812_program);
    turn_on_power();
    ws2812_program_init(pio_0, sm, offset, WS2812_PIN, FREQ, IS_RGBW);

    // We're going to capture into a u32 buffer, for best DMA efficiency. Need
    // to be careful of rounding in case the number of pins being sampled
    // isn't a power of 2.
    uint total_sample_bits = CAPTURE_N_SAMPLES * CAPTURE_PIN_COUNT;
    total_sample_bits += bits_packed_per_word(CAPTURE_PIN_COUNT) - 1;
    uint buf_size_words = total_sample_bits / bits_packed_per_word(CAPTURE_PIN_COUNT);
    uint32_t *capture_buf = malloc(buf_size_words * sizeof(uint32_t));
    hard_assert(capture_buf);
    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_W_BITS | BUSCTRL_BUS_PRIORITY_DMA_R_BITS;

    PIO pio_1 = pio1;
    uint dma_chan = 0;

    logic_analyser_init(pio_1, sm, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, 256.f);

    gpio_init(BOOT_PIN);
    gpio_set_dir(BOOT_PIN, GPIO_IN);

    char input;
    scanf("%c",&input);
    printf("Start to press button.\n");

    uint32_t key = 0x00000000;
    uint32_t flag = 0x00000000;

    while(true){
        key = getchar_timeout_us(0);
        switch(key){
            case 'r':
                set_neopixel_color(0X00FF0000);
                sleep_ms(1000);
                flag = 0x00000000;

                while(true){
                    
                    flag = getchar_timeout_us(0);
                    logic_analyser_arm(pio_1, sm, dma_chan, capture_buf, buf_size_words, CAPTURE_PIN_BASE, false);
                    print_capture_buf(capture_buf, CAPTURE_PIN_BASE, CAPTURE_PIN_COUNT, CAPTURE_N_SAMPLES);
                    
                    if(boot_pin_pressed){
                        printf("1\n");
                        set_neopixel_color(0X0000FF00);
                    } 
                    else {
                        printf("0\n");
                        set_neopixel_color(0x00000000);
                    }
                    if(flag == 'J'){
                        set_neopixel_color(0X00000000);
                        sleep_ms(10);
                        break;
                    }
                    sleep_ms(10); 
                }
                           
                break;
                
            case 'p':
                flag = 0x00000000;
                while(true){
                    
                    flag = getchar_timeout_us(0);
                    if(flag == '1'){
                        set_neopixel_color(0X000000FF);
                    }
                    if(flag == '0'){
                        set_neopixel_color(0x00000000);
                    }
                    if(flag == 'J'){
                        set_neopixel_color(0x00000000);
                        sleep_ms(10);
                        break;
                    }
                    sleep_ms(10);
                }
                break;
        }
    }    
}
