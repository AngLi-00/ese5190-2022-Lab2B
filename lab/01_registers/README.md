### TODO:

Toggle the Qt Py's Neopixel LED when the BOOT button is pressed, using only direct register reads to access the boot button status. You may use the SDK to initialize the board and the pins, and the WS2812 example code to toggle the Neopixel. 

### Demo  
![](https://github.com/AngLi-00/ese5190-2022-Lab2B/blob/e24796e04403414e74434f74fcc83c9a6c6f8d6f/figure/QT_PY_RP2040_PIN.png)

  
From the figure above we know that the GPIO of BOOT button is GPIO 21. We need to use `gpio_get()` to know the status of BOOT button.  

  
![alt](https://github.com/AngLi-00/ese5190-2022-Lab2B/blob/9864fe594c2c0b90d8d6168eab47c72a19def2f9/figure/part1.gif)
