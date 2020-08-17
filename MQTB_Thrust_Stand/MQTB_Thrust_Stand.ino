/*

Pin connections for this software:

PWM Output:                  Pin 2 or PE_4 - I/O Board A4
PWM Output:                  Pin 7 or PB_4 - I/O Board A5
PWM Output:                  Pin 5 or PC_4 - I/O Board B2
PWM Output:                  Pin 6 or PC_5 - I/O Board B3
DShot Output:                Pin   or PA_5 - I/O Board B8
Voltage Sensor:              Pin 27 or PE_1 - I/O Board AY
Current Sensor:              Pin 28 or PE_2 - I/O Board AX
Load Cell Amp HX711.DOUT     Pin 38 or PB_3 - I/O Board A6
              HX711.PD_SCK   Pin 19 or PB_2 - I/O BOard A7
M1 RPMs                      Pin 32 or PD_7 - I/O BOard B4
M2 RPMs                      Pin 33 or PD_6 - I/O BOard B5
M3 RPMs                      Pin 34 or PC_7 - I/O BOard B6
M4 RPMs                      Pin 35 or PC_6 - I/O BOard B7

Hardware PWM output adapted from: http://codeandlife.com/2012/10/30/stellaris-launchpad-pwm-tutorial/
Stellaris timer code adapted from:  http://patolin.com/blog/2014/06/29/stellaris-launchpad-energia-pt-2-timers/
*/
#include "config.h"
#include "Energia.h"  
#include "inc/hw_memmap.h" 
#include "inc/hw_types.h" 
#include "inc/hw_ints.h" 
#include "inc/hw_timer.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ssi.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/Debug.h" 
#include "driverlib/interrupt.h" 
#include "driverlib/sysctl.h" 
#include "driverlib/systick.h"
#include "driverlib/adc.h"
#include "driverlib/rom.h"
#include "driverlib/timer.h"
#include "driverlib/pwm.h" 
#include "driverlib/udma.h"
#include "driverlib/ssi.h"
#include "HX711.h"             // Requires HX711 Library from: https://github.com/bogde/HX711
#include "Average.h"           // Requires Average Library from: https://github.com/MajenkoLibraries/Average
#include <EEPROM.h>
#include <PriUint64.h>
#include <HardwareSerial.h>
#include "variables.h"
#include "ramp.h"

void setup() {

  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);  // Set system clock to 80mhz

  // initialize serial communication:
  Serial.begin(UARTBAUD);
  //tlmSerial.begin(115200); //, SERIAL_8N1, 16, 17);

  
  // attach Interupt for RPM sensor

    int rpmPins[] = {31,33,34,35};
    void (*rpmFunctions[4])() {rpmTrigger1,rpmTrigger2,rpmTrigger3,rpmTrigger4};
    for (int i = 0; i < USE_MOTORS; i++) {
      pinMode(rpmPins[i], INPUT_PULLUP);
      attachInterrupt(digitalPinToInterrupt(rpmPins[i]), rpmFunctions[i], FALLING);
    }

  switch (ESCOUTPUT) {
     case 0:
       pwmMultiplier = 4000;  // Standard PWM
       break;
     case 1:
       pwmMultiplier = 1000;  // Oneshot 125
       break;
       
     case 2: 
       pwmMultiplier = 336;  // Oneshot 42
       break;
     
     case 3: 
       pwmMultiplier = 160;   // Multishot
       break;
  }
  
  if(USE_LOAD_CELL) {
    scale.set_scale(LSCALE);  // Eventually set this via EEPROM
    scale.tare();	            // Reset the scale to 0
  }
  
  adcTimer(SENSORRATE);     // Start timer for load cell and analog reads
  initPWMOut();             // Start PWM output
  initRPMCount();           // Start RPM counter timers

}

void loop() {
  
  
  isTestRunning = false;  // Stop reads from load cell and reset step counters
  stepCount[1] = 0;
  stepCount[2] = 0;
  stepCount[3] = 0;
  stepCount[4] = 0;
  stepDiff[1] = 0;
  stepDiff[2] = 0;
  stepDiff[3] = 0;
  stepDiff[4] = 0;
  updatePWM(MINCOMMAND);  // Double check throttle is at 0

  //if(!isTared) {
  //  scale.tare();
  //  isTared = true;
  //}

  // Prompt for input and read it
  Serial.println("Type: t(Tare), v(Battery Voltage), w(Load Cell), c(Calibrate),");
  Serial.println("      k(KV Test), m(Main Test), i(Idle) or z(Custom)");
  input="";
  while(!Serial.available());
  while(Serial.available()) {
      character = Serial.read();
      input.concat(character);
      delay(2);
  }

  Serial.print("Input: ");
  Serial.println(input);

  //Check input and run appropriate routine

  if(input.indexOf("t") >= 0) {
    tareScale();
  } else
  if(input.indexOf("c") >= 0) {
    calibrate();
  } else
  if(input.indexOf("i") >= 0) {
    idle();
  } else
  if(input.indexOf("m") >= 0) {
    mainTest();
  } else
  if(input.indexOf("v") >= 0) {
    returnVoltage();
  } else
  if(input.indexOf("w") >= 0) {
    returnScale();
  } else
  if(input.indexOf("k") >= 0) {
    kvTest();
  } else
  if(input.indexOf("z") >= 0) {
    customTest();
  }
  delay(100);
}
