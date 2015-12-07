/*

Pin connections for this software:

PWM Output:                  Pin 14 or PB_6
Current Sensor:              Pin 29 or PE_2 
Voltage Sensor:              Pin 28 or PE_3
Load Cell Amp HX711.DOUT     Pin 9 or PA_6
              HX711.PD_SCK   Pin 8 or PA_5 
Electical/Magnetic RPMs      Pin 31 or PF_4 or PUSH1
Optical RPMs                 Pin 17 or PF_0 or PUSH2

Hardware PWM output adapted from: http://codeandlife.com/2012/10/30/stellaris-launchpad-pwm-tutorial/
Stellaris timer code adapted from:  http://patolin.com/blog/2014/06/29/stellaris-launchpad-energia-pt-2-timers/
*/

#include "Energia.h" 
#include "inc/hw_memmap.h" 
#include "inc/hw_types.h" 
#include "inc/hw_ints.h" 
#include "inc/hw_timer.h"
#include "driverlib/Debug.h" 
#include "driverlib/interrupt.h" 
#include "driverlib/sysctl.h" 
#include "driverlib/pin_map.h"
#include "driverlib/adc.h"
#include "driverlib/rom.h"
#include "driverlib/timer.h" 
#include "HX711.h"           // Requires HX711 Library from: https://github.com/bogde/HX711
#include "Average.h"           // Requires Average Library from: https://github.com/MajenkoLibraries/Average
#include <EEPROM.h>

// Configuration options
#define UARTBAUD 921600    // UART Baud rate (DO NOT set to less than 115200) 
#define SENSORRATE 500     // Refresh rate in HZ of load cell and analog read timer.
#define MAGSENS true       // Using Magnetic RPM sensor?
#define OPTISENS false     // Using Magnetic RPM sensor?
#define POLES 14           // Number of magnetic poles in test motor.
#define MINTHROTTLE 1000   // Low end of ESC calibrated range
#define MAXTHROTTLE 2000   // High end of ESC calibrated range
#define MINCOMMAND 980     // Value sent to ESC when test isn't running.
#define OVERSAMPLING 64    // Analog oversampling multiplier
#define VSCALE 26          // Scale factor for Voltage divider.
#define CSCALE 100         // Scale factor for current sensor.
#define LSCALE -395        // Scale factor for load cell amplifier.
#define BRAKEMAXRPM 30000  // Maximum RPM limit used in braking test.
#define BRAKEMINRPM 3800   // Maximum RPM limit used in braking test.
#define BRAKERPMSAMPLE 150 // Sample size of RPM averaging for target RPM detection during brake test.

// Scale Pins
// HX711.DOUT	- pin #9
// HX711.PD_SCK	- pin #8
HX711 scale(9, 8);	

// RPM input variables
volatile uint32_t stepCount1 = 0;
volatile uint32_t stepCount2 = 0;
volatile uint64_t stepTime1 = 0;
volatile uint64_t stepTime2 = 0;
volatile uint32_t RPMs1 = 0;
volatile uint32_t RPMs2 = 0;

// analog value variablesyour
uint32_t ulADC0Value[8];
volatile uint32_t voltageValue = 0;
volatile uint32_t currentValue = 0;
volatile uint32_t thrust = 0;

// Misc Variables
uint32_t ulPeriod = (((MAXTHROTTLE - 1000)*10) + 10000) + 799;
uint8_t character;
String input;
uint64_t startTime;
uint64_t loopStart;
boolean isTestRunning = false;
uint32_t currentMicros = 0;
boolean isTared = false;

union f_bytes
{
  byte b[4];
  float fval;
}u;

void setup() {
  
  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);  // Set system clock to 80mhz
  
  // initialize serial communication:
  Serial.begin(UARTBAUD);
    
  // attach Interupt for RPM sensor
  if(MAGSENS) {
    pinMode(33, OUTPUT);
    digitalWrite(33,LOW);
    pinMode(32, OUTPUT);
    digitalWrite(32,HIGH);
    pinMode(PUSH1, INPUT_PULLUP);
    attachInterrupt(PUSH1, countRpms, FALLING);
  }
  if(OPTISENS) {
    pinMode(PUSH2, INPUT_PULLUP);
    attachInterrupt(PUSH2, countRpms2, FALLING);
  }
  
  scale.set_scale(LSCALE);  // Eventually set this via EEPROM
  scale.tare();	// Reset the scale to 0
  
  adcTimer(SENSORRATE);     // Start timer for load cell and analog reads
  initPWMOut();               // Start PWM output
}

void countRpms () {
  if(isTestRunning) {
    uint64_t stepMicros1 = micros();
    uint64_t lastStep1 = stepTime1;
    stepTime1 = stepMicros1;
    stepCount1++;    // Increase Step counter
    // Calculate RPMs from step time.
    RPMs1 = ((((float)1/(float)(stepMicros1 - lastStep1))*1000000)/(POLES/2))*60;
  }
}

void countRpms2 () {
  if(isTestRunning) {
    uint64_t stepMicros2 = micros();
    uint64_t lastStep2 = stepTime2;
    stepTime2 = stepMicros2;
    stepCount2++;    // Increase Step counter
    // Calculate RPMs from step time.
    RPMs2 = ((((float)1/(float)(stepMicros2 - lastStep2))*1000000)/(POLES/2))*60;
  }
}

void loop() {
  
  isTestRunning = false;  // Stop reads from load cell and reset step counters
  stepCount1 = 0;
  stepCount2 = 0;
  updatePWM(MINCOMMAND);  // Double check throttle is at 0
  
  if(!isTared) {
    scale.tare();
    isTared = true;
  }
  
  // Prompt for input and read it
  Serial.println("Type t(Tare), c(Calibrate), s(Start), or i(Idle)");
  input="";
  while(!Serial.available());
  while(Serial.available()) {
      character = Serial.read();
      input.concat(character);
      delay(2);
  }
  Serial.print("Input: ");
  Serial.println(input);
  
  //Check input
  
  if(input.indexOf("t") >= 0) {
    input="";
    Serial.println("Taring");
    scale.tare();
  }
  if(input.indexOf("c") >= 0) {
    input="";
    
    // Start ESC Calibration Routine
    Serial.println("Please make SURE the ESC is unplugged from power, then hit any key. Otherwise press 'e' to exit");
    while(!Serial.available());
    while(Serial.available()) {
        character = Serial.read();
        input.concat(character);
        delay(1);
    }
    if(input.indexOf("e") < 0) {
      updatePWM(MAXTHROTTLE - 15);  // Calibrate to Max throttle -15 usecs to ensure full throttle is reached.
      Serial.println("Plug in the ESC to battery power and wait for the calibration beeps, then press any key to continue.");
      while(!Serial.available());
      updatePWM(MINTHROTTLE);      // Set bottom of range
      Serial.println("Once calibration has finished unplug the battery and hit any key to continue");
      while(!Serial.available());
      Serial.println("ESC Calibration Complete, thank you!");
      delay(3000);
    }
  }
  if(input.indexOf("i") >= 0) {
    Serial.println("Idling, press any key to exit");
    delay(2000);
    
    // Idle for 4 seconds
    startTime=micros();
    updatePWM(1100);
    while(!Serial.available()&& micros()-startTime < 4000000) {
    }
    while(Serial.available()) {
        character = Serial.read();
        delay(1);
    }
    isTared = false;
  }
  
  if(input.indexOf("b") >= 0) {
    
    // Print CSV header output
    Serial.println("Begining automated braking test, press any key to exit");
    delay(2000);
    
    startTime=micros();
    isTestRunning = true;
    uint16_t escMicros = MINCOMMAND;
    uint16_t minRPMThrottle = 0;
    uint16_t maxRPMThrottle = 0;
    Average<uint16_t> avgRPMs(BRAKERPMSAMPLE);
    
    Serial.println("Calibrating Braking RPMs");
    while(!Serial.available() && isTestRunning) {
      
      loopStart = micros(); 
      uint32_t currentLoopTime = loopStart-startTime;
      
      if(currentLoopTime<6000000) 
        // Iterate through whole throttle range based on time
        escMicros = (((float)(currentLoopTime)/6000000.0)* 1000)+1000;
      else if(currentLoopTime<=8000000) 
        escMicros = MINCOMMAND;
      else {
        isTestRunning = false;
        isTared = false;
      }
      if(escMicros != currentMicros) {
        updatePWM(escMicros);
        currentMicros = escMicros;
      }
      for (uint8_t i = 0; i < BRAKERPMSAMPLE; i++) {
        delayMicroseconds(980);
        if(MAGSENS) {
          avgRPMs.push(RPMs1);
        } 
        if(OPTISENS) {
          avgRPMs.push(RPMs2);
        }
      }
      Serial.println("Cycle time: " + (loopStart - micros()) );
      if(avgRPMs.mean() > BRAKEMINRPM && minRPMThrottle == 0) {
        minRPMThrottle = currentMicros;
      }
      if(avgRPMs.mean() > BRAKEMAXRPM && maxRPMThrottle == 0) {
        maxRPMThrottle = currentMicros;
      }
    }
    
    scale.tare();
    Serial.println("Beginning Brake test:");\
    delay(2000);
    
    // Print CSV header output
    Serial.print("Thrust(g),");
    if(MAGSENS) {
      Serial.print("mSteps,");
    }
    if(OPTISENS) {
      Serial.print("oSteps,");
    }
    Serial.print("Throttle(uS),");
    Serial.print("Time(uS),");
    if(MAGSENS) {
      Serial.print("mPRMs,");
    }
    if(OPTISENS) {
      Serial.print("oRPMs,");
    }
    
    // Initiate test run
    startTime=micros();
    isTestRunning = true;
    escMicros = MINCOMMAND;
    
    while(!Serial.available() && isTestRunning ) {
      loopStart = micros(); 
      uint32_t currentLoopTime = loopStart-startTime;
      if(currentLoopTime<2000000)
        escMicros = minRPMThrottle;
      else if(currentLoopTime<4000000)
        escMicros = maxRPMThrottle;
      else if(currentLoopTime<6000000)
        escMicros = minRPMThrottle;
      else if(currentLoopTime<8000000)
        escMicros = MINCOMMAND;
      else {
        isTestRunning = false;
        isTared = false;
      }
      if(escMicros != currentMicros) {
        updatePWM(escMicros);
        currentMicros = escMicros;
      }
      
      // Print out data
      
      Serial.print(thrust);
      Serial.print(",");
      if(MAGSENS) {
        Serial.print(stepCount1);
        Serial.print(",");
      }
      if(OPTISENS) {
        Serial.print(stepCount2);
        Serial.print(","); 
      }
      Serial.print(escMicros);
      Serial.print(",");
      Serial.print(currentLoopTime);
      Serial.print(",");
      if(MAGSENS) {
        Serial.print(RPMs1);
        Serial.print(",");
      }
      if(OPTISENS) {
        Serial.print(RPMs2);
        Serial.print(",");
      }
    }
    
    minRPMThrottle = 0;
    maxRPMThrottle = 0;
    RPMs1 = 0;
    RPMs2 = 0;
    
  }
  
  if(input.indexOf("s") >= 0) {
    input="";
    
    // Print CSV header output
    Serial.println("Begining automated test, press any key to exit");
    delay(2000);
    Serial.print("Thrust(g),");
    /*
    Serial.print("Voltage,");
    Serial.print("Current,");
    */
    if(MAGSENS) {
      Serial.print("mSteps,");
    }
    if(OPTISENS) {
      Serial.print("oSteps,");
    }
    Serial.print("Throttle(uS),");
    Serial.print("Time(uS),");
    if(MAGSENS) {
      Serial.print("mPRMs,");
    }
    if(OPTISENS) {
      Serial.print("oRPMs,");
    }
    Serial.print("Volts,");
    Serial.print("Amps");
    
    // Initiate test run
    startTime=micros();
    isTestRunning = true;
    isTared = false;
    uint16_t escMicros = MINCOMMAND;
    
    while(!Serial.available() && isTestRunning) {
      loopStart = micros(); 
      uint32_t currentLoopTime = loopStart-startTime;
      if(currentLoopTime<2000000)
        escMicros = 1250;
      else if(currentLoopTime<4000000)
        escMicros = 1100;
      else if(currentLoopTime<6000000)
        escMicros = 1500;
      else if(currentLoopTime<8000000)
        escMicros = 1100;
      else if(currentLoopTime<10000000)
        escMicros = MAXTHROTTLE;
      else if(currentLoopTime<11000000)
        escMicros = MINCOMMAND;
      else if(currentLoopTime<12000000 && !isTared) {
        // Tare scale between passes to increase accuracy.
        scale.tare();
        isTared = true;
      } 
      else if(currentLoopTime<18000000)
        // Iterate through whole throttle range based on time
        escMicros = (((float)(currentLoopTime-12000000)/6000000.0)* 1000)+1000;   
      else if(currentLoopTime<20000000)
        escMicros = MAXTHROTTLE;
      else if(currentLoopTime<=22000000)
        escMicros = 1100;
      else if(currentLoopTime<=24000000)
        escMicros = MINCOMMAND;
      else {
        isTestRunning = false;
        isTared = false;
      }
      if(escMicros != currentMicros) {
        updatePWM(escMicros);
        currentMicros = escMicros;
      }
      
      // If no steps have happened in 500ms reset rpms to 0
      // This means that the minimum RPMs the code is capable of detecting is
      // 120 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
      // at about 2000 rpms.
      if(loopStart-stepTime1 > 500000) {
        RPMs1 = 0;
      }
      if(loopStart-stepTime2 > 500000) {
        RPMs2 = 0;
      }
      
      // Print out data
      
      Serial.print(thrust);
      Serial.print(",");
      /*
      Serial.print(voltageValue);
      Serial.print(",");
      Serial.print(currentValue);
      Serial.print(",");
      */
      if(MAGSENS) {
        Serial.print(stepCount1);
        Serial.print(",");
      }
      if(OPTISENS) {
        Serial.print(stepCount2);
        Serial.print(","); 
      }
      Serial.print(escMicros);
      Serial.print(",");
      Serial.print(currentLoopTime);
      Serial.print(",");
      if(MAGSENS) {
        Serial.print(RPMs1);
        Serial.print(",");
      }
      if(OPTISENS) {
        Serial.print(RPMs2);
        Serial.print(",");
      }
      Serial.print(((float)voltageValue/4096) * (float)VSCALE); // Calculate Volts from analog sensor
      Serial.print(",");
      Serial.println(((float)currentValue/4096) * (float)CSCALE); // Calculate Amps from analog sensor
   
      // Delay here adjusts the sample rate for the RPM sensors, as they are updated asynchronously via the interrupts.
      // Note that cycle times are limited by serial baud rates as well. You can change delay here to just higher than
      // the serial delay to get more stable cycle times.
      // 115200 = ~2ms cycle
      // 230400 = ~1.5ms cycle
      // All faster bauds = ~1ms cycle
      // minimum looptime is set to 1ms for all higher baud rates.

      uint32_t thisDelay;
      uint32_t loopTime = micros() - loopStart;
      switch(UARTBAUD) {
        case 115200:
          thisDelay = (1897 - loopTime);
          break;
          
        case 230400:
          thisDelay = (1497 - loopTime);
          break;
          
        default:
          thisDelay = (997 - loopTime);
          break;
      }  
      if (thisDelay < 0) { thisDelay = 0; }
      delayMicroseconds(thisDelay);
    }
    while(Serial.available()) {
        character = Serial.read();
        delay(1);
    }
  } 
  delay(1);
}

void adcTimer (unsigned Hz) { 
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  ADCHardwareOversampleConfigure(ADC0_BASE, OVERSAMPLING);
  ADCSequenceDisable(ADC0_BASE, 0);
  
  ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 3);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH1);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH1);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH1);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH1 | ADC_CTL_IE | ADC_CTL_END);
  ADCSequenceEnable(ADC0_BASE, 0);
  ADCIntClear(ADC0_BASE, 0);
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);  
  TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC); 
  uint64_t ulPeriod = (SysCtlClockGet () / Hz);
  TimerLoadSet(TIMER1_BASE, TIMER_A, ulPeriod -1); 
  IntEnable(INT_TIMER1A); 
  TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT); 
  TimerIntRegister(TIMER1_BASE, TIMER_A, Timer1IntHandler); 
  TimerEnable(TIMER1_BASE, TIMER_A); 
}

void Timer1IntHandler() {
  TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
  if(isTestRunning) {
    if(ADCIntStatus(ADC0_BASE, 0, false)){
      ADCIntClear(ADC0_BASE, 0);
      ADCSequenceDataGet(ADC0_BASE, 0, ulADC0Value);
      voltageValue = (ulADC0Value[0] + ulADC0Value[1] + ulADC0Value[2] + ulADC0Value[3] + 2)/4;
      currentValue = (ulADC0Value[4] + ulADC0Value[5] + ulADC0Value[6] + ulADC0Value[7] + 2)/4;
    } else {
      ADCIntClear(ADC0_BASE, 0);
      ADCProcessorTrigger(ADC0_BASE, 0);
    }
    if(scale.is_ready()){
      thrust = scale.get_units();
    }
  }

}

void initPWMOut () { 
    
    // Configure PB6 as T0CCP0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinConfigure(GPIO_PB6_T0CCP0);
    GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_6);

    // Configure timer
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_PWM); 
    TimerControlLevel(TIMER0_BASE, TIMER_A, 1);                            // Set timer to PWM mode
    TimerLoadSet(TIMER0_BASE, TIMER_A, 20800 - 1);                         // Set PWM period 260us
    updatePWM(MINCOMMAND);                                                 // Set PWM Timer match to MINTHROTTLE
    TimerEnable(TIMER0_BASE, TIMER_A);
    
}

void updatePWM(unsigned pulseWidth) {
        //Prevent escMicros from overflowing the timer
        if (pulseWidth > 2075) {
          //pulseWidth = 2075;
        }
        // Convert 1000-2000us range to 125-250us range and apply to PWM output
        uint32_t dutyCycle = (pulseWidth *10) - 1;
        /*Serial.print("Pulse Width: ");
        Serial.print(pulseWidth);
        Serial.print(" Duty Cycle: ");
        Serial.println(dutyCycle);*/
        TimerMatchSet(TIMER0_BASE, TIMER_A, dutyCycle ); 
        
}
