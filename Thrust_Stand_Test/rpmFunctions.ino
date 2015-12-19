

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
uint32_t calculateRPMs (int64_t thisTime) {
  
  return thisTime;
  //if (thisTime >= 3000)
    // If delay greater than limit, set RPMs to 0 rather than save last RPM infintely
   // return 0;
 // else if (thisTime > 0)
   // return ((((float)1/(float)(thisTime))*1000000)/(POLES/2))*60;

}

