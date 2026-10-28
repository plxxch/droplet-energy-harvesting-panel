/*
 * main_alarm.c
 *
 * Receiver for the 3-byte packet (temperature + smoke alarm).
 * Outputs "Data:<smoke/alarm>,<temperature>" over UART for VOFA+.
 *
 * Copyright (c) 2022 Pengyu Li
 * SPDX-License-Identifier: MIT
 */


#include "smoke-receiver.h"
#include "MSP430.h"
#include "CC430F5137.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>





#define  PACKET_LEN         (5)            // PACKET_LEN <= 61
#define  RSSI_IDX           (PACKET_LEN)    // Index of appended RSSI 
#define  CRC_LQI_IDX        (PACKET_LEN+1)  // Index of appended LQI, checksum
#define  CRC_OK             (BIT7)          // CRC_OK bit 
#define  PATABLE_VAL        (0x51)          // 0 dBm output 



unsigned char string[30];
char data[35];

unsigned char node=0;

//void TransmitUartData(unsigned char nodeIndex );
unsigned char nodetime=0;
unsigned char packetReceived;
unsigned char packetTransmit; 

unsigned char RxBuffer[PACKET_LEN+2];
unsigned char RxBufferLength = 0;
const unsigned char TxBuffer[PACKET_LEN];

unsigned char Smokedata[4]={0,0,0,0};
unsigned char Tempdata[2]={0,0};
unsigned char buttonPressed = 0;
unsigned int i = 0; 

char ready[]={"Ready"};
void Uart_ready(unsigned char* string2, unsigned int length);

unsigned char transmitting = 0; 
unsigned char receiving = 0; 
unsigned int a,b,c;

unsigned char recieved=0;
void SetupRtc(void);
void UpdateNodeInfoOnLcd(void);
void Uart_init(void);

char Tx_empty[21];
void Uart_Tx(char* string, unsigned int length);
char Tx_data[24];
unsigned char txtype=0;

unsigned char rx_data=0;

long temp=0;
long smoke=0;
float Uartdata=0;
float IntDegC=0;



void main()
{  
  // Stop watchdog timer to prevent time out reset 
  WDTCTL = WDTPW + WDTHOLD; 

  SetVCore(3);

  UCSCTL3 |= SELREF_2;                      // Set DCO FLL reference = REFO
               __bis_SR_register(SCG0);                  // Disable the FLL control loop
              // UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
               UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
               UCSCTL2 = FLLD_1 + 599;                   // Set DCO Multiplier for 8MHz
                                                         // (N + 1) * FLLRef = Fdco
               __bic_SR_register(SCG0);
//SetupRtc();
  Uart_init();

   ResetRadioCore();
  InitRadio();
   InitButtonLeds();
__bis_SR_register(GIE);

//sprintf(Tx_empty,"Empty:%.2f,%.2f,%.2f\n",0.0,0.0,0.0);
////Tx_empty[]={"Data:0,0,0\n"};



   //P1DIR =0xff;
//   RTCCTL01 = RTCTEVIE + RTCSSEL_2 + RTCTEV_0; // Counter Mode, RTC1PS, 8-bit ovf                                             // overflow interrupt enable
//   RTCPS0CTL = RT0PSDIV_2;                   // ACLK, /8, start timer
//   RTCPS1CTL = RT1SSEL_2 + RT1PSDIV_3;


  ReceiveOn();
  receiving = 1;
  Uart_Tx(ready, sizeof ready);
  while (1)
  {
      ReceiveOn();
      receiving = 1;
      //__bic_SR_register(LPM3_bits+GIE);
    }
}



void InitButtonLeds(void)
{
  // Set up the button as interruptible 
  P2DIR &= ~BIT1;
  P2REN |= BIT1;
  P2IES &= BIT1;
  P2IFG = 0;
  P2OUT |= BIT1;
  P2IE  |= BIT1;

  // Initialize Port J
  P1OUT = 0x00;
  P1DIR = 0xFF;

  // Set up LEDs 
  P1OUT &= ~BIT0;
  P1DIR |= BIT0;

  P1OUT &= ~BIT2;
  P1DIR |= BIT2;
  P1DIR |= BIT3;
 // P1DIR=0xFF;
}

void InitRadio(void)
{
  // Set the High-Power Mode Request Enable bit so LPM3 can be entered
  // with active radio enabled 
  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMHPMRE_L; 
  PMMCTL0_H = 0x00; 
  
  WriteRfSettings();
  
  WriteSinglePATable(PATABLE_VAL);
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{

      P2IE = 0;                             // Debounce by disabling buttons
      buttonPressed = 1;
     // P1OUT ^= 0x02;
     // TransmitUartData(2);
      __bic_SR_register_on_exit(LPM3_bits); // Exit active    

}

void Transmit(unsigned char *buffer, unsigned char length)
{
  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // Clear pending interrupts
  RF1AIE |= BIT9;                           // Enable TX end-of-packet interrupt
  
  WriteBurstReg(RF_TXFIFOWR, buffer, length);     
  Strobe( RF_STX );                         // Strobe STX   
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
        if(receiving)             // RX end of packet
              {
                // Read the length byte from the FIFO
                RxBufferLength = ReadSingleReg( RXBYTES );
                ReadBurstReg(RF_RXFIFORD, RxBuffer, RxBufferLength);

        Tempdata[0]=RxBuffer[0];
        Tempdata[1]=RxBuffer[1];
        Uartdata = (float) RxBuffer[2];
        temp=(Tempdata[0]<<8) | Tempdata[1];
        IntDegC = 10*(temp*0.0233735871 + 25.0000038)-724.5-15;
        sprintf(Tx_data,"Data:%.2f,%.2f\n",Uartdata,IntDegC);
        Uart_Tx(Tx_data, sizeof Tx_data);







        P1OUT ^= BIT0;

              ReceiveOff();


              }
              else if(transmitting)         // TX end of packet
              {
                RF1AIE &= ~BIT9;                    // Disable TX end-of-packet interrupt
                P1OUT &= ~BIT0;                     // Turn off LED after Transmit
                transmitting = 0;
              }
              else while(1);                // trap
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






void Uart_init(void)
{              // Stop WDT

  PMAPPWD = 0x02D52;                        // Get write-access to port mapping regs
  P1MAP5 = PM_UCA0RXD;                      // Map UCA0RXD output to P1.5
  P1MAP6 = PM_UCA0TXD;                      // Map UCA0TXD output to P1.6
  PMAPPWD = 0;                              // Lock port mapping registers

  P1DIR |= BIT6;                            // Set P1.6 as TX output
  P1SEL |= BIT5 + BIT6;                     // Select P1.5 & P1.6 to UART function

  UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
  UCA0CTL1 |= UCSSEL_1;                     // CLK = ACLK
  UCA0BR0 = 0x03;                           // 32kHz/9600=3.41 (see User's Guide)
  UCA0BR1 = 0x00;                           //
  UCA0MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}

void Uart_Tx(char* string, unsigned int length)
{
   unsigned int i;
for(i=0;i<length;i++)
   {
   UCA0TXBUF=string[i];
   while (!(UCA0IFG & UCTXIFG));
   }
}



#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
  switch(__even_in_range(UCA0IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
    //while (!(UCA0IFG&UCTXIFG));             // USCI_A0 TX buffer ready?
                      // TX -> RXed character
    break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;
  }
}



#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)

{
  switch(__even_in_range(RTCIV,16))
  {
    case 0: break;                          // No interrupts
    case 2: break;                          // RTCRDYIFG
    case 4:                                 // RTCEVIFG


      break;
    case 6: break;                          // RTCAIFG
    case 8: break;                          // RT0PSIFG
    case 10: break;                         // RT1PSIFG
    case 12: break;                         // Reserved
    case 14: break;                         // Reserved
    case 16: break;                         // Reserved
    default: break;
  }
}


