//*****************************************************************************
//
//      dshot protocol via tiva SSI     CCS v10     driverlib  2.2.0.295, the newest, Apr 2020
//      ======================================================================================
//
//      - bit cell
//      - dshot frame
//      - information comes from :     blck.mn/.../
//
//      DSHOT           - 16 bit frame:  11 data,  1 status, 4 crc
//                      -  TODO  the Fss / CS brackets each dshot bit cell, adds time - is this critical?
//                      -  TODO
//                      - repetition rate:  32KHz.   +- 3 mSec in this code snippet
//
//      Tiva clocking:  - sysclock 50MHz
//                      - SSI clock 6MHz   (sysclock div 12, should come from SS0 API)
//                      - dshot bit cell divided into 10 slices due to 10-bit SSI character
//
//      BIT CELL:       - ONE bit       11.1111.1000        0x03f8     (tweak)
//                      - ZERO bit      11.1000.0000        0x0380     (tweak)
//
//      SSI0 peripheral - GPIO Port A peripheral (for SSI0 pins)
//                      - SSI0Clk - PA2
//                      - SSI0Fss - PA3
//                      - SSI0Rx  - PA4
//                      - SSI0Tx  - PA5
//
//      The dshot frame SSI routine has been made 'atomic / uninterruptable' - in this snippet
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h" 
#include "inc/hw_timer.h"
#include "inc/hw_ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pin_map.h"
#include "driverlib/ssi.h"
#include "driverlib/udma.h"

#define PART_TM4C123GH6PM        // Set board type to TivaC instead of Stellaris
#define UDMA_CHANNEL_SSI2TX 13   // Set UDMA Channel

 //  16-bit             32-bit                             64-bit
 //  1111111111111000   11111111111111111111111100000000   1111111111111111111111111111111111111111111111110000000000000000   
 //  1111111000000000   11111111111100000000000000000000   1111111111111111111111110000000000000000000000000000000000000000   
 //  0xFFF8             0xFFFFFF00                         0xFFFFFFFFFFFF0000
 //  0xFE00             0xFFF00000                         0xFFFFFF0000000000                                                               

#define ONE    0xFFF8                
#define ZERO   0xFE00    

#define RED_LED   GPIO_PIN_1        // LP pin PF1
#define BLUE_LED  GPIO_PIN_2        // LP pin PF2
#define GREEN_LED GPIO_PIN_3        // LP pin PF3


#define ESCRATE     8000     // PWM update rate for ESC in HZ

    uint64_t    array [] =   {                
                               ONE, ONE, ZERO, ZERO,    // 16 bits/frame   *** TEST DATA HEREIN ***
                               ONE, ZERO, ONE, ZERO,
                               ZERO, ONE, ZERO, ONE,
                               ZERO, ZERO, ONE, ONE
                                };
    uint32_t    ui32Index = 0;
    uint32_t    Frame = (sizeof array) / 2;             // number of characters to send = number dshot bits +1

    // dShot variables
uint8_t receivedBytes = 0;
volatile bool requestTelemetry = false;
bool printTelemetry = true;
uint16_t dshotUserInputValue = 0;
static uint64_t dshotPacket[16];  //Array to save the Dshot Packet states
bool dShotWriteActive = false;

void setup() {
 //  ***********************************************************************
    // Setup the system clock to run at 80 Mhz from PLL with crystal reference
    SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|
                    SYSCTL_OSC_MAIN);


    //  ***********************************************************************
    // Enable and wait for port F, set as output for LEDs
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF))
    {
    }
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);

    
    //  ***********************************************************************
    // Enable the SSI 0 peripheral
    //
    //  - set up the GPIO pins on Port A
    //  - set up the SSI 0
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinConfigure(GPIO_PB7_SSI2TX);                  // this is the DSHOT output stream, the only needed signal
    GPIOPinTypeSSI(GPIO_PORTB_BASE,
                   GPIO_PIN_7);
    //
    // Configure the SSI.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI2))
    {
    }
    SSIConfigSetExpClk(SSI2_BASE, SysCtlClockGet(),
                       SSI_FRF_MOTO_MODE_0,
                       SSI_MODE_MASTER,
                       10000000,
                       15);
    //
    // Enable the SSI module.
    //
    SSIEnable(SSI2_BASE);

    dshotOutput(0,false);

    //****************************************************************************
    //uDMA SSI2 TX
    //****************************************************************************

    //
    // Put the attributes in a known state for the uDMA SSI2TX channel.  These
    // should already be disabled by default.
    //
    uDMAChannelAttributeDisable(UDMA_CHANNEL_SSI2TX,
                                    UDMA_ATTR_ALTSELECT |
                                    UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK);

    //
    // Set the USEBURST attribute for the uDMA SSI2TX channel.  This will
    // force the controller to always use a burst when transferring data from
    // the TX buffer to the SSI0.  This is somewhat more effecient bus usage
    // than the default which allows single or burst transfers.
    //
    uDMAChannelAttributeEnable(UDMA_CHANNEL_SSI2TX, UDMA_ATTR_USEBURST);

    //
    // Configure the control parameters for the SSI2 TX.
    //
    uDMAChannelControlSet(UDMA_CHANNEL_SSI2TX | UDMA_PRI_SELECT,
                              UDMA_SIZE_16 | UDMA_SRC_INC_16 | UDMA_DST_INC_NONE |
                              UDMA_ARB_16);


    //
    // Set up the transfer parameters for the uDMA SSI2 TX channel.  This will
    // configure the transfer source and destination and the transfer size.
    // Basic mode is used because the peripheral is making the uDMA transfer
    // request.  The source is the TX buffer and the destination is the SSI2
    // data register.
    //
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

    //
    // Enable the SSI2 DMA TX/RX interrupts.
    //
    SSIIntEnable(SSI2_BASE, SSI_DMATX);

    //
    // Enable the SSI2 peripheral interrupts.
    //
    IntEnable(INT_SSI2);

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
void loop() {
 GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, RED_LED);
 SysCtlDelay(8000000);
 GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, ~RED_LED);
 SysCtlDelay(8000000);
}

void dshotOutput(uint16_t value, bool telemetry) {
    
    uint16_t packet;
    
    // telemetry bit    
    if (telemetry) {
        packet = (value << 1) | 1;
    } else {
        packet = (value << 1) | 0;
    }

    // https://github.com/betaflight/betaflight/blob/09b52975fbd8f6fcccb22228745d1548b8c3daab/src/main/drivers/pwm_output.c#L523
    int csum = 0;
    int csum_data = packet;
    for (int i = 0; i < 3; i++) {
        csum ^=  csum_data;
        csum_data >>= 4;
    }
    csum &= 0xf;
    packet = (packet << 4) | csum;

    // durations are for dshot600
    // https://blck.mn/2016/11/dshot-the-new-kid-on-the-block/
    // Bit length (total timing period) is 1.67 microseconds (T0H + T0L or T1H + T1L).
    // For a bit to be 1, the pulse width is 1250 nanoseconds (T1H – time the pulse is high for a bit value of ONE)
    // For a bit to be 0, the pulse width is 625 nanoseconds (T0H – time the pulse is high for a bit value of ZERO)
    
    uint64_t steps = 0;
    while(dShotWriteActive == true){}   // Wait for dshot to finish writing
    for (int i = 0; i < 16; i++) {
      dshotPacket[i] = 0;
      
      if(i < 16) {
        if (packet & 0x8000) {
            // construct packet 1
            steps = ONE;          //  11.1111.1000
        } else {
            // construct packet 0
            steps = ZERO;          //  11.1100.0000
        }
        dshotPacket[i]= steps;
        packet <<= 1;
      }
    }
   return;

}

uint8_t update_crc8(uint8_t crc, uint8_t crc_seed){
  uint8_t crc_u, i;
  crc_u = crc;
  crc_u ^= crc_seed;
  for ( i=0; i<8; i++) crc_u = ( crc_u & 0x80 ) ? 0x7 ^ ( crc_u << 1 ) : ( crc_u << 1 );
  return (crc_u);
}

uint8_t get_crc8(uint8_t *Buf, uint8_t BufLen){
  uint8_t crc = 0, i;
  for( i=0; i<BufLen; i++) crc = update_crc8(Buf[i], crc);
  return (crc);
}

void executeDshot() {
  
  TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);


  // Reset variables
  dShotWriteActive = true;     
  
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
  
  dShotWriteActive = false;    
  
}
