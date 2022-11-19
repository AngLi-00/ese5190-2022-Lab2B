#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "pio_i2c.h"

#include "ws2812.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "registers.h"

#include <stdlib.h>
#include "hardware/structs/bus_ctrl.h"

const int addr = 0x39;
PIO pio = pio0;
uint sm = 0;

static void APDS9960_reset() {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {0x80, 0x27};
    pio_i2c_write_blocking(pio, sm, addr, buf, 2, false);
}

static void mpu6050_read_raw(int8_t *temp) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[1];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x9C;
    pio_i2c_write_blocking(pio, sm, addr, &val, 1, true); // true to keep master control of bus
    pio_i2c_read_blocking(pio, sm, addr, buffer, 1, false);

    *temp = buffer[0];
}

static void APDS_read_color(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c) {

    uint8_t buffer[2];
    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t c_addr = 0x94;
    pio_i2c_write_blocking(pio, sm, addr, &c_addr, 1, true); // true to keep master control of bus
    pio_i2c_read_blocking(pio, sm, addr, buffer, 2, false);

    *c = buffer[1];
    *c = *c << 8;
    *c |= buffer[0];

    uint8_t r_addr = 0x96;
    pio_i2c_write_blocking(pio, sm, addr, &r_addr, 1, true); // true to keep master control of bus
    pio_i2c_read_blocking(pio, sm, addr, buffer, 2, false);

    *r = buffer[0] | (buffer[1] << 8);

    uint8_t g_addr = 0x98;
    pio_i2c_write_blocking(pio, sm, addr, &g_addr, 1, true); // true to keep master control of bus
    pio_i2c_read_blocking(pio, sm, addr, buffer, 2, false);

    *g = buffer[0] | (buffer[1] << 8);

    uint8_t b_addr = 0x9A;
    pio_i2c_write_blocking(pio, sm, addr, &b_addr, 1, true); // true to keep master control of bus
    pio_i2c_read_blocking(pio, sm, addr, buffer, 2, false);

    *b = buffer[0] | (buffer[1] << 8);
}

int main() {
    const uint SDA_PIN = 22;
    const uint SCL_PIN = 23;

    stdio_init_all();
    // i2c_init(i2c1, 200*1000);
    // rp_init();
    // turn_on_pixel();

    // set_pixel_color(0x00A2B4E6);
    // sleep_ms(1000);
    // set_pixel_color(0x00000000);

    uint offset = pio_add_program(pio, &i2c_program);
    i2c_program_init(pio, sm, offset, SDA_PIN, SCL_PIN);
    // pio_i2c_repstart(pio, sm);
    // gpio_pull_up(SDA_PIN);
    // gpio_pull_up(SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(SDA_PIN, SCL_PIN, GPIO_FUNC_I2C));
    APDS9960_reset();

    uint8_t proximity = 0;
    uint16_t r,g,b,c;

    while (true) {     
        // set_pixel_color(0x00000000);
        proximity = 0;
        r = 0; g = 1; b = 0; c = 0;
        mpu6050_read_raw(&proximity);
        APDS_read_color(&r, &g, &b, &c);

        printf("The proximity is: %d\n", proximity);
        printf("The r g b c are: %d %d %d %d\n", r, g, b, c);

        sleep_ms(500);
    }
}