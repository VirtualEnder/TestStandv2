

// RPM Count and calculations

void countRpms () {
  if(isTestRunning) {
    uint32_t nowMicros = micros();
    uint32_t lastStep = stepTime;
    stepTime = nowMicros;
    // Calculate RPMs from step time.
    sprintf(thisFormula, "%lu - %lu", nowMicros, lastStep);
    int64_t thisDiff = nowMicros - lastStep;
    RPMs1 = thisDiff;
    stepCount1++;    // Increase Step counter
  }
}

void countRpms2 () {
 if(isTestRunning) {
    uint64_t stepMicros2 = micros();
    uint64_t lastStep2 = stepTime2;
    stepTime2 = stepMicros2;
    // Calculate RPMs from step time.
    int64_t thisDiff = stepMicros2 - lastStep2;
    if (thisDiff > 200 && thisDiff < 5000) {
      RPMs2 = thisDiff;
    }
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

