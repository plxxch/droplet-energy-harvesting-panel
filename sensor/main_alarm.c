/******************************************************************************

 @file  main_alarm.c

 @brief Smoke/temperature sensor node, 3-byte variant: temperature + smoke alarm. Reads the MAX30105
        optical smoke sensor over I2C and the internal temperature sensor on
        ADC12 channel A10, then transmits the result over the CC430F5137
        sub-1 GHz radio. Based on example code from Texas Instruments.

 @author Pengyu Li (STVL, City University of Hong Kong)

 Copyright (c) 2022 Pengyu Li
 SPDX-License-Identifier: MIT

 Target Device: CC430F5137

 ******************************************************************************/

#include "smoke-sensor.h"

#include "CC430F5137.h"


unsigned char RXData[4]= {0,0,0,0};        // Pointer to TX data

unsigned char TXByteCtr;
unsigned char RXByteCtr;
unsigned char Read=0;


unsigned char TXData[20];

unsigned char awake=1;
unsigned char rx_index=0;
unsigned char tx_index=0;


//unsigned char TxBuffer[7]={0x0,0x0,0x0,0x0,0x0,0x02,0x03};

unsigned char TxBuffer[3]={0x0,0x0,0x0};
long smoke=0;
long temp=0;
void Trans_Data(void);


void maincode(void);

void timer(unsigned int t);
void write(unsigned char writeaddr,unsigned char command);
void read(unsigned char readaddr,unsigned char Bytetoread);


void main( void )
{


    write(0x09,0x80);
    InitRadio();            //Initialize the radio core



    REFCTL0 |= REFMSTR + REFVSEL_0 + REFON;    // Enable internal 1.5V reference

            /* Initialize ADC12_A */
            ADC12CTL0 = ADC12SHT0_8 + ADC12ON;        // Set sample time
            ADC12CTL1 = ADC12SHP;                     // Enable sample timer
            ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // ADC input ch A10 => temp sense
            ADC12IE = 0x001;                          // ADC_IFG upon conv result-ADCMEMO

            __delay_cycles(75);                       // 35us delay to allow Ref to settle
            ADC12CTL0 |= ADC12ENC;

       __bis_SR_register(GIE); //Enable interrup



      // write(0xFF,0x0);

       write(0x09,0x02);  //red only 0x02,red+ir 0x03, multiled 0x07

       write(0x0C,0x02);  ////pulse amplitude //red proximity independent   //Default is 0x1F which gets us 6.4mA
       ////powerLevel = 0x02, 0.4mA - Presence detection of ~4 inch
       ////powerLevel = 0x1F, 6.4mA - Presence detection of ~8 inch
       ////powerLevel = 0x7F, 25.4mA - Presence detection of ~8 inch
       ////powerLevel = 0xFF, 50.0mA - Presence detection of ~12 inch

while(1)
{
    if(awake==1)        //If awake from low power mode
    {
         maincode();    //Execute measurement and transmission

         awake=0;

    }
         timer(30000);

    //__bis_SR_register(LPM3_bits + GIE);

}
}




void InitRadio(void)
{
  // Set the High-Power Mode Request Enable bit so LPM0 can be entered
  // with active radio enabled 
  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMHPMRE_L; 
  PMMCTL0_H = 0x00; 
  
  WriteRfSettings();
  
  WriteSinglePATable(0xC0);  //50-10dbm  26--12dbm
}


void Transmit(unsigned char *buffer, unsigned char length)
{

  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // Clear pending interrupts
  RF1AIE |= BIT9;                           // Enable TX end-of-packet interrupt
  
  WriteBurstReg(RF_TXFIFOWR, buffer, length);     
  
  Strobe( RF_STX );                         // Strobe STX
 // Strobe( RF_STX );
}

void ReceiveOn(void)
{  
  RF1AIES |= BIT9;                          // Falling edge of RFIFG9
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt 
  
  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  Strobe( RF_SRX );                      
}

void ReceiveOff(void)
{
  RF1AIE &= ~BIT9;                          // Disable RX interrupts
  RF1AIFG &= ~BIT9;                         // Clear pending IFG

  // It is possible that ReceiveOff is called while radio is receiving a packet.
  // Therefore, it is necessary to flush the RX FIFO after issuing IDLE strobe 
  // such that the RXFIFO is empty prior to receiving a packet.
  Strobe( RF_SIDLE );
  Strobe( RF_SFRX  );                       
}

#pragma vector=CC1101_VECTOR
__interrupt void CC1101_ISR(void)
{
  switch(__even_in_range(RF1AIV,32))        // Prioritizing Radio Core Interrupt 
  {
    case  0: break;                         // No RF core interrupt pending
    case  2: break;                         // RFIFG0
    case  4: break;                         // RFIFG1
    case  6: break;                         // RFIFG2
    case  8: break;                         // RFIFG3
    case 10: break;                         // RFIFG4
    case 12: break;                         // RFIFG5
    case 14: break;                         // RFIFG6
    case 16: break;                         // RFIFG7
    case 18: break;                         // RFIFG8
    case 20:                                // RFIFG9
               RF1AIE &= ~BIT9;                    // Disable TX end-of-packet interrupt

             break;
    case 22: break;                         // RFIFG10
    case 24: break;                         // RFIFG11
    case 26: break;                         // RFIFG12
    case 28: break;                         // RFIFG13
    case 30: break;                         // RFIFG14
    case 32: break;                         // RFIFG15
  }  
  __bic_SR_register_on_exit(LPM3_bits);
}






#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void)

{
    awake=1;
    TA0CTL=MC_0;
   __bic_SR_register_on_exit(LPM3_bits);

}


void timer(unsigned int t)    //For delay
{
    TA0CTL=MC_0;
    TA0CCTL0=CCIE;
    TA0CCR0=t;
    TA0CTL=TASSEL_1 +MC_1 + TACLR;
    __bis_SR_register(LPM3_bits + GIE);
}




void maincode()
{
    ReceiveOff();

read(0x07,3);


P2OUT &= ~BIT0;

ADC12CTL0 |= ADC12SC;                   // Sampling and conversion start

   __bis_SR_register(LPM3_bits + GIE);

Trans_Data();



Transmit(TxBuffer, sizeof TxBuffer);  //Transmit data
__bis_SR_register(LPM3_bits + GIE);

P2OUT |= BIT0;

timer(500);
write(0x06,0x0); // clear
timer(300);
write(0x04,0x0);

}






#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCI_B0_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(UCB0IV,12))
  {
  case  0: break;                           // Vector  0: No interrupts
  case  2: break;                           // Vector  2: ALIFG
  case  4: break;                           // Vector  4: NACKIFG
  case  6:                                  // Vector  6: STTIFG
    UCB0IFG &= ~UCSTTIFG;
    break;
  case  12:                                  // Vector  8: STPIFG
      switch(Read)
                                       {
                                         case 1:         //Read

                                     if (TXByteCtr)                                // Check TX byte counter
                                        {

                                         TXByteCtr--;

                                         UCB0TXBUF =TXData[TXByteCtr] ;            // Load TX buffer

                                                                      // Decrement TX byte counter
                                        }
                                     else
                                        {
                                       //  P1OUT ^= BIT0;
                                         rx_index=0;
                                         UCB0IE |= UCRXIE;              // Enable RX interrupt
                                         UCB0IE &= ~UCTXIE;             // Disable TX interrupt
                                         UCB0CTLW0 &= ~UCTR;            // Switch to receiver

                                         UCB0CTLW0 |= UCTXSTT;          // Send repeated start
                                         if (RXByteCtr == 1)
                                         {
                                                           //Must send stop since this is the N-1 byte
                                          while((UCB0CTLW0 & UCTXSTT));
                                          UCB0CTLW0 |= UCTXSTP;      // Send stop condition
                                                       }
                                        }
                                     break;

                                         case 0:         //write
                                             if (TXByteCtr)                                // Check TX byte counter
                                                      {
                                                        TXByteCtr--;

                                                       UCB0TXBUF = TXData[tx_index];
                                                       tx_index++;
                                                      }
                                                   else
                                                      {
                                                    //   P1OUT ^= BIT0;
                                                        UCB0CTLW0 |= UCTXSTP;      // Send stop condition
                                                        UCB0IE &= ~UCTXIE;                       // disable TX interrupt

                                                        __bic_SR_register_on_exit(LPM3_bits);      // Exit LPM0

     //
                                       }
                                         default:
                                                      __no_operation();
                                                      break;
                                       }

               break;
  case 10:                                  // Vector 10: RXIFG
      if (RXByteCtr)
                  {
                      RXByteCtr--;
                      RXData[RXByteCtr] = UCB0RXBUF;
                      rx_index++;
                  }

                  if (RXByteCtr == 1)
                  {
                    UCB0CTLW0 |= UCTXSTP;
                  }
                  else if (RXByteCtr == 0)
                  {
                    UCB0IE &= ~UCRXIE;
                 //   P1OUT ^= BIT1;
                    __bic_SR_register_on_exit(LPM3_bits);      // Exit LPM0
                  }



                  break;

  default: break;
  }
}
















void Trans_Data(void)
{

//    TxBuffer[0] = RXData[0];
//    TxBuffer[1] = RXData[1];
//    TxBuffer[2] = RXData[2];

    smoke = *(long *)RXData;
    if(smoke>6000)
    {
        TxBuffer[2]=0x01;
    }
    else
    {
        TxBuffer[2]=0x0;
    }
    TxBuffer[0] = (unsigned char) (temp >> 8) & 0xFF;
    TxBuffer[1] = (unsigned char) temp & 0xFF;

}


#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  6:                                  // Vector  6:  ADC12IFG0
    temp = ADC12MEM0;                       // Move results, IFG is cleared
    __bic_SR_register_on_exit(LPM3_bits);   // Exit active CPU
   // timer_start();
    break;
  default: break;
  }
}


//fifo average
//
//led mode
//
//adc range
//
//sample rate
//
//
//pulsewidth  69
//
//pulse amplitude //red proximity independent   //Default is 0x1F which gets us 6.4mA
////powerLevel = 0x02, 0.4mA - Presence detection of ~4 inch
////powerLevel = 0x1F, 6.4mA - Presence detection of ~8 inch
////powerLevel = 0x7F, 25.4mA - Presence detection of ~8 inch
////powerLevel = 0xFF, 50.0mA - Presence detection of ~12 inch
//
//enable led
//



void write(unsigned char writeaddr,unsigned char command)
{

    TXData[0]=writeaddr;
    TXData[1]=command;


                                                 TXByteCtr =2;
                                                 RXByteCtr=0;
                                                 tx_index=0;

                                                 Read=0;
                                                 UCB0I2CSA = 0x57;   //0x68, 0x6A, 0x6C, 0x6E
                                                 UCB0IFG &= ~(UCTXIFG + UCRXIFG);       // Clear any pending interrupts
                                                 UCB0IE &= ~UCRXIE;                       // Disable RX interrupt
                                                 UCB0IE |= UCTXIE;                        // Enable TX interrupt


                                                 UCB0CTLW0 |= UCTR + UCTXSTT;             // I2C TX, start condition

                                              __bis_SR_register(LPM3_bits + GIE);

}

void read(unsigned char readaddr,unsigned char Bytetoread)
{
                                                             TXData[0]=readaddr;

                                                             TXByteCtr =1;
                                                             RXByteCtr=Bytetoread;
                                                             tx_index=0;

                                                     Read=1;
                                                     UCB0I2CSA = 0x57;   //0x68, 0x6A, 0x6C, 0x6E
                                                     UCB0IFG &= ~(UCTXIFG + UCRXIFG);       // Clear any pending interrupts
                                                     UCB0IE &= ~UCRXIE;                       // Disable RX interrupt
                                                     UCB0IE |= UCTXIE;                        // Enable TX interrupt


                                                     UCB0CTLW0 |= UCTR + UCTXSTT;             // I2C TX, start condition

                                                  __bis_SR_register(LPM3_bits + GIE);
}



