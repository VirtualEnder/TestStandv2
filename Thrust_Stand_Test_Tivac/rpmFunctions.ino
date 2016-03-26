

// RPM Count and calculations

void countRpms () {
  if(isTestRunning) {
    stepDiff1 = stepTime1;
<<<<<<< HEAD
=======
    //save the value in a rotating array for averaging
    //pre-divide by STEP_COUNT to simplify averaging code
    step_times[step_idx++ % STEP_COUNT] = stepDiff1 / STEP_COUNT;

>>>>>>> origin/master
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

