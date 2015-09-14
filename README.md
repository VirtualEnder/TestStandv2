# Smart Turnigy Thrust Stand 
Thrust, voltage, amperage, and RPM sensing designed to replace the internal electronics in the Turnigy Thrust stand. Code is for the Stellaris Launchpad, but should also work fine on the Tiva C launchpad. It may also work for Arduino with a bit of reworking on the timer code, though I can't guarantee what the cycle time will be.

## Sample Rates
Currently the RPM sample rate depends on your serial output rate.  At 115200 baud, sample rate is ~2ms, at 230400 ~1ms, and at 460800 ~600us.  Voltage (analog in), Amperage (analog in) and Thrust (load cell amp) are measured via a timer at a configurable rate.  The load cell amp refreshes at 80hz, but the timer can be set faster to get higher resolution analog reads.  Setting the timer to a very fast refresh rate may impact analog read accuracy.

##Caveats
In order to save cycle time, I am currently doing as little processing on the MCU as possible, so I am simply outputing raw pulses for reading RPMs and raw analog values for voltage and current.  I am performing tare/calibration calculations on the thrust readings, but I don't really see a valid way of doing that off MCU without a whole ton of extra work and potential inaccuracy.
