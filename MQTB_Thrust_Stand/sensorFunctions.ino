// Current and Voltage functions

float getVoltage(int analogValue) {
    return ((((float)analogValue/4096) * (float)VSCALE) + VOFFSET); // Calculate Volts from analog sensor
}
float getCurrent(int analogValue) {
    return ((((float)analogValue-2048) * (float)CSCALE) + COFFSET); // Calculate Amps from analog sensor
}

// RPM Count and calculations

void countRpms () {
  if(isTestRunning) {
    stepDiff1 = stepTime1;
    //save the value in a rotating array for averaging
    //pre-divide by STEP_COUNT to simplify averaging code
    step_times[step_idx++ % STEP_COUNT] = stepDiff1 / STEP_COUNT;

    stepTime1 = 0;   // Reset step timer variable
    stepCount1++;    // Increase Step counter
  }
}

void countRpms2 () {
  if(isTestRunning) {
    stepDiff2 = stepTime2;
    stepTime2 = 0;   // Reset step timer variable
    stepCount2++;    // Increase Step counter
  }
}

int calculateRPMs (int thisTime, boolean useAverage = true) {
  static int last_ret = 0;

  // Filter bad values outside the range of ~52000 - 600 RPMs
  if (thisTime > 165 && thisTime < 100000) {
    if(USEAVG && useAverage) {
      thisTime = avgStepDiff1.rolling(thisTime);
    }
    int outRPMs = (120000000/(thisTime*POLES));
    if(outRPMs < 52000 && outRPMs > 600)
      last_ret = outRPMs;
  }
  return last_ret;
}

int averageRPMs() {
  int result = 0;
  int i;
  uint32_t curr_num = step_idx;

  //if we haven't acquired 20 readings yet, then we
  //need to do some averaging magic
  if (curr_num < STEP_COUNT) {
    for (i = 0; i < step_idx; i++) {
      result += (step_times[i] * 20) / curr_num;
    }
  } else {
    for (i = 0; i < STEP_COUNT; i++) {
      result += step_times[i];
    }
  }
  return (120000000/result*POLES);
}
