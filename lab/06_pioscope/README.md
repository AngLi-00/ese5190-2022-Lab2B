### TODO:

- modify the PIO/DMA logic analyzer example to record a timestamped trace of an RP2040-ADPS9960 exchange while the BOOT button is pressed
- with a partner, connect a second Qt Py to the Stemma Qt bus of an ADPS9960 that's attached to a first Qt Py running the Lab 1 Python code
- record a trace of this exchange


### Demo
We connect 2 Qt Py and ADPS9960 via two Stemma Qt bus lines like below.  
![](https://github.com/AngLi-00/ese5190-2022-Lab2B/blob/460e113eb4becd210847ac2662f9b6345a327e98/figure/part6connnect.jpg)  
When one Qt Py running logic_analyser and the other Qt Py running the Lab 1 Python code, we can see the recorded trace given as follows:  
![](https://github.com/AngLi-00/ese5190-2022-Lab2B/blob/460e113eb4becd210847ac2662f9b6345a327e98/figure/part6.png)
