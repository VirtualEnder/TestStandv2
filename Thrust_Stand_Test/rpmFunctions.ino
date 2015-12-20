

// RPM Count and calculations

void countRpms () {
  if(isTestRunning) {
    stepDiff1 = stepTime1;
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
    last_ret = (120000000/(thisTime*POLES));
  }
  return last_ret;
}

