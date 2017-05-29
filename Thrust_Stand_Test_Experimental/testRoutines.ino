

/*
################################
 #     Scale Tare Routine       #
 ################################
 */

void tareScale() {
  delay(20);
  input="";
  Serial.println("Taring");
  scale.tare();
}


/*
################################
#         Scale Value          #
################################
*/

void returnScale() {
    isTestRunning = true;
    delay(20);
    input="";
    Serial.print("Current Load Value: ");
    Serial.println(thrust);
    isTestRunning = false;
}


/*
################################
#       Voltage Routine        #
################################
*/

void returnVoltage() {
    isTestRunning = true;
    delay(20);
    input="";
    Serial.print("Current Battery Voltage: ");
    Serial.println(((float)voltageValue/4096) * (float)VSCALE); // Calculate Volts from analog sensor
    isTestRunning = false;
}

/*
################################
 #     Calibration Routine      #
 ################################
 */

void calibrate() {
  input="";
  // Start ESC Calibration Routine
  delay(20);
  Serial.println("Please make SURE the ESC is unplugged from power, then hit any key. Otherwise press 'e' to exit");
  while(!Serial.available());
  while(Serial.available()) {
    character = Serial.read();
    input.concat(character);
    delay(1);
  }
  if(input.indexOf("e") < 0) {
    delay(20);
    updatePWM(MAXTHROTTLE - 15);  // Calibrate to Max throttle -15 usecs to ensure full throttle is reached.
    Serial.println("Plug in the ESC to battery power and wait for the calibration beeps, then press any key to continue.");
    while(!Serial.available());
    updatePWM(MINTHROTTLE);      // Set bottom of range
    Serial.println("Once calibration has finished unplug the battery and hit any key to continue");
    while(!Serial.available());
    Serial.println("ESC Calibration Complete, thank you!");
    delay(3000);
  }
  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
}


/*
################################
 #        Idle Routine          #
 ################################
 */

void idle() {
  delay(20);
  Serial.println("Idling, press any key to exit");
  delay(2000);

  // Idle for 4 seconds
  startTime=micros();
  updatePWM(IDLEPWM);
  while(!Serial.available()&& micros()-startTime < 4000000) {
  }
  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
  isTared = false;
}


/*
################################
 #     Brake Test Routine       #
 ################################
 */

void brakeTest() {

  delay(20);
  Serial.println("Begining automated braking test, press any key to exit");
  delay(2000);

  isTestRunning = true;
  uint16_t escMicros = MINCOMMAND;
  uint16_t minRPMThrottle = 0;
  uint16_t maxRPMThrottle = 0;
  Average<uint16_t> avgRPMs(BRAKERPMSAMPLE);

  Serial.println("Calibrating Braking RPMs");
  delay(1000);
  escMicros = MINTHROTTLE;
  updatePWM(escMicros);
  delay(2000);

  startTime=micros();
  while(!Serial.available() && isTestRunning) {

    loopStart = micros();
    uint32_t currentLoopTime = loopStart-startTime;
    if (currentLoopTime <= 0) {
      currentLoopTime = 1;
    }
    if(currentLoopTime<6000000)
      // Iterate through whole throttle range based on time
      escMicros = (((float)(currentLoopTime)/6000000.0)*(2000-MINTHROTTLE))+ MINTHROTTLE;
    else if(currentLoopTime<=7000000)
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
      delayMicroseconds(200);
      if(MAGSENS) {
        avgRPMs.push(stepDiff1);
      }
      if(OPTISENS) {
        avgRPMs.push(stepDiff2);
      }
    }
    float thisAvg = calculateRPMs(avgRPMs.mean(), false);
    avgRPMs.clear();
    uint16_t thisLoop = micros() - loopStart;
    /*Serial.print("Average : ");
     Serial.println(thisAvg);
     Serial.print("Throttle: ");
     Serial.println(escMicros);*/
    if(thisAvg > BRAKEMINRPM && minRPMThrottle == 0) {
      minRPMThrottle = currentMicros;
    }
    if(thisAvg > BRAKEMAXRPM && maxRPMThrottle == 0) {
      maxRPMThrottle = currentMicros;
    }
  }

  if (minRPMThrottle > 0 && maxRPMThrottle > 0) {
    scale.tare();
    Serial.println("Beginning Brake test:");
    Serial.print("Low Throttle: ");
    Serial.print(minRPMThrottle);
    Serial.print(" High Throttle: ");
    Serial.println(maxRPMThrottle);
    Serial.print("Low RPM Target: ");
    Serial.print(BRAKEMINRPM);
    Serial.print(" High RPM Target: ");
    Serial.println(BRAKEMAXRPM);

    delay(2000);

    // Print CSV header output
    Serial.print("Time(uS),");
    Serial.print("Throttle(uS),");
    Serial.print("Thrust(g),");
    if(MAGSENS) {
      Serial.print("eSteps,");
    }
    if(OPTISENS) {
      Serial.print("oSteps,");
    }
    if(MAGSENS) {
      Serial.println("eRPMs,");
    }
    if(OPTISENS) {
      Serial.println("oRPMs,");
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
        // End test and reset variables
        minRPMThrottle = 0;
        maxRPMThrottle = 0;
        isTared = false;
        isTestRunning = false;
      }
      if(escMicros != currentMicros) {
        updatePWM(escMicros);
        currentMicros = escMicros;
      }

      // Grab RPM calculations from last step times.
      if (MAGSENS) {
        theseRpms = calculateRPMs(stepDiff1);
      }
      if (OPTISENS) {
        theseRpms = calculateRPMs(stepDiff2);
      }

      // If no steps have happened in 100ms reset rpms to 0
      // This means that the minimum RPMs the code is capable of detecting is
      // 600 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
      // at about 2000 rpms.
      if(stepTime1 > 100000 || stepTime2 > 100000) {
        theseRpms = 0;
        avgStepDiff1.push(0);
      }

      // Print out data
      Serial.print(currentLoopTime);
      Serial.print(",");
      Serial.print(escMicros);
      Serial.print(",");
      Serial.print(thrust);
      Serial.print(",");
      if(MAGSENS) {
        Serial.print(stepCount1);
      }
      if(OPTISENS) {
        Serial.print(stepCount2);
      }
      Serial.print(",");
      if(MAGSENS) {
        theseRpms = calculateRPMs(stepDiff1);
      }
      if(OPTISENS) {
        theseRpms = calculateRPMs(stepDiff2);
      }
      Serial.println(theseRpms);

    }
  } 
  else {
    Serial.println("Throttle positions for target RPMs could not be acquired, aborting test!");
  }
  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
}

/*
################################
 #   ThrustRate test routine    #
 ################################
 */
void rateTest() {
  if (LOOPDELAY == 0) {
    Serial.println("Loop delay must be specified");
    return;
  }

  delay(20);
  Serial.println("Begining automated test, press any key to exit");
  delay(2000);

  char *headers[8] = {
    "thrust", "steps", "throttle",
    "time", "rpms", "volts", "amps", NULL     };
  //print headers
  for (int i = 0;headers[i] != NULL; i++) {
    Serial.print(headers[i]);
  }

  uint32_t start_time, curr_time, delta_time, loop_delay, loop_time;
  int prev_pwm, curr_pwm;
  ramp r;

  //setup code
  ramp_init(&r);
  start_time = micros();
  prev_pwm = 0;

  //add a six second ramp up/down
  ramp_add_static(&r, IDLEPWM, 2000); //idle for 2 seconds
  ramp_add_range(&r, IDLEPWM, MAXTHROTTLE, 6000); //ramp up over 6 seconds
  ramp_add_static(&r, MAXTHROTTLE, 500); //hold max for 500ms
  ramp_add_range(&r, MAXTHROTTLE, IDLEPWM, 6000); //ramp down over 6 seconds
  ramp_add_static(&r, IDLEPWM, 500); //hold min throttle for 500ms

  //one second ramp up/down
  ramp_add_range(&r, IDLEPWM, MAXTHROTTLE, 1000); //ramp up over 1 seconds
  ramp_add_static(&r, MAXTHROTTLE, 500); //hold max for 500ms
  ramp_add_range(&r, MAXTHROTTLE, IDLEPWM, 1000); //ramp down over 1 seconds
  ramp_add_static(&r, IDLEPWM, 500); //hold min throttle for 500ms

  //500ms ramp up/down
  ramp_add_range(&r, IDLEPWM, MAXTHROTTLE, 500); //ramp up over 500ms
  ramp_add_static(&r, MAXTHROTTLE, 500); //hold max for 500ms
  ramp_add_range(&r, MAXTHROTTLE, IDLEPWM, 500); //ramp down over 500ms
  ramp_add_static(&r, IDLEPWM, 500); //hold min throttle for 500ms

  //250ms ramp up/down
  ramp_add_range(&r, IDLEPWM, MAXTHROTTLE, 250); //ramp up over 250ms
  ramp_add_static(&r, MAXTHROTTLE, 500); //hold max for 500ms
  ramp_add_range(&r, MAXTHROTTLE, IDLEPWM, 250); //ramp down over 250ms
  ramp_add_static(&r, IDLEPWM, 500); //hold min throttle for 500ms

  //50ms ramp up/down
  ramp_add_range(&r, IDLEPWM, MAXTHROTTLE, 50); //ramp up over 50ms
  ramp_add_static(&r, MAXTHROTTLE, 500); //hold max for 500ms
  ramp_add_range(&r, MAXTHROTTLE, IDLEPWM, 50); //ramp down over 50ms
  ramp_add_static(&r, IDLEPWM, 2000); //hold min throttle for 500ms

  //main test loop
  isTestRunning = true;
  while(!Serial.available()) {
    curr_time = micros();
    delta_time = curr_time - start_time;

    //determine the current pwm
    curr_pwm = ramp_get_pwm(&r, delta_time);

    //-1 means the test is over
    if (curr_pwm == -1) {
      isTestRunning = false;
      break;
    }

    //only send pwm pulse if necessary
    if (curr_pwm != prev_pwm) {
      updatePWM(curr_pwm);
      prev_pwm = curr_pwm;
    }

    // Grab RPM calculations from average of last 20 step times
    if (MAGSENS) {
      theseRpms = averageRPMs();
    }
    else
      if (OPTISENS) {
        theseRpms = calculateRPMs(stepDiff2);
      }

    // Print out data
    Serial.print(thrust);
    Serial.print(",");
    Serial.print(",");
    Serial.print(curr_pwm);
    Serial.print(",");
    Serial.print(delta_time);
    Serial.print(",");
    Serial.print(theseRpms);
    Serial.print(",");
    Serial.print(((float)voltageValue/4096) * (float)VSCALE);
    Serial.print(",");
    Serial.println(((float)currentValue/4096) * (float)CSCALE);

    //the following code makes sure we're running at a constant loop time
    loop_time = micros() - curr_time;
    delayMicroseconds(LOOPDELAY - loop_time);
  }

  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
}



 /*
 ################################
 #    Stepping Test Routine     #
 ################################
 */
void steppingTest() {
  if (LOOPDELAY == 0) {
    Serial.println("Loop delay must be specified");
    return;
  }

  delay(20);
  Serial.println("Begining automated test, press any key to exit");
  delay(2000);

  Serial.print("Time(uS),");
  Serial.print("Throttle(uS),");
  Serial.print("Thrust(g),");
  if(MAGSENS) {
    Serial.print("eSteps,");
  }
  if(OPTISENS) {
    Serial.print("oSteps,");
  }
  if(MAGSENS) {
    Serial.print("eRPMs,");
  }
  if(OPTISENS) {
    Serial.print("oRPMs,");
  }
  Serial.print("Volts,");
  Serial.println("Amps");
  
  startTime=micros();
  isTestRunning = true;
  isTared = false;
  int pwm, prev_pwm;
  ramp r;

  //setup code
  ramp_init(&r);
  prev_pwm = 0;

  //add a six second ramp up/down
  ramp_add_static(&r, IDLEPWM, 2000); //idle for 2 seconds
  ramp_add_range(&r, IDLEPWM, MAXTHROTTLE, 12000); //ramp up over 6 seconds
  ramp_add_static(&r, MAXTHROTTLE, 500); //hold max for 500ms
  ramp_add_range(&r, MAXTHROTTLE, IDLEPWM, 12000); //ramp down over 6 seconds
  ramp_add_static(&r, IDLEPWM, 500); //hold min throttle for 500ms

  //main test loop
  isTestRunning = true;
  while(!Serial.available()) {
    loopStart = micros();
    pwm = ramp_get_pwm(&r, loopStart - startTime);
    if (pwm == -1) {
      isTestRunning = false;
      isTared = false;
      break;
    }
    if(pwm != prev_pwm) {
      updatePWM(pwm);
      prev_pwm = pwm;
    }

    // Grab RPM calculations from last step times.
    if (MAGSENS) {
      theseRpms = calculateRPMs(stepDiff1);
    }
    if (OPTISENS) {
      theseRpms = calculateRPMs(stepDiff2);
    }

    // If no steps have happened in 100ms reset rpms to 0
    // This means that the minimum RPMs the code is capable of detecting is
    // 600 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
    // at about 2000 rpms.
    if(stepTime1 > 100000 || stepTime2 > 100000) {
      theseRpms = 0;
      avgStepDiff1.push(0);
    }

    // Print out data
    Serial.print(loopStart-startTime);
    Serial.print(",");
    Serial.print(pwm);
    Serial.print(",");
    Serial.print(thrust);
    Serial.print(",");
    if(MAGSENS) {
      Serial.print(stepCount1);
    }
    if(OPTISENS) {
      Serial.print(stepCount2);
    }
    Serial.print(",");
    Serial.print(theseRpms);
    Serial.print(",");
    Serial.print(((float)voltageValue/4096) * (float)VSCALE); // Calculate Volts from analog sensor
    Serial.print(",");
    Serial.println(((float)currentValue-2048) * (float)CSCALE); // Calculate Amps from analog sensor

    // Delay here adjusts the sample rate for the RPM sensors, as they are updated asynchronously via the interrupts.
    // Note that cycle times are limited by serial baud rates as well. You can change delay here to just higher than
    // the serial delay to get more stable cycle times.
    // 115200 = ~2ms cycle
    // 230400 = ~1.5ms cycle
    // All faster bauds = ~1ms cycle
    // minimum looptime is set to 1ms for all higher baud rates.

    uint32_t thisDelay;
    uint32_t loopTime = micros() - loopStart;
    if(!LOOPDELAY) {
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
    }
    else {
      thisDelay = LOOPDELAY - loopTime;
    }
    if (thisDelay > 0) {
      delayMicroseconds(thisDelay);
    }
  }

  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
}

/*
################################
 #      Main test Routine       #
 ################################
 */

void mainTest() {
  input="";

  delay(20);
  Serial.println("Begining automated test, press any key to exit");
  delay(2000);

  // Print CSV header output
  Serial.print("Time(uS),");
  Serial.print("Throttle(uS),");
  Serial.print("Thrust(g),");
  if(MAGSENS) {
    Serial.print("eSteps,");
  }
  if(OPTISENS) {
    Serial.print("oSteps,");
  }
  if(MAGSENS) {
    Serial.print("eRPMs,");
  }
  if(OPTISENS) {
    Serial.print("oRPMs,");
  }
  Serial.print("Volts,");
  Serial.println("Amps");

  // Initiate test run
  startTime=micros();
  isTestRunning = true;
  isTared = false;
  int pwm, prev_pwm;
  ramp r;

  //setup our ramp
  ramp_init(&r);

  //six second ramp up
  ramp_add_static(&r, 1250, 2000);
  ramp_add_static(&r, 1100, 2000);
  ramp_add_static(&r, 1500, 2000);
  ramp_add_static(&r, 1100, 2000);
  ramp_add_static(&r, 1750, 2000);
  ramp_add_static(&r, 1100, 2000);
  ramp_add_static(&r, MAXTHROTTLE, 2000);
  ramp_add_static(&r, MINCOMMAND, 2000);
  ramp_add_range(&r, MINTHROTTLE, MAXTHROTTLE, 6000);
  ramp_add_static(&r, MAXTHROTTLE, 2000);
  ramp_add_static(&r, 1100, 2000);
  ramp_add_static(&r, MINCOMMAND, 2000);

  prev_pwm = 0;
  while(!Serial.available() && isTestRunning) {
    loopStart = micros();
    pwm = ramp_get_pwm(&r, loopStart - startTime);
    if (pwm == -1) {
      isTestRunning = false;
      isTared = false;
      break;
    }
    if(pwm != prev_pwm) {
      updatePWM(pwm);
      prev_pwm = pwm;
    }

    // Grab RPM calculations from last step times.
    if (MAGSENS) {
      theseRpms = calculateRPMs(stepDiff1);
    }
    if (OPTISENS) {
      theseRpms = calculateRPMs(stepDiff2);
    }

    // If no steps have happened in 100ms reset rpms to 0
    // This means that the minimum RPMs the code is capable of detecting is
    // 600 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
    // at about 2000 rpms.
    if(stepTime1 > 100000 || stepTime2 > 100000) {
      theseRpms = 0;
      avgStepDiff1.push(0);
    }

    // Print out data
    Serial.print(loopStart-startTime);
    Serial.print(",");
    Serial.print(pwm);
    Serial.print(",");
    Serial.print(thrust);
    Serial.print(",");
    if(MAGSENS) {
      Serial.print(stepCount1);
    }
    if(OPTISENS) {
      Serial.print(stepCount2);
    }
    Serial.print(",");
    Serial.print(theseRpms);
    Serial.print(",");
    Serial.print(((float)voltageValue/4096) * (float)VSCALE); // Calculate Volts from analog sensor
    Serial.print(",");
    Serial.println(((float)currentValue-2048) * (float)CSCALE); // Calculate Amps from analog sensor

    // Delay here adjusts the sample rate for the RPM sensors, as they are updated asynchronously via the interrupts.
    // Note that cycle times are limited by serial baud rates as well. You can change delay here to just higher than
    // the serial delay to get more stable cycle times.
    // 115200 = ~2ms cycle
    // 230400 = ~1.5ms cycle
    // All faster bauds = ~1ms cycle
    // minimum looptime is set to 1ms for all higher baud rates.

    uint32_t thisDelay;
    uint32_t loopTime = micros() - loopStart;
    if(!LOOPDELAY) {
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
    }
    else {
      thisDelay = LOOPDELAY - loopTime;
    }
    if (thisDelay > 0) {
      delayMicroseconds(thisDelay);
    }
  }
  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
}


 /*
 ################################
 #     Latency Test Routine     #
 ################################
 */

void latencyTest() {
  input="";

  delay(20);
  Serial.println("Begining automated test, press any key to exit");
  delay(2000);

  // Print CSV header output
  Serial.print("Time(uS),");
  Serial.print("Throttle(uS),");
  Serial.print("Thrust(g),");
  if(MAGSENS) {
    Serial.print("eSteps,");
  }
  if(OPTISENS) {
    Serial.print("oSteps,");
  }
  if(MAGSENS) {
    Serial.print("eRPMs,");
  }
  if(OPTISENS) {
    Serial.print("oRPMs,");
  }

  // Initiate test run
  startTime=micros();
  isTestRunning = true;
  isTared = false;
  int pwm, prev_pwm;
  ramp r;

  //setup our ramp
  ramp_init(&r);

  //six second ramp up
  ramp_add_static(&r, 1100, 500);
  ramp_add_static(&r, 1250, 250);
  ramp_add_static(&r, 1300, 30);
  ramp_add_static(&r, 1250, 470);
  ramp_add_static(&r, 1300, 20);
  ramp_add_static(&r, 1250, 480);
  ramp_add_static(&r, 1300, 10);
  ramp_add_static(&r, 1250, 490);
  ramp_add_static(&r, 1300, 1);
  ramp_add_static(&r, 1250, 249);
  ramp_add_static(&r, 1500, 250);
  ramp_add_static(&r, 1550, 30);
  ramp_add_static(&r, 1500, 470);
  ramp_add_static(&r, 1550, 20);
  ramp_add_static(&r, 1500, 480);
  ramp_add_static(&r, 1550, 10);
  ramp_add_static(&r, 1500, 490);
  ramp_add_static(&r, 1550, 1);
  ramp_add_static(&r, 1500, 249);
  ramp_add_static(&r, 1750, 250);
  ramp_add_static(&r, 1800, 30);
  ramp_add_static(&r, 1750, 470);
  ramp_add_static(&r, 1800, 20);
  ramp_add_static(&r, 1750, 480);
  ramp_add_static(&r, 1800, 10);
  ramp_add_static(&r, 1750, 490);
  ramp_add_static(&r, 1800, 1);
  ramp_add_static(&r, 1750, 249);
  ramp_add_static(&r, 1100, 499);
  ramp_add_static(&r, MINCOMMAND, 500);

  prev_pwm = 0;
  while(!Serial.available() && isTestRunning) {
    loopStart = micros();
    pwm = ramp_get_pwm(&r, loopStart - startTime);
    if (pwm == -1) {
      isTestRunning = false;
      isTared = false;
      break;
    }
    if(pwm != prev_pwm) {
      updatePWM(pwm);
      prev_pwm = pwm;
    }

    // Grab RPM calculations from last step times.
    if (MAGSENS) {
      theseRpms = calculateRPMs(stepDiff1);
    }
    if (OPTISENS) {
      theseRpms = calculateRPMs(stepDiff2);
    }

    // If no steps have happened in 100ms reset rpms to 0
    // This means that the minimum RPMs the code is capable of detecting is
    // 600 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
    // at about 2000 rpms.
    if(stepTime1 > 100000 || stepTime2 > 100000) {
      theseRpms = 0;
      avgStepDiff1.push(0);
    }

    // Print out data
    Serial.print(loopStart-startTime);
    Serial.print(",");
    Serial.print(pwm);
    Serial.print(",");
    Serial.print(thrust);
    Serial.print(",");
    if(MAGSENS) {
      Serial.print(stepCount1);
    }
    if(OPTISENS) {
      Serial.print(stepCount2);
    }
    Serial.print(",");
    Serial.println(theseRpms);
  }
  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
}

/*
################################
#       KV test Routine        #
################################
*/

void kvTest() {
   input="";

  delay(20);
  Serial.println("Begining automated test, press any key to exit");
  delay(2000);

  // Print CSV header output
  Serial.print("Time(uS),");
  Serial.print("Throttle(uS),");
  Serial.print("Thrust(g),");
  if(MAGSENS) {
    Serial.print("eSteps,");
  }
  if(OPTISENS) {
    Serial.print("oSteps,");
  }
  if(MAGSENS) {
    Serial.print("eRPMs,");
  }
  if(OPTISENS) {
    Serial.print("oRPMs,");
  }
  Serial.print("Volts,");
  Serial.println("Amps");

  // Initiate test run
  startTime=micros();
  isTestRunning = true;
  isTared = false;
  int pwm, prev_pwm;
  ramp r;

  //setup our ramp
  ramp_init(&r);

  //six second ramp up
  ramp_add_range(&r, MINTHROTTLE, MAXTHROTTLE, 3000);
  ramp_add_static(&r, MAXTHROTTLE, 2000);
  ramp_add_static(&r, 1100, 2000);
  ramp_add_static(&r, MINCOMMAND, 2000);

  prev_pwm = 0;
  while(!Serial.available() && isTestRunning) {
    loopStart = micros();
    pwm = ramp_get_pwm(&r, loopStart - startTime);
    if (pwm == -1) {
      isTestRunning = false;
      isTared = false;
      break;
    }
    if(pwm != prev_pwm) {
      updatePWM(pwm);
      prev_pwm = pwm;
    }

    // Grab RPM calculations from last step times.
    if (MAGSENS) {
      theseRpms = calculateRPMs(stepDiff1);
    }
    if (OPTISENS) {
      theseRpms = calculateRPMs(stepDiff2);
    }

    // If no steps have happened in 100ms reset rpms to 0
    // This means that the minimum RPMs the code is capable of detecting is
    // 600 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
    // at about 2000 rpms.
    if(stepTime1 > 100000 || stepTime2 > 100000) {
      theseRpms = 0;
      avgStepDiff1.push(0);
    }

    // Print out data
    Serial.print(loopStart-startTime);
    Serial.print(",");
    Serial.print(pwm);
    Serial.print(",");
    Serial.print(thrust);
    Serial.print(",");
    if(MAGSENS) {
      Serial.print(stepCount1);
    }
    if(OPTISENS) {
      Serial.print(stepCount2);
    }
    Serial.print(",");
    Serial.print(theseRpms);
    Serial.print(",");
    Serial.print(((float)voltageValue/4096) * (float)VSCALE); // Calculate Volts from analog sensor
    Serial.print(",");
    Serial.println(((float)currentValue-2048) * (float)CSCALE); // Calculate Amps from analog sensor

    // Delay here adjusts the sample rate for the RPM sensors, as they are updated asynchronously via the interrupts.
    // Note that cycle times are limited by serial baud rates as well. You can change delay here to just higher than
    // the serial delay to get more stable cycle times.
    // 115200 = ~2ms cycle
    // 230400 = ~1.5ms cycle
    // All faster bauds = ~1ms cycle
    // minimum looptime is set to 1ms for all higher baud rates.

    uint32_t thisDelay;
    uint32_t loopTime = micros() - loopStart;
    if(!LOOPDELAY) {
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
    }
    else {
      thisDelay = LOOPDELAY - loopTime;
    }
    if (thisDelay > 0) {
      delayMicroseconds(thisDelay);
    }
  }
  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
}

 /*
 ################################
 #     Custom test Routine      #
 ################################
 */

void customTest() {
  input="";

  delay(20);
  Serial.println("Begining automated test, press any key to exit");
  delay(2000);

  // Print CSV header output
  Serial.print("Time(uS),");
  Serial.print("Throttle(uS),");
  Serial.print("Thrust(g),");
  if(MAGSENS) {
    Serial.print("eSteps,");
  }
  if(OPTISENS) {
    Serial.print("oSteps,");
  }
  if(MAGSENS) {
    Serial.print("eRPMs,");
  }
  if(OPTISENS) {
    Serial.print("oRPMs,");
  }
  Serial.print("Volts,");
  Serial.println("Amps");

  // Initiate test run
  startTime=micros();
  isTestRunning = true;
  isTared = false;
  int pwm, prev_pwm;
  ramp r;

  //setup our ramp
  ramp_init(&r);

  //six second ramp up
  ramp_add_static(&r, 1100, 2000);
  ramp_add_static(&r, 1185, 2000);
  ramp_add_static(&r, 1100, 2000);
  ramp_add_static(&r, 1190, 2000);
  ramp_add_static(&r, 1100, 2000);
  ramp_add_range(&r, 1100, 1185, 6000);
  ramp_add_static(&r, 1100, 2000);
  ramp_add_range(&r, 1100, 1190, 6000);
  ramp_add_static(&r, 1100, 2000);
  ramp_add_static(&r, MINCOMMAND, 2000);

  prev_pwm = 0;
  while(!Serial.available() && isTestRunning) {
    loopStart = micros();
    pwm = ramp_get_pwm(&r, loopStart - startTime);
    if (pwm == -1) {
      isTestRunning = false;
      isTared = false;
      break;
    }
    if(pwm != prev_pwm) {
      updatePWM(pwm);
      prev_pwm = pwm;
    }

    // Grab RPM calculations from last step times.
    if (MAGSENS) {
      theseRpms = calculateRPMs(stepDiff1);
    }
    if (OPTISENS) {
      theseRpms = calculateRPMs(stepDiff2);
    }

    // If no steps have happened in 100ms reset rpms to 0
    // This means that the minimum RPMs the code is capable of detecting is
    // 600 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
    // at about 2000 rpms.
    if(stepTime1 > 100000 || stepTime2 > 100000) {
      theseRpms = 0;
      avgStepDiff1.push(0);
    }

    // Print out data
    Serial.print(loopStart-startTime);
    Serial.print(",");
    Serial.print(pwm);
    Serial.print(",");
    Serial.print(thrust);
    Serial.print(",");
    if(MAGSENS) {
      Serial.print(stepCount1);
    }
    if(OPTISENS) {
      Serial.print(stepCount2);
    }
    Serial.print(",");
    Serial.print(theseRpms);
    Serial.print(",");
    Serial.print(((float)voltageValue/4096) * (float)VSCALE); // Calculate Volts from analog sensor
    Serial.print(",");
    Serial.println(((float)currentValue-2048) * (float)CSCALE); // Calculate Amps from analog sensor

    // Delay here adjusts the sample rate for the RPM sensors, as they are updated asynchronously via the interrupts.
    // Note that cycle times are limited by serial baud rates as well. You can change delay here to just higher than
    // the serial delay to get more stable cycle times.
    // 115200 = ~2ms cycle
    // 230400 = ~1.5ms cycle
    // All faster bauds = ~1ms cycle
    // minimum looptime is set to 1ms for all higher baud rates.

    uint32_t thisDelay;
    uint32_t loopTime = micros() - loopStart;
    if(!LOOPDELAY) {
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
    }
    else {
      thisDelay = LOOPDELAY - loopTime;
    }
    if (thisDelay > 0) {
      delayMicroseconds(thisDelay);
    }
  }
  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
}

