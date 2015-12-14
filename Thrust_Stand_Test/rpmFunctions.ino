

// RPM Count and calculations

void countRpms () {
  if(isTestRunning) {
    uint64_t stepMicros1 = micros();
    uint64_t lastStep1 = stepTime1;
    stepTime1 = stepMicros1;
    // Calculate RPMs from step time.
    uint16_t thisDiff = stepMicros1 - lastStep1;
    if (thisDiff > 200 && thisDiff < 5000) {
      RPMs1 = thisDiff;
    }
    stepCount1++;    // Increase Step counter
  }
}

void countRpms2 () {
  if(isTestRunning) {
    uint64_t stepMicros2 = micros();
    uint64_t lastStep2 = stepTime2;
    stepTime2 = stepMicros2;
    // Calculate RPMs from step time.
    uint16_t thisDiff = stepMicros2 - lastStep2;
    if (thisDiff > 200 && thisDiff < 5000) {
      RPMs2 = thisDiff;
    }
    stepCount2++;    // Increase Step counter
  }
}
uint32_t calculateRPMs (uint32_t thisTime) {

  if (thisTime >= 3000)
    // If delay greater than limit, set RPMs to 0 rather than save last RPM infintely
    return 0;
  else if (thisTime > 0)
    return ((((float)1/(float)(thisTime))*1000000)/(POLES/2))*60;

}

