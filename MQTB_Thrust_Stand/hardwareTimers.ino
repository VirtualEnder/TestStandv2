
// Hardware Timers

void adcTimer(unsigned Hz) { 
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  ADCHardwareOversampleConfigure(ADC0_BASE, OVERSAMPLING);
  ADCSequenceDisable(ADC0_BASE, 0);
  
  ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 3);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH2);  // CH2 = PE_1
  ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH2);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_CH2);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 3, ADC_CTL_CH2);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 4, ADC_CTL_CH1);  // CH1 = PE_2
  ADCSequenceStepConfigure(ADC0_BASE, 0, 5, ADC_CTL_CH1);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 6, ADC_CTL_CH1);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 7, ADC_CTL_CH1 | ADC_CTL_IE | ADC_CTL_END);
  ADCSequenceEnable(ADC0_BASE, 0);
  ADCIntClear(ADC0_BASE, 0);
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);  
  TimerConfigure(TIMER1_BASE, TIMER_CFG_SPLIT_PAIR|TIMER_CFG_A_PERIODIC); 
  uint64_t ulPeriod = (SysCtlClockGet () / Hz);
  TimerLoadSet(TIMER1_BASE, TIMER_A, ulPeriod -1); 
  IntEnable(INT_TIMER1A); 
  TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT); 
  TimerIntRegister(TIMER1_BASE, TIMER_A, Timer1IntHandler); 
  TimerEnable(TIMER1_BASE, TIMER_A); 
}

void Timer1IntHandler() {
  TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
  if(isTestRunning) {
    if(ADCIntStatus(ADC0_BASE, 0, false)){
      ADCIntClear(ADC0_BASE, 0);
      ADCSequenceDataGet(ADC0_BASE, 0, ulADC0Value);
      voltageValue = (ulADC0Value[0] + ulADC0Value[1] + ulADC0Value[2] + ulADC0Value[3] + 2)/4;
      currentValue = (ulADC0Value[4] + ulADC0Value[5] + ulADC0Value[6] + ulADC0Value[7] + 2)/4;
    } else {
      ADCIntClear(ADC0_BASE, 0);
      ADCProcessorTrigger(ADC0_BASE, 0);
    }
    
    if(USE_LOAD_CELL) {
      if(scale.is_ready()){
        thrust = scale.get_units();
      }
    }
  }

}

void initPWMOut () { 
    
      if(ESCOUTPUT == 0) {
        SysCtlPWMClockSet( SYSCTL_PWMDIV_2 );
      }
      
      uint32_t PWMPeriod;
      if(ESCOUTPUT == 0) {
        PWMPeriod = (SysCtlClockGet()/2) / ESCRATE;
      } else {
        PWMPeriod = (SysCtlClockGet()) / ESCRATE;
      } 
      
      
      
      SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);       //Enable control of PWM module 0
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);      //Enable control of GPIO B
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);      //Enable control of GPIO C
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);      //Enable control of GPIO E
  
      GPIOPinConfigure(GPIO_PB4_M0PWM2);                // Map PB4 to PWM0 G1, OP 2
      GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_4);      //Configure PB4 as PWM

      GPIOPinConfigure(GPIO_PE4_M0PWM4);                // Map PE4 to PWM0 G2, OP 4
      GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4);      //Configure PE4 as PWM
      
      GPIOPinConfigure(GPIO_PC4_M0PWM6);                // Map PC4 to PWM0 G3, OP 6
      GPIOPinTypePWM(GPIO_PORTC_BASE, GPIO_PIN_4);      //Configure PC4 as PWM

      GPIOPinConfigure(GPIO_PC5_M0PWM7);                // Map PC5 to PWM0 G3, OP 7
      GPIOPinTypePWM(GPIO_PORTC_BASE, GPIO_PIN_5);      //Configure PC5 as PWM
      
      PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);    //Configure PWM0 G0 as UP/DOWN counter with no sync of updates
      PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);    //Configure PWM0 G0 as UP/DOWN counter with no sync of updates
      PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);    //Configure PWM0 G0 as UP/DOWN counter with no sync of updates

      PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PWMPeriod - 1);    //Set period of PWM0 G1
      PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWMPeriod - 1);    //Set period of PWM0 G2
      PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, PWMPeriod - 1);    //Set period of PWM0 G3
      
      updatePWM(MINCOMMAND);
      
      PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT , true);    //Enable OP 2 on PWM0 G1
      PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT , true);    //Enable OP 4 on PWM0 G2
      PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT , true);    //Enable OP 6 on PWM0 G3
      PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT , true);    //Enable OP 7 on PWM0 G3

      
      PWMGenEnable(PWM0_BASE, PWM_GEN_1);    //Enable PWM0, G1 */
      PWMGenEnable(PWM0_BASE, PWM_GEN_2);    //Enable PWM0, G2 */
      PWMGenEnable(PWM0_BASE, PWM_GEN_3);    //Enable PWM0, G3 */
      
      if(ESCOUTPUT == 4) {
        
        for(int i = 1; i<=USE_MOTORS; i++) {
          PWMPulseWidthSet(PWM0_BASE, pwmOutputs[i], 0);    //turn off PWM outputs
        }       

        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);      //Enable control of GPIO A
        SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
        while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI0)){}
        
        SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_SSI0);
        
        //
        // Configure GPIO Pins for SSI2 mode.
        //
        GPIOPinConfigure(GPIO_PA5_SSI0TX);
        GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5);
    
        SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_0,
                SSI_MODE_MASTER, 10000000, 15);
    
        SSIEnable(SSI0_BASE);
        
        
        // Set up Timer 3A as dShot Trigger
        SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);  
        TimerConfigure(TIMER3_BASE, TIMER_CFG_PERIODIC);
        
        // Trigger dShot packet transmission at ESC rate.
        TimerLoadSet(TIMER3_BASE, TIMER_A, (SysCtlClockGet() / ESCRATE) - 1);  
        
        TimerIntClear(TIMER3_BASE,TIMER_TIMA_TIMEOUT);
        TimerIntRegister(TIMER3_BASE,TIMER_A,executeDshot);
        TimerIntEnable(TIMER3_BASE,TIMER_TIMA_TIMEOUT);

        TimerEnable(TIMER3_BASE,TIMER_A); 

        
      }
  
}

void updatePWM(unsigned pulseWidth, unsigned pwmOutput) {
                  
  //Prevent escMicros from overflowing the timer
  if (pulseWidth > MAXTHROTTLE) 
    pulseWidth = MAXTHROTTLE;
  if (pulseWidth < MINCOMMAND)
    pulseWidth = MINCOMMAND;
    
  
  if(ESCOUTPUT < 4) {
      
    // Convert 1000-2000us range to 125-250us range and apply to PWM output
    uint32_t dutyCycle = (pulseWidth * pwmMultiplier)/100;
    if(ESCOUTPUT == 3) {
      dutyCycle -= 1230;
    }
    if(pwmOutput > 0) {
      PWMPulseWidthSet(PWM0_BASE, pwmOutputs[pwmOutput], dutyCycle);    //Set duty cycle of pwm output
    } else {
        for(int i = 1; i<=USE_MOTORS; i++) {
          PWMPulseWidthSet(PWM0_BASE, pwmOutputs[i], dutyCycle);    //Set duty cycle of pwm output
        }
    }
    
  } else {
    //Update dShot here
    if(pulseWidth == MINCOMMAND) {
      dshotUserInputValue = 0;
    } else {
      dshotUserInputValue = map(pulseWidth,1000,2000,48, 2048);
    }
    if(dshotUserInputValue < 48 && dshotUserInputValue != 0) {
      dshotUserInputValue = 48;
    }
    dshotOutput(dshotUserInputValue, requestTelemetry);

    if (requestTelemetry) {                
        requestTelemetry = false;
        receivedBytes = 0;
    }
  }
}

void initRPMCount() {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);  
  TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC); 
  uint64_t ulPeriod = (SysCtlClockGet () / (1000000));
  TimerLoadSet(TIMER2_BASE, TIMER_A, ulPeriod -1); 
  IntEnable(INT_TIMER2A); 
  TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT); 
  TimerIntRegister(TIMER2_BASE, TIMER_A, rpmTimer); 
  TimerEnable(TIMER2_BASE, TIMER_A); 
}

void rpmTimer() {
  TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
  if(isTestRunning) {
      for(int i = 1; i<=USE_MOTORS; i++) {
        stepTime[i]++;
      }
  }
}

void executeDshot() {
  
  TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);


  // Reset variables
  dShotWriteActive = true;    

  bool bIntsOff;
  //
  // Turn interrupts off temporarily.
  //
  bIntsOff = IntMasterDisable();


  //
  // Send the array 16 dshot bits = one dshot frame
  //

  for(uint32_t ui32Index = 0; ui32Index < 16; ui32Index++)
  {
      //
      // Send the data using the "blocking" put function.  This function
      // will wait until there is room in the send FIFO before returning.
      // This allows you to assure that all the data you send makes it into
      // the send FIFO.
      //
      SSIDataPut(SSI0_BASE, dshotPacket[ui32Index]);
  }

  //
  // Wait until SSI0 is done transferring all the data in the transmit FIFO.
  //
  while(SSIBusy(SSI0_BASE))
  {
  }


  //
  // Restore the interrupt state
  //
  if(!bIntsOff)
  {
      IntMasterEnable();
  }
  /*
  //Execute Dshot Packet Transmission here.
  uDMAChannelTransferSet(UDMA_CHANNEL_SSI2TX | UDMA_PRI_SELECT,
                         UDMA_MODE_BASIC, dshotPacket,
                         (void *)(SSI2_BASE + SSI_O_DR),
                         16);
    
  //
  // Now the uDMA SSI2 TX channel is primed to start a
  // transfer.  As soon as the channel is enabled, the peripheral will
  // issue a transfer request and the data transfer will begin.
  //
  uDMAChannelEnable(UDMA_CHANNEL_SSI2TX);
  */
  dShotWriteActive = false;    
  
}
