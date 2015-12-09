# Smart Turnigy Thrust Stand 
Thrust, voltage, amperage, and RPM sensing designed to replace the internal electronics in the Turnigy Thrust stand.   Check config.h for configuration options and documentation.

## Requirements
This code is designed to work in the Energia IDE for the TI Launchpad series. Code is for the Stellaris Launchpad, but should also work fine on the Tiva C launchpad. It may also work for Arduino with a bit of reworking on the timer code, though I can't guarantee what the cycle time will be. Energia is available here: http://energia.nu/download/

This code is built around the HX711 breakout board from sparkfun: https://www.sparkfun.com/products/13230
The HX711 Library from ogde is also required and can be downloaded here: https://github.com/bogde/HX711
Even though this is an Arduino library, it doesn't use any low level code and is compatible out of the box with Energia and the Stellaris.

In addition the code uses the Average library during the braking test. However, in order for the averaging to be accurate, you must change the \_sum variable type definition in the library to uint64\_t to avoid overflows.  The average library can be found here: http://playground.arduino.cc/Main/Average

## Sample Rates
Currently the output update rate depends on your serial output rate.  At 115200 baud, output update rate is ~2.5ms, at 230400 ~2ms, and at 460800 and greater ~1us. Default looptimes can be over-ridden via config.h to decrease data density. RPMs are triggered on an interrupt and are updated asynchronously from the loop and output rate. Voltage (analog in), Amperage (analog in) and Thrust (load cell amp) are measured via a timer at a configurable rate.  The load cell amp refreshes at 80hz, but reads are done asynchrnously to the loop.  Analog reads are done via the Stellaris internal interrupts, are asynchrnous to the loop, and have hardware oversampling configured via a define.  The timer only needs to be run slightly faster than the fastest read delay.  The internal analog reads update around 250hz with 64x oversampling.  For all the data, the output rate stays constant and new information is simply updated as it comes in, allowing for flexible resolution, and no need to wait on the slower reading inputs during the output stage. With a suitibly fast output loop, this should mean that the data in the output is always the freshest available.


### Credits
Thanks to zdayton at Davis RotorCross for the initial schematic and code: http://www.davisrotorcross.org/2015/09/09/adding-data-logging-to-the-turnigy-thrust-test-stand/
