
// Hardware Timers

void adcTimer (unsigned Hz) { 
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
  
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);      //Enable control of GPIO B
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);      //Enable control of GPIO C
      SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);      //Enable control of GPIO E
        
      if(ESCOUTPUT > 4) {
        
        // Use PWM module
      
        if(ESCOUTPUT == 0) {
          SysCtlPWMClockSet( SYSCTL_PWMDIV_2 );
        }
        SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);       //Enable control of PWM module 0
    
        GPIOPinConfigure(GPIO_PB4_M0PWM2);                // Map PB4 to PWM0 G1, OP 2
        GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_4);      //Configure PB4 as PWM
  
        GPIOPinConfigure(GPIO_PE4_M0PWM4);                // Map PE4 to PWM0 G2, OP 4
        GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4);      //Configure PE4 as PWM
        
        GPIOPinConfigure(GPIO_PC4_M0PWM6);                // Map PC4 to PWM0 G3, OP 6
        GPIOPinTypePWM(GPIO_PORTC_BASE, GPIO_PIN_4);      //Configure PC4 as PWM
  
        GPIOPinConfigure(GPIO_PC5_M0PWM7);                // Map PC5 to PWM0 G3, OP 7
        GPIOPinTypePWM(GPIO_PORTC_BASE, GPIO_PIN_5);      //Configure PC5 as PWM
        
        uint32_t PWMPeriod;
        if(ESCOUTPUT == 0) {
          PWMPeriod = (SysCtlClockGet()/2) / ESCRATE;
        } else {
          PWMPeriod = (SysCtlClockGet()) / ESCRATE;
        }
        PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);    //Configure PWM0 G0 as UP/DOWN counter with no sync of updates
        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PWMPeriod - 1);    //Set period of PWM0 G1
        PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);    //Configure PWM0 G0 as UP/DOWN counter with no sync of updates
        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWMPeriod - 1);    //Set period of PWM0 G2
        PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);    //Configure PWM0 G0 as UP/DOWN counter with no sync of updates
        PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, PWMPeriod - 1);    //Set period of PWM0 G2
  
        updatePWM(MINCOMMAND);
        
        PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT , true);    //Enable OP 2 on PWM0 G1
        PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT , true);    //Enable OP 2 on PWM0 G1
        PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT , true);    //Enable OP 2 on PWM0 G1
        PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT , true);    //Enable OP 2 on PWM0 G1
  
  
        
        PWMGenEnable(PWM0_BASE, PWM_GEN_1);    //Enable PWM0, G1 */
        PWMGenEnable(PWM0_BASE, PWM_GEN_2);    //Enable PWM0, G2 */
        PWMGenEnable(PWM0_BASE, PWM_GEN_3);    //Enable PWM0, G3 */
      } else {
        
        // Enable GPIO on dshot output pins
        GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4);      //Configure PB4 as I/O
        GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_4);      //Configure PE4 as I/O
        GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_4);      //Configure PC4 as I/O
        GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_5);      //Configure PC5 as I/O

        
        
        //Set up DSHOT timer and DMA HERE EXAMPLE DMA: https://sites.google.com/site/luiselectronicprojects/tutorials/tiva-tutorials/tiva-dma/blink-with-dma
        // Set up Timer 3A as dShot Timer
        SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);  
        TimerConfigure(TIMER3_BASE, TIMER_CFG_PERIODIC);

        TimerLoadSet(TIMER3_BASE, TIMER_A, 1);
        
        TimerIntClear(TIMER3_BASE,TIMER_TIMA_DMA);
        TimerIntRegister(TIMER3_BASE,TIMER_A,dshotTimer);
        TimerIntEnable(TIMER3_BASE,TIMER_TIMA_DMA);
        
        TimerDMAEventSet(TIMER3_BASE,TIMER_DMA_TIMEOUT_A);

        //Set up DMA
        SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
        uDMAEnable();
        uDMAControlBaseSet(DMAcontroltable);
        
        //Set the channel trigger to be Timer3A
        uDMAChannelAssign(UDMA_CH2_TIMER3A);
        
        //Disable all the atributes in case any was set
        uDMAChannelAttributeDisable(UDMA_CH2_TIMER3A,
        UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST |
          UDMA_ATTR_HIGH_PRIORITY |
          UDMA_ATTR_REQMASK);
      
        /*
          This sets up the item size to 16bits, source increment to 16bits
          and destination increment to none and arbitration size to 1
        */
        uDMAChannelControlSet(UDMA_CH2_TIMER3A | UDMA_PRI_SELECT,
        UDMA_SIZE_16 | UDMA_SRC_INC_16 | UDMA_DST_INC_NONE |
          UDMA_ARB_1);
        
        /*
          This will setup the transfer mode to basic, source address to the array we want
          and destination address to the GPIO state we chosed. It also sets the total transfer
          size to 16.
        */
        uDMAChannelTransferSet(UDMA_CH2_TIMER3A | UDMA_PRI_SELECT,
          UDMA_MODE_BASIC,
          dshotPacket, (void *)(GPIO_PORTB_BASE + GPIO_PIN_4),
          16);
      
      
        //Enable the DMA chanel
        uDMAChannelEnable(UDMA_CH2_TIMER3A);

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
    int pwmOutputs[] = {
                    0 ,
                    PWM_OUT_2,
                    PWM_OUT_4,
                    PWM_OUT_6,
                    PWM_OUT_7};
      
    // Convert 1000-2000us range to 125-250us range and apply to PWM output
    uint32_t dutyCycle = (pulseWidth * pwmMultiplier)/100;
    if(ESCOUTPUT == 3) {
      dutyCycle -= 1350;
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

    dshotUserInputValue = map(pulseWidth,1000,2000,0, 2048);
        
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

void dshotTimer() {
  TimerIntClear(TIMER3_BASE,TIMER_TIMA_DMA);
    
  //Set again the same source address and destination
  uDMAChannelTransferSet(UDMA_CH2_TIMER3A | UDMA_PRI_SELECT,
          UDMA_MODE_BASIC,
          dshotPacket, (void *)(GPIO_PORTB_BASE + GPIO_PIN_4),
          16);
  
  //Always needed since after it's done the DMA is disabled when in basic mode
  uDMAChannelEnable(UDMA_CH2_TIMER3A);
}
