# Smart Turnigy Thrust Stand 
Thrust, voltage, amperage, and RPM sensing designed to replace the internal electronics in the Turnigy Thrust stand. 

## Requirements
This code is designed to work in the Energia IDE for the TI Launchpad series. Code is for the Stellaris Launchpad, but should also work fine on the Tiva C launchpad. It may also work for Arduino with a bit of reworking on the timer code, though I can't guarantee what the cycle time will be. Energia is available here: http://energia.nu/download/

This code is built around the HX711 breakout board from sparkfun: https://www.sparkfun.com/products/13230
The HX711 Library from ogde is also required and can be downloaded here: https://github.com/bogde/HX711
Even though this is an Arduino library, it doesn't use any low level code and is compatible out of the box with Energia and the Stellaris.

## Sample Rates
Currently the RPM sample rate depends on your serial output rate.  At 115200 baud, sample rate is ~2.6ms, at 230400 ~2.1ms, and at 460800 ~1.2us.  Voltage (analog in), Amperage (analog in) and Thrust (load cell amp) are measured via a timer at a configurable rate.  The load cell amp refreshes at 80hz, but reads are done asynchrnously to the loop.  Analog reads are done via the Stellaris internal interrupts and are asynchrnous to the loop and have hardware oversampling configured via a define.  The timer that checks if the analog values are ready only needs to be run slightly faster than the fastest read delay.  The internal analog reads update around 250hz with 64x oversampling.


### Credits
Thanks to zdayton at Davis RotorCross for the initial schematic and code: http://www.davisrotorcross.org/2015/09/09/adding-data-logging-to-the-turnigy-thrust-test-stand/
