

// RPM Count and calculations

void countRpms () {
  if(isTestRunning) {
    uint64_t stepMicros1 = micros();
    uint64_t lastStep1 = stepTime1;
    stepTime1 = stepMicros1;
    // Calculate RPMs from step time.
    RPMs1 = stepMicros1 - lastStep1;
    stepCount1++;    // Increase Step counter
  }
}

void countRpms2 () {
  if(isTestRunning) {
    uint64_t stepMicros2 = micros();
    uint64_t lastStep2 = stepTime2;
    stepTime2 = stepMicros2;
    // Calculate RPMs from step time.
    RPMs2 = stepMicros2 - lastStep2;
    stepCount2++;    // Increase Step counter
  }
}
uint32_t calculateRPMs (uint32_t thisTime) {
  if(thisTime == 0)
    return 0;
  else
    return ((((float)1/(float)(thisTime))*1000000)/(POLES/2))*60;
}

