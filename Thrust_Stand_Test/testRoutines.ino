

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
    while(Serial.available()) {
        character = Serial.read();
        delay(1);
    }
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
        Serial.println("mPRMs,");
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
        Serial.print(thrust);
        Serial.print(",");
        if(MAGSENS) {
          Serial.print(stepCount1);
        }
        if(OPTISENS) {
          Serial.print(stepCount2);
        }
        Serial.print(",");
        Serial.print(escMicros);
        Serial.print(",");
        Serial.print(currentLoopTime);
        Serial.print(",");
        if(MAGSENS) { 
          theseRpms = calculateRPMs(stepDiff1);
        }
        if(OPTISENS) {
          theseRpms = calculateRPMs(stepDiff2);
        }
        Serial.println(theseRpms);
        
      }
    } else {
      Serial.println("Throttle positions for target RPMs could not be aquired, aborting test!");
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
    Serial.print("Volts,");
    Serial.println("Amps");
    
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
        escMicros = 1750;
      else if(currentLoopTime<12000000)
        escMicros = 1100;
      else if(currentLoopTime<14000000)
        escMicros = MAXTHROTTLE;
      else if(currentLoopTime<15000000)
        escMicros = MINCOMMAND;
      else if(currentLoopTime<16000000 && !isTared) {
        // Tare scale between passes to increase accuracy.
        //scale.tare();
        //isTared = true;
      }
      else if(currentLoopTime<22000000)
        // Iterate through whole throttle range based on time
        escMicros = (((float)(currentLoopTime-16000000)/6000000.0)*(2000-MINTHROTTLE))+ MINTHROTTLE;   
      else if(currentLoopTime<24000000)
        escMicros = MAXTHROTTLE;
      else if(currentLoopTime<=26000000)
        escMicros = 1100;
      else if(currentLoopTime<=28000000)
        escMicros = MINCOMMAND;
      else {
        isTestRunning = false;
        isTared = false;
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
      Serial.print(thrust);
      Serial.print(",");
      if(MAGSENS) {
        Serial.print(stepCount1);
      }
      if(OPTISENS) {
        Serial.print(stepCount2);
      }
      Serial.print(",");
      Serial.print(escMicros);
      Serial.print(",");
      Serial.print(currentLoopTime);
      Serial.print(",");
      Serial.print(theseRpms);
      Serial.print(",");
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
      } else {
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
