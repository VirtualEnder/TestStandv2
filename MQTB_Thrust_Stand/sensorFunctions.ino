// Current and Voltage functions

float getVoltage(int analogValue) {
    return ((((float)analogValue/4096) * (float)VSCALE) + VOFFSET); // Calculate Volts from analog sensor
}
float getCurrent(int analogValue) {
    return ((((float)analogValue-2048) * (float)CSCALE) + COFFSET); // Calculate Amps from analog sensor
}

// RPM Count and calculations

void countRpms (int inputID) {
  if(isTestRunning) {
    stepDiff[inputID] = stepTime[inputID];
    //save the value in a rotating array for averaging
    //pre-divide by STEP_COUNT to simplify averaging code
    step_times[inputID][step_idx[inputID]++ % STEP_COUNT] = stepDiff[inputID] / STEP_COUNT;

    stepTime[inputID] = 0;   // Reset step timer variable
    stepCount[inputID]++;    // Increase Step counter
  }
}

void rpmTrigger1() {
  countRpms(1);
}

void rpmTrigger2() {
  countRpms(2);
}

void rpmTrigger3() {
  countRpms(3);
}

void rpmTrigger4() {
  countRpms(4);
}

int calculateRPMs (int thisTime, int inputID = 1 ,boolean useAverage = true) {
  static int last_ret = 0;

  // Filter bad values outside the range of ~52000 - 600 RPMs
  if (thisTime > 165 && thisTime < 100000) {
    if(USEAVG && useAverage) {
      thisTime = avgStepDiff[inputID].rolling(thisTime);
    }
    int outRPMs = (120000000/(thisTime*MOTOR_POLES));
    if(outRPMs < 52000 && outRPMs > 600)
      last_ret = outRPMs;
  }
  return last_ret;
}

int averageRPMs() {
  int result = 0;
  int i;
  uint32_t curr_num = step_idx[1];

  //if we haven't acquired 20 readings yet, then we
  //need to do some averaging magic
  if (curr_num < STEP_COUNT) {
    for (i = 0; i < step_idx[1]; i++) {
      result += (step_times[1][i] * 20) / curr_num;
    }
  } else {
    for (i = 0; i < STEP_COUNT; i++) {
      result += step_times[1][i];
    }
  }
  return (120000000/result*MOTOR_POLES);
}
