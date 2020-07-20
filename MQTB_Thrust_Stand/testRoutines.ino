

/*
################################
 #     Scale Tare Routine       #
 ################################
 */

void tareScale() {
  delay(20);
  input="";
  
  if(USE_LOAD_CELL) {
    Serial.println("Taring");
    scale.tare();
  } else {
    Serial.println("No Load Cell Configured");
  }
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
    Serial.println("Load Cell Calibration Value: ");
    Serial.print("LSCALE: ");
    Serial.println(LSCALE);
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
    Serial.println("Electrical Sensor Calibration Values: ");
    Serial.print("CSCALE: ");
    Serial.print(CSCALE);
    Serial.print(" COFFSET: ");
    Serial.print(COFFSET);
    Serial.print(" VSCALE: ");
    Serial.print(VSCALE);
    Serial.print(" VOFFSET: ");
    Serial.println(VOFFSET);
    Serial.print("Battery Voltage: ");
    Serial.print(getVoltage(voltageValue)); // Calculate Volts from analog sensor
    Serial.print(", Current: ");
    Serial.println(getCurrent(currentValue)); // Calculate Amps from analog sensor
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
 // Initiate test run
  startTime=micros();
  isTestRunning = true;
  isTared = false;
  int pwm, prev_pwm;
  ramp r;

  //setup our ramp
  ramp_init(&r);

  //six second ramp up
  ramp_add_range(&r, MINCOMMAND, IDLEPWM, 500);  // Ramp to IDLE for slower startup
  ramp_add_static(&r, IDLEPWM, 3500);          // Run at IDLE for time in milliseconds
  ramp_add_static(&r, MINCOMMAND, 1000);       // Drop back to mincommand to stop motor

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
  }
  
  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
  isTared = false;
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
  if(USE_LOAD_CELL) {
    Serial.print("Thrust(g),");
  }
  if(RUN_RPM_TEST) {
      for(int i = 1; i<=USE_MOTORS; i++) {
        String stepTitle = "eSteps "+i;
        String rpmTitle = "eRPMs "+i;
        Serial.print(stepTitle+",");
        Serial.print(rpmTitle+",");
      }
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


  ramp_add_range(&r, MINCOMMAND, IDLEPWM, 1000);  // Ramp to IDLE for slower startup
  ramp_add_static(&r, IDLEPWM, 2000);
  ramp_add_static(&r, 1250, 2000);
  ramp_add_static(&r, IDLEPWM, 2000);
  ramp_add_static(&r, 1500, 2000);
  ramp_add_static(&r, IDLEPWM, 2000);
  ramp_add_static(&r, 1750, 2000);
  ramp_add_static(&r, IDLEPWM, 2000);
  ramp_add_static(&r, MAXTHROTTLE, 2000);
  ramp_add_static(&r, MINCOMMAND, 2000);
  ramp_add_range(&r, MINTHROTTLE, MAXTHROTTLE, 6000);
  ramp_add_static(&r, MAXTHROTTLE, 2000);
  ramp_add_static(&r, IDLEPWM, 2000);
  ramp_add_static(&r, MINCOMMAND, 2000);

  prev_pwm = 0;
  
  while(!Serial.available() && isTestRunning) {
    loopStart = micros();
    pwm = ramp_get_pwm(&r, loopStart - startTime);
    if (pwm == -1) {
      isTestRunning = false;
      isTared = false;
      delay(20);
      Serial.println("Test Ended!");
      delay(2000);
      break;
    }
    if(pwm != prev_pwm) {
      updatePWM(pwm);
      prev_pwm = pwm;
    }
    if(RUN_RPM_TEST) {
      // Grab RPM calculations from last step times.
      theseRpms[1] = calculateRPMs(stepDiff[1]);
  
      // If no steps have happened in 100ms reset rpms to 0
      // This means that the minimum RPMs the code is capable of detecting is
      // 600 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
      // at about 2000 rpms.
      if(stepTime[1] > 100000) {
        theseRpms[1] = 0;
        avgStepDiff[1].push(0);
      }
    }
    int32_t sampleTime = loopStart-startTime-3000000;  //Subtract 3 seconds of time to start the counter at 0 when the testing sequence begins.

    if(sampleTime >= 0) {
      // Print out data
      Serial.print(sampleTime);
      Serial.print(",");
      Serial.print(pwm);
      Serial.print(",");
      if(USE_LOAD_CELL) {
        Serial.print(thrust);
        Serial.print(",");
      }
      if(RUN_RPM_TEST) {
        
        for(int i = 1; i<=USE_MOTORS; i++) {
          Serial.print(stepCount[i]);
          Serial.print(",");
          Serial.print(theseRpms[i]);
          Serial.print(",");
        }
      }
      Serial.print(getVoltage(voltageValue)); // Calculate Volts from analog sensor
      Serial.print(",");
      Serial.print(getCurrent(currentValue)); // Calculate Amps from analog sensor
      Serial.println("");
  
      // Delay here adjusts the sample rate for the RPM sensors, as they are updated asynchronously via the interrupts.
      // Note that cycle times are limited by serial baud rates as well. You can change delay here to just higher than
      // the serial delay to get more stable cycle times.
      // 115200 = ~2ms cycle
      // 230400 = ~1.5ms cycle
      // All faster bauds = ~1ms cycle
      // minimum looptime is set to 1ms for all higher baud rates.
  
      int32_t thisDelay;
      uint32_t loopTime = micros() - loopStart;
      if(!loopDelay) {
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
        thisDelay = loopDelay - loopTime;
      }
      if (thisDelay > 0) {
        delayMicroseconds(thisDelay);
      }
    }
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
  Serial.print("eSteps,");
  Serial.print("eRPMs,");
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
  ramp_add_static(&r, IDLEPWM, 2000);
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
    theseRpms[1] = calculateRPMs(stepDiff[1]);

    // If no steps have happened in 100ms reset rpms to 0
    // This means that the minimum RPMs the code is capable of detecting is
    // 600 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
    // at about 2000 rpms.
    if(stepTime[1] > 100000) {
      theseRpms[1] = 0;
      avgStepDiff[1].push(0);
    }

    // Print out data
    Serial.print(PriUint64<DEC>(loopStart-startTime));
    Serial.print(",");
    Serial.print(pwm);
    Serial.print(",");
    Serial.print(thrust);
    Serial.print(",");
    Serial.print(stepCount[1]);
    Serial.print(",");
    Serial.print(theseRpms[1]);
    Serial.print(",");
    Serial.print(getVoltage(voltageValue)); // Calculate Volts from analog sensor
    Serial.print(",");
    Serial.println(getCurrent(currentValue)); // Calculate Amps from analog sensor

    // Delay here adjusts the sample rate for the RPM sensors, as they are updated asynchronously via the interrupts.
    // Note that cycle times are limited by serial baud rates as well. You can change delay here to just higher than
    // the serial delay to get more stable cycle times.
    // 115200 = ~2ms cycle
    // 230400 = ~1.5ms cycle
    // All faster bauds = ~1ms cycle
    // minimum looptime is set to 1ms for all higher baud rates.

    int32_t thisDelay;
    uint64_t loopTime = micros() - loopStart;
    if(!loopDelay) {
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
      thisDelay = loopDelay - loopTime;
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
  if(USE_LOAD_CELL) {
    Serial.print("Thrust(g),");
  }
  if(RUN_RPM_TEST) {
      for(int i = 1; i<=USE_MOTORS; i++) {
        String stepTitle = "eSteps "+i;
        String rpmTitle = "eRPMs "+i;
        Serial.print(stepTitle+",");
        Serial.print(rpmTitle+",");
      }
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
  
  /*
  ramp_add_static(&r, MINCOMMAND, 4000);          // 4 Seconds off
  ramp_add_range(&r, MINCOMMAND, IDLEPWM, 1000);  // Ramp to IDLE for slower startup
  ramp_add_static(&r, IDLEPWM, 10000);            // 10 Seconds at Idle
  ramp_add_static(&r, MAXTHROTTLE, 40000);        // 40 Seconds at Max
  ramp_add_static(&r, IDLEPWM, 10000);            // 10 Seconds at Idle
  ramp_add_static(&r, MINCOMMAND, 5000);          // 5 Seconds off
  */

  
  ramp_add_range(&r, MINCOMMAND, IDLEPWM, 500);  // Ramp to IDLE for slower startup
  ramp_add_static(&r, IDLEPWM, 1000);            // 1 Second at Idle
  ramp_add_static(&r, 1200, 500);            // 1 Second at 1200
  ramp_add_static(&r, 1250, 500);            // 1 Second at 1300
  ramp_add_static(&r, 1200, 500);            // 1 Second at 1200
  ramp_add_static(&r, IDLEPWM, 1000);         // 1 Second at Idle
  ramp_add_static(&r, 1300, 500);            
  ramp_add_static(&r, 1350, 500);            
  ramp_add_static(&r, 1300, 500);            
  ramp_add_static(&r, IDLEPWM, 1000);         
  ramp_add_static(&r, 1400, 500);           
  ramp_add_static(&r, 1450, 500);           
  ramp_add_static(&r, 1400, 500);           
  ramp_add_static(&r, IDLEPWM, 1000);        
  ramp_add_static(&r, 1500, 500);          
  ramp_add_static(&r, 1550, 500);          
  ramp_add_static(&r, 1500, 500);          
  ramp_add_static(&r, IDLEPWM, 1000);        
  ramp_add_static(&r, 1600, 500);           
  ramp_add_static(&r, 1650, 500);           
  ramp_add_static(&r, 1600, 500);           
  ramp_add_static(&r, IDLEPWM, 1000);        
  ramp_add_static(&r, 1700, 500);           
  ramp_add_static(&r, 1750, 500);           
  ramp_add_static(&r, 1700, 500);           
  ramp_add_static(&r, IDLEPWM, 1000);       
  ramp_add_static(&r, 1800, 500);          
  ramp_add_static(&r, 1850, 500);          
  ramp_add_static(&r, 1800, 500);          
  ramp_add_static(&r, IDLEPWM, 1000);       
  ramp_add_static(&r, 1900, 500);          
  ramp_add_static(&r, 1950, 500);    
  ramp_add_static(&r, 1900, 500);          
  ramp_add_static(&r, IDLEPWM, 1000);       

  prev_pwm = 0;
  
  while(!Serial.available() && isTestRunning) {
    loopStart = micros();
    pwm = ramp_get_pwm(&r, loopStart - startTime);
    if (pwm == -1) {
      isTestRunning = false;
      isTared = false;
      delay(20);
      Serial.println("Test Ended!");
      delay(2000);
      break;
    }
    if(pwm != prev_pwm) {
      updatePWM(pwm);
      prev_pwm = pwm;
    }
    if(RUN_RPM_TEST) {
      // Grab RPM calculations from last step times.
      theseRpms[1] = calculateRPMs(stepDiff[1]);
  
      // If no steps have happened in 100ms reset rpms to 0
      // This means that the minimum RPMs the code is capable of detecting is
      // 600 RPMs.  This shouldn't matter as pretty much every ESC starts out minimum
      // at about 2000 rpms.
      if(stepTime[1] > 100000) {
        theseRpms[1] = 0;
        avgStepDiff[1].push(0);
      }
    }

    
    int64_t sampleTime = loopStart-startTime;  

    if(sampleTime >= 0) {
      // Print out data
      Serial.print(PriUint64<DEC>(sampleTime));
      Serial.print(",");
      Serial.print(pwm);
      Serial.print(",");
      if(USE_LOAD_CELL) {
        Serial.print(thrust);
        Serial.print(",");
      }
      if(RUN_RPM_TEST) {
        
        for(int i = 1; i<=USE_MOTORS; i++) {
          Serial.print(stepCount[i]);
          Serial.print(",");
          Serial.print(theseRpms[i]);
          Serial.print(",");
        }
      }
      Serial.print(getVoltage(voltageValue)); // Calculate Volts from analog sensor
      Serial.print(",");
      Serial.print(getCurrent(currentValue)); // Calculate Amps from analog sensor
      Serial.println("");

      // Delay here adjusts the sample rate for the RPM sensors, as they are updated asynchronously via the interrupts.
      // Note that cycle times are limited by serial baud rates as well. You can change delay here to just higher than
      // the serial delay to get more stable cycle times.
      // 115200 = ~2ms cycle
      // 230400 = ~1.5ms cycle
      // All faster bauds = ~1ms cycle
      // minimum looptime is set to 1ms for all higher baud rates.
  
      int32_t thisDelay;
      uint64_t loopTime = micros() - loopStart;
      if(!loopDelay) {
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
        thisDelay = loopDelay - loopTime;
      }
      if (thisDelay > 0) {
        delayMicroseconds(thisDelay);
      }
    }
  }
  while(Serial.available()) {
    character = Serial.read();
    delay(1);
  }
}
