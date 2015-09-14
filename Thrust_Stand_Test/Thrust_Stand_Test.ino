
#include "Energia.h" 
#include "inc/hw_memmap.h" 
#include "inc/hw_types.h" 
#include "inc/hw_ints.h" 
#include "driverlib/Debug.h" 
#include "driverlib/interrupt.h" 
#include "driverlib/sysctl.h" 
#include "driverlib/timer.h" //See more at: http://patolin.com/blog/2014/06/29/stellaris-launchpad-energia-pt-2-timers/#sthash.VheM8bk6.dpuf
#include <EEPROM.h>
#include <Servo.h> 

//IO pins
int datPin = 9;
int clkPin=8;
int voltagePin=A0;
int currentPin=A1;
int potentiometerPin=A2;
int potHigh=7;
int potLow=6;
int ESCPin=12;
int probePowerHigh=5;
int probeVOfsPin=A3;
int currentMicros = 0;

// RPM pins
volatile int rpmCount = 0;
volatile int rpmCount2 = 0;

// analog value variables
int voltageValue = 0;
int currentValue = 0;
int thrust = 0;

Servo ESC;
char character;
String input;
String calibrationWeight;
float tare=0;
float calibration;
int calibrationmass;
float throttle;
unsigned long startTime;

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
  //if the read flaot is nan, clear the eeprom
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
// the setup routine runs once when you press reset:
void setup() {
  
  // initialize serial communication:
  Serial.begin(460800);
  
  // make the pushbutton's pin an input:
  pinMode(datPin, INPUT);
  pinMode(clkPin,OUTPUT);
  pinMode(potHigh,OUTPUT);
  pinMode(potLow,OUTPUT);
  pinMode(probePowerHigh,OUTPUT);
  
  //attach ESC servo output
  ESC.attach(ESCPin,1050,1990);
  
  //attach Interupt for RPM sensor
  pinMode(PUSH1, INPUT_PULLUP);
  attachInterrupt(PUSH1, countRpms, FALLING);
  pinMode(PUSH2, INPUT_PULLUP);
  attachInterrupt(PUSH2, countRpms2, FALLING);
  
  //high and low legs of the pot, because there aren't enough 5v and gnd pins on the uno
  digitalWrite(potHigh,1);
  digitalWrite(potLow,0);
  digitalWrite(probePowerHigh,1);
  
  tare = readFloatFromEEPROM(0);
  calibration=readFloatFromEEPROM(4);  
  
  initTimer(120);
}

float readPot()
{
  return (float)analogRead(potentiometerPin)/1024.0f;
}
void setThrottle()
{
  ESC.write((int)(throttle*180.0f));
}

float readLoadCell()
{
  unsigned long load=0;
  float loadOut;
  while(digitalRead(datPin));
  for(char i=0;i<24;i++)
  {
    digitalWrite(clkPin,1);
    load=load<<1;
    digitalWrite(clkPin,0);
    if(digitalRead(datPin))
      load++;
  }
  digitalWrite(clkPin,1);
  load=load^0x800000;
  loadOut = (float)load/128.0f;
  digitalWrite(clkPin,0);
  return loadOut;
}


void countRpms () {
  rpmCount++;
}

void countRpms2 () {
  rpmCount2++;
}
// the loop routine runs over and over again forever:
void loop() {
  ESC.write(0);
  Serial.println("Type Tare, Calibrate, Start, or Free");
  input="";
  while(!Serial.available());
  while(Serial.available())
  {
      character = Serial.read();
      input.concat(character);
      delay(1);
  }
  
  //Serial.print("Input: ");
  //Serial.println(input);
  
  if(input.indexOf("Tare") >= 0)
  {
    input="";
    Serial.println("Taring");
    tare=readLoadCell();
    saveFloatToEEPROM(tare,0);
  }
  if(input.indexOf("Calibrate") >= 0)
  {
    input="";
    Serial.println("You must Tare before calibrating.  To exit calibration without saving a new value, type Exit. Otherwise enter the calibration mass in grams, without units");
    while(!Serial.available());
    while(Serial.available())
    {
        character = Serial.read();
        input.concat(character);
        delay(1);
    }
    if(input.indexOf("Exit") < 0)
    {
      calibrationmass=input.toInt();
      Serial.print("Calibration mass: ");
      Serial.println(calibrationmass);
      calibration=(float)calibrationmass/(readLoadCell()-tare);
      saveFloatToEEPROM(calibration,4);
      Serial.print("New measured mass: ");
      Serial.println((readLoadCell()-tare)*calibration);
    }
  }
  if(input.indexOf("Free") >= 0)
  {
    input="";
    Serial.println("Begining free run, press any key to exit");
    delay(2000);
    Serial.println("Thrust(g),Voltage,Current,oRotations,eRotations,Throttle(%),Time(ms)");
    startTime=millis();
    while(!Serial.available())
    {
      throttle=readPot();
      setThrottle();
      Serial.print(thrust);
      Serial.print(",");
      Serial.print(voltageValue);
      Serial.print(",");
      Serial.print(currentValue);
      Serial.print(",");
      Serial.print(rpmCount);
      Serial.print(",");
      Serial.print(rpmCount2);
      Serial.print(","); 
      Serial.print(throttle);
      /*Serial.print(",");
      int diffMicros = micros() - currentMicros;
      currentMicros = micros();
      Serial.print(diffMicros);*/
      Serial.print(",");
      Serial.println(millis()-startTime);
    }
    while(Serial.available())
    {
        character = Serial.read();
        delay(1);
    }
  }
  if(input.indexOf("Start") >= 0)
  {
    input="";
    Serial.println("Begining automated test, press any key to exit");
    delay(2000);
    Serial.println("Thrust(g),Voltage,Current,oRotations,eRotations,Throttle(%),Cycle(ms),Time(ms)");
    startTime=millis();
    while(!Serial.available() && (millis()-startTime)<18000)
    {  
      if((millis()-startTime)<2000)
        throttle=0.25;
      else if((millis()-startTime)<4000)
        throttle=0.1;
      else if((millis()-startTime)<6000)
        throttle=0.50;
      else if((millis()-startTime)<8000)
        throttle=0.1;
      else if((millis()-startTime)<10000)
        throttle=1.0;
      else if((millis()-startTime)<12000)
        throttle=0.0;
      else if((millis()-startTime)<18000)
        throttle=(float)(millis()-startTime-12000)/6000.0;
      else
        throttle=0.0;
      setThrottle();
      Serial.print(thrust);
      Serial.print(",");
      Serial.print(voltageValue);
      Serial.print(",");
      Serial.print(currentValue);
      Serial.print(",");
      Serial.print(rpmCount);
      Serial.print(",");
      Serial.print(rpmCount2);
      Serial.print(","); 
      Serial.print(throttle);
      /*Serial.print(",");
      int diffMicros = micros() - currentMicros;
      currentMicros = micros();
      Serial.print(diffMicros);*/
      Serial.print(",");
      Serial.println(millis()-startTime);
    }
    while(Serial.available())
    {
        character = Serial.read();
        delayMicroseconds(200);
    }
  }
  //delayMicroseconds(200);        // delay in between reads for stability
}

void initTimer (unsigned Hz) { 
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0); 
  //TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER); 
  TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC); 
  unsigned long ulPeriod = (SysCtlClockGet () / Hz) / 2; 
  TimerLoadSet(TIMER0_BASE, TIMER_A, ulPeriod -1); 
  IntEnable(INT_TIMER0A); 
  TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT); 
  TimerIntRegister(TIMER0_BASE, TIMER_A, Timer0IntHandler); 
  TimerEnable(TIMER0_BASE, TIMER_A); 
}

void Timer0IntHandler() {
  TimerIntClear (TIMER0_BASE, TIMER_TIMA_TIMEOUT);
  voltageValue = analogRead(voltagePin);
  currentValue = analogRead(currentPin);
  thrust = (readLoadCell()-tare)*calibration;
}

