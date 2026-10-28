/*****************************************************************************/
/* _SYSTEM_PRE_INIT.C          - Perform any application-specific inits      */
/*                                                                           */
/* Copyright (c) 2015 Texas Instruments Incorporated                         */
/* http://www.ti.com/                                                        */
/*                                                                           */
/*  Redistribution and  use in source  and binary forms, with  or without    */
/*  modification,  are permitted provided  that the  following conditions    */
/*  are met:                                                                 */
/*                                                                           */
/*     Redistributions  of source  code must  retain the  above copyright    */
/*     notice, this list of conditions and the following disclaimer.         */
/*                                                                           */
/*     Redistributions in binary form  must reproduce the above copyright    */
/*     notice, this  list of conditions  and the following  disclaimer in    */
/*     the  documentation  and/or   other  materials  provided  with  the    */
/*     distribution.                                                         */
/*                                                                           */
/*     Neither the  name of Texas Instruments Incorporated  nor the names    */
/*     of its  contributors may  be used to  endorse or  promote products    */
/*     derived  from   this  software  without   specific  prior  written    */
/*     permission.                                                           */
/*                                                                           */
/*  THIS SOFTWARE  IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS    */
/*  "AS IS"  AND ANY  EXPRESS OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT    */
/*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR    */
/*  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT    */
/*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    */
/*  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT  NOT    */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,    */
/*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY    */
/*  THEORY OF  LIABILITY, WHETHER IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE    */
/*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.     */
/*                                                                           */
/*****************************************************************************/

/*****************************************************************************/
/* _SYSTEM_PRE_INIT() - _system_pre_init() is called in the C/C++ startup    */
/* routine (_c_int00()) and provides a mechanism for the user to             */
/* insert application specific low level initialization instructions prior   */
/* to calling main().  The return value of _system_pre_init() is used to     */
/* determine whether or not C/C++ global data initialization will be         */
/* performed (return value of 0 to bypass C/C++ auto-initialization).        */
/*                                                                           */
/* PLEASE NOTE THAT BYPASSING THE C/C++ AUTO-INITIALIZATION ROUTINE MAY      */
/* RESULT IN PROGRAM FAILURE.                                                */
/*                                                                           */
/* The version of _system_pre_init() below is skeletal and is provided to    */
/* illustrate the interface and provide default behavior.  To replace this   */
/* version rewrite the routine and include it as part of the current project.*/
/* The linker will include the updated version if it is linked in prior to   */
/* linking with the C/C++ runtime library.                                   */
/*****************************************************************************/
#include<CC430F5137.h>
extern "C" int _system_pre_init(void)
{
    WDTCTL = WDTPW + WDTHOLD;

    P3DIR=0XFF;
    P3OUT=0X0;
        P1DIR=0XFF;
        P1OUT=0X00;
        P2DIR=0XFF;
        P2OUT=0X0;
        P5DIR=0XFF;
        P5OUT=0X0;
        PJOUT = 0x00;
        PJDIR = 0xFF;

        P2DIR =0X01;
        P2OUT |= BIT0;

        UCSCTL3 |= SELREF_2;                      // Set DCO FLL reference = REFO
             __bis_SR_register(SCG0);                  // Disable the FLL control loop
            // UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
             UCSCTL1 = DCORSEL_5;                      // Select DCO range 16MHz operation
             UCSCTL2 = FLLD_1 + 250;                   // Set DCO Multiplier for 8MHz
                                                       // (N + 1) * FLLRef = Fdco
             __bic_SR_register(SCG0);





//
             PMAPPWD = 0x02D52;                        // Get write-access to port mapping regs
               P1MAP3 = PM_UCB0SDA;                      // Map UCB0SDA output to P1.3
               P1MAP2 = PM_UCB0SCL;                      // Map UCB0SCL output to P1.2
               PMAPPWD = 0;                              // Lock port mapping registers

               P1SEL |= BIT2 + BIT3;                     // Select P1.2 & P1.3 to I2C function

//
//
//
//
//
              UCB0CTL1 |= UCSWRST;                      // Enable SW reset
              UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
              UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK
              UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
              UCB0BR1 = 0;
              UCB0CTL1 &=~ UCSWRST;                            // clear reset register
              UCB0IE |= UCNACKIE;                     // transmit and NACK interrupt enable






//              UCB0CTL1 |= UCSWRST;                      // Enable SW reset
//              UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
//              UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
//              UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
//              UCB0BR1 = 0;
//              UCB0I2CSA = 0x5E;                         // Slave Address is 048h
//              UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
//              UCB0IE |= UCTXIE;                         // Enable TX interrupt
//





    return 1;
}
