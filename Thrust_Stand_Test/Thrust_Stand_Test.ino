#include "Energia.h" 
#include "inc/hw_memmap.h" 
#include "inc/hw_types.h" 
#include "inc/hw_ints.h" 
#include "driverlib/Debug.h" 
#include "driverlib/interrupt.h" 
#include "driverlib/sysctl.h" 
#include "driverlib/adc.h"
#include "driverlib/rom.h"
#include "driverlib/timer.h" //See more at: http://patolin.com/blog/2014/06/29/stellaris-launchpad-energia-pt-2-timers/#sthash.VheM8bk6.dpuf
#include "HX711.h"           //Requires HX711 Library from: https://github.com/bogde/HX711
#include <EEPROM.h>
#include <Servo.h> 

// Configuration options
#define UARTBAUD 921600   // UART Baud rate (DO NOT set to less than 115200) 
#define SENSORRATE 500    // Refresh rate in HZ of load cell and analog read timer.
#define MAGSENS true      // Using Magnetic RPM sensor?
#define OPTISENS false    // Using Magnetic RPM sensor?
#define POLES 14          // Number of magnetic poles in test motor.
#define MINTHROTTLE 1000  // Low end of ESC calibrated range
#define MAXTHROTTLE 2000  // High end of ESC calibrated range
#define MINCOMMAND 980    // Value sent to ESC when test isn't running.
#define OVERSAMPLING 64   // Analog oversampling multiplier
#define VSCALE 26         // Scale factor for Voltage divider.
#define CSCALE 100        // Scale factor for current sensor.
#define LSCALE -700       // Scale factor for load cell amplifier.

//IO pins
int ESCPin=36;            // ESC PWM output pin

// Scale Pins
// HX711.DOUT	- pin #9
// HX711.PD_SCK	- pin #8
HX711 scale(9, 8);	

// RPM input variables
volatile unsigned int stepCount1 = 0;
volatile unsigned int stepCount2 = 0;
volatile unsigned int stepTime1 = 0;
volatile unsigned int stepTime2 = 0;
volatile unsigned int RPMs1 = 0;
volatile unsigned int RPMs2 = 0;

// analog value variables
unsigned long ulADC0Value[8];
volatile int voltageValue = 0;
volatile int currentValue = 0;
volatile int thrust = 0;

// Misc Variables
Servo ESC;
char character;
String input;
String calibrationWeight;
float calibration;
int calibrationmass;
unsigned long startTime;
unsigned long loopStart;
boolean isTestRunning = false;
unsigned int currentMicros = 0;
boolean isTared = false;

union f_bytes
{
  byte b[4];
  float fval;
}u;

float readFloatFromEEPROM(int address)
{
  for(char i=0;i<4;i++)
  {
    u.b[i]=EEPROM.read(address+i);
  }
  // if the read float is nan, clear the eeprom
  if(u.fval!=u.fval)
  {
    EEPROM.write(address,0);
    EEPROM.write(address+1,0);
    EEPROM.write(address+2,0);
    EEPROM.write(address+3,0);
  }
  return u.fval;
}
void saveFloatToEEPROM(float toSave,int address)
{
  u.fval=toSave;
  for(char i=0;i<4;i++)
  {
    EEPROM.write(address+i,u.b[i]);
  }
  
}
:
void setup() {
  
  // initialize serial communication:
  Serial.begin(UARTBAUD);
  
  // attach ESC servo output
  ESC.attach(ESCPin,MINTHROTTLE,MAXTHROTTLE);
  ESC.writeMicroseconds(MINCOMMAND);  // Ensure throttle is at 0
  
  // attach Interupt for RPM sensor
  pinMode(33, OUTPUT);
  digitalWrite(33,LOW);
  pinMode(32, OUTPUT);
  digitalWrite(32,HIGH);
  pinMode(PUSH1, INPUT_PULLUP);
  attachInterrupt(PUSH1, countRpms, FALLING);
  pinMode(PUSH2, INPUT_PULLUP);
  attachInterrupt(PUSH2, countRpms2, FALLING);
  
  
  //calibration=readFloatFromEEPROM(4);  
  
  scale.set_scale(LSCALE);  // Eventually set this via EEPROM
  scale.tare();	// Reset the scale to 0
  
  initTimer0(SENSORRATE);     // Start timer for load cell and analog reads
}

void countRpms () {
  if(isTestRunning) {
    stepCount1++;    // Increase Step counter
    // Calculate RPMs from step time.
    RPMs1 = ((((float)1/(float)(micros() - stepTime1))*1000000)/(POLES/2))*60;
    stepTime1 = micros();
  }
}

void countRpms2 () {
  if(isTestRunning) {
    stepCount2++;    // Increase Step counter
    // Calculate RPMs from step time.
    RPMs2 = ((((float)1/(float)(micros() - stepTime2))*1000000)/(POLES/2))*60;
    stepTime2 = micros();
  }
}

void loop() {
  
  isTestRunning = false;  // Stop reads from load cell and reset step counters
  stepCount1 = 0;
  stepCount2 = 0;
  ESC.writeMicroseconds(MINCOMMAND);  // Double check throttle is at 0
  
  if(!isTared) {
    scale.tare();
    isTared = false;
  }
  
  // Prompt for input and read it
  Serial.println("Type t(Tare), c(Calibrate), s(Start), or i(Idle)");
  input="";
  while(!Serial.available());
  while(Serial.available()) {
      character = Serial.read();
      input.concat(character);
      delay(1);
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
      ESC.writeMicroseconds(MAXTHROTTLE - 15);  // Calibrate to Max throttle -15 usecs to ensure full throttle is reached.
      Serial.println("Plug in the ESC to battery power and wait for the calibration beeps, then press any key to continue.");
      while(!Serial.available());
      ESC.writeMicroseconds(MINTHROTTLE);      // Set bottom of range
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
    ESC.writeMicroseconds(1100);
    while(!Serial.available()&& micros()-startTime < 4000000) {
    }
    while(Serial.available()) {
        character = Serial.read();
        delay(1);
    }
  }
  
  if(input.indexOf("s") >= 0) {
    input="";
    
    //Create CSV header output
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
    int escMicros = MINCOMMAND;
    
    while(!Serial.available() && isTestRunning) {
      loopStart = micros(); 
      int currentLoopTime = micros()-startTime;
      if(currentLoopTime<2000000)
        escMicros = 1250;
      else if(currentLoopTime<4000000)
        escMicros = 1100;
      else if(currentLoopTime<6000000)
        escMicros = 1500;
      else if(currentLoopTime<8000000)
        escMicros = 1100;
      else if(currentLoopTime<10000000)
        escMicros = 2000;
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
        escMicros = 2000;
      else if(currentLoopTime<=22000000)
        escMicros = 1100;
      else if(currentLoopTime<=24000000)
        escMicros = MINCOMMAND;
      else {
        isTestRunning = false;
        isTared = false;
      }
      if(escMicros != currentMicros) {
        ESC.writeMicroseconds(escMicros);
        currentMicros = escMicros;
      }
      
      // If no steps have happened in 500ms reset rpms to 0
      // This means that the minimum RPMs the code is capable of detecting is
      // 120 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
      // at about 2000 rpms.
      if(micros()-stepTime1 > 500000) {
        RPMs1 = 0;
      }
      if(micros()-stepTime1 > 500000) {
        RPMs1 = 0;
      }
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

      int thisDelay;
      unsigned int loopTime = micros() - loopStart;
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

void initTimer0 (unsigned Hz) { 
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  ADCHardwareOversampleConfigure(ADC0_BASE, OVERSAMPLING);
  ADCSequenceDisable(ADC0_BASE, 0);
  
  ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
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
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); 
  //TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER); 
  TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC); 
  unsigned long ulPeriod = (SysCtlClockGet () / Hz);
  TimerLoadSet(TIMER0_BASE, TIMER_A, ulPeriod -1); 
  IntEnable(INT_TIMER0A); 
  TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT); 
  TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0IntHandler); 
  TimerEnable(TIMER0_BASE, TIMER_A); 
}

void Timer0IntHandler() {
  TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
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

void initTimer1 (unsigned Hz) { 
 
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
  //TimerConfigure(TIMER1_BASE, TIMER_CFG_32_BIT_PER); 
  TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC); 
  unsigned long ulPeriod = (SysCtlClockGet () / Hz);
  TimerLoadSet(TIMER1_BASE, TIMER_A, ulPeriod -1); 
  IntEnable(INT_TIMER1A); 
  TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT); 
  TimerIntRegister(TIMER1_BASE, TIMER_A, Timer1IntHandler); 
  TimerEnable(TIMER1_BASE, TIMER_A); 
}

void Timer1IntHandler() {
   TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
}
