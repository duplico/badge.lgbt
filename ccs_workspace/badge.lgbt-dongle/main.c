#include <msp430fr2433.h>

/// Initialize clock signals and the three system clocks.
/**
 ** We'll take the DCO to 16 MHz, and divide it by 1 for MCLK.
 ** Then we'll divide MCLK by 2 to get 8 MHz SMCLK.
 **
 ** Our available clock sources are:
 **  VLO:     10kHz very low power low-freq
 **  REFO:    32.768kHz (typ) reference oscillator
 **  DCO:     Digitally controlled oscillator (1MHz default)
 **           Specifically, 1048576 Hz typical.
 **
 ** At startup, our clocks are as follows:
 **  MCLK:  Sourced by the DCO
 **         (Available: DCO, REFO, VLO)
 **  SMCLK: Sourced from MCLK, with no divider
 **         (Available dividers: {1,2,4,8})
 **  ACLK: Sourced from REFO
 **         (the only available internal source)
 */
void init_clocks() {
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    __bis_SR_register(SCG0);    // disable FLL
    CSCTL3 |= SELREF__REFOCLK;  // Set REFO as FLL reference source
    CSCTL0 = 0;                 // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);     // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_5;        // Set DCO = 16MHz
    CSCTL2 = FLLD_0 + 487;      // set to fDCOCLKDIV = (FLLN + 1)*(fFLLREFCLK/n)
                                //                   = (487 + 1)*(32.768 kHz/1)
                                //                   = 16 MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                        // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));      // FLL locked

    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK; // Select default clock sources.

    // SYSTEM CLOCKS
    // =============

    // MCLK
    //  All sources but MODOSC are available at up to /128
    //  Set to DCO/1 = 16 MHz
    CSCTL5 |= DIVM__1;

    // SMCLK
    //  Derived from MCLK with divider up to /8
    //  Set to MCLK/2 = 8 MHz
    CSCTL5 |= DIVS__2;
}

/// Apply the initial configuration of the GPIO and peripheral pins.
/**
 **
 */
void init_io() {
    // Unlock the pins from high-impedance mode:
    // (AKA the MSP430FR magic make-it-work command)
    PM5CTL0 &= ~LOCKLPM5;

    // TODO: Timer/PWM on P1.1

    // GPIO:
    // P1.0 (SEL 00; DIR 1) LED
    // P1.1 (SEL 00; DIR 1) LED
    // P1.2 (SEL 10; DIR 1) TA0.2 out
    // P1.3 (SEL 00; DIR 1) unused
    // P1.4 (SEL 01; DIR 1) UCA0 TXD
    // P1.5 (SEL 01; DIR 0) UCA0 RXD
    // P1.6 (SEL 00; DIR 1) unused
    // P1.7 (SEL 00; DIR 1) unused
    P1DIR =  0b11011111;
    P1SEL0 = 0b00110000;
    P1SEL1 = 0b00000100;
    P1OUT =  0b00000000;

    // P2.0 (SEL 01; DIR 1) XOUT
    // P2.1 (SEL 01; DIR 0) XIN
    // P2.2 (SEL 00; DIR 1 OUT 1) nRST; GPIO OUT; HIGH
    // P2.3 (SEL 00; DIR 1) unused
    // P2.4 (SEL 00; DIR 1) unused
    // P2.5 (SEL 01; DIR 0) UCA1 RXD
    // P2.6 (SEL 01; DIR 1) UCA1 TXD
    // P2.7 (SEL 00; DIR 1) unused
    P2DIR =  0b11011101;
    P2SEL0 = 0b01100011;
    P2SEL1 = 0b00000000;
    P2OUT =  0b00000100;

    // P3.0 (SEL 00; DIR 1) unused
    // P3.1 (SEL 00; DIR 1) unused
    // P3.2 (SEL 00; DIR 1; OUT 0) SD; GPIO OUT; LOW
    P3DIR = 0b00000111;
    P3SEL0 = 0x00;
    P3SEL1 = 0x00;
    P3OUT = 0x00;
}

void init_serial() {
    // First, we need to set up our 16XCLK. 2% clock error is acceptable.
    // The period should be SMCLK/BAUD_RATE
    TA0CCR0 = 26;                             // PWM Period
    TA0CCTL2 = OUTMOD_7;                      // CCR1 reset/set
    TA0CCR2 = 26;                             // CCR1 PWM duty cycle 50%
    TA0CTL = TASSEL__SMCLK | MC__UP | TACLR;  // SMCLK, up mode, clear TAR

    // UCA0: USB /////////////////////////////////////////////////////////////

    UCA0CTLW0 |= UCSSEL__SMCLK;               // CLK = SMCLK
    // Pause the UART peripheral:    UCA0CTLW0 |= UCSWRST;
    // Source the baud rate generation from SMCLK (8 MHz)
    // 8N1 (8 data bits, no parity bits, 1 stop bit)
    // Configure the baud rate
    //  (See page 589 in the family user's guide, SLAU445I)
    // The below is for 8.00 MHz SMCLK:
    UCA0BRW = 26;
    UCA0MCTLW |= UCOS16 | UCBRF_0 | 0xB600;

    // UCA1: IR  //////////////////////////////////////////////////////////////

    UCA1CTLW0 |= UCSSEL__SMCLK;
    // Pause the UART peripheral:
    UCA1CTLW0 |= UCSWRST;
    // Source the baud rate generation from SMCLK (8 MHz)
    // 8N1 (8 data bits, no parity bits, 1 stop bit)
    // Configure the baud rate
    //  (See page 589 in the family user's guide, SLAU445I)
    // The below is for 8.00 MHz SMCLK:
    UCA1BRW = 26;
    UCA1MCTLW = 0xB600 | UCOS16 | UCBRF_0;


    // Activate the UARTs:
    UCA0CTLW0 &= ~UCSWRST;
    UCA1CTLW0 &= ~UCSWRST;

    // The TX interrupt flag (UCTXIFG) gets set upon enabling the UART.
    //  But, we'd prefer that interrupt not to fire, so we'll clear it
    //  now:
    UCA0IFG &= ~UCTXIFG;
    UCA1IFG &= ~UCTXIFG;

    // Enable interrupts for TX and RX:
    UCA0IE |= UCRXIE + UCTXIE;
    UCA1IE |= UCRXIE + UCTXIE;
}

/// Perform basic initialization of the cbadge.
void init() {
    // Stop the watchdog timer.
    WDTCTL = WDTPW | WDTCNTCL;

    init_clocks();
    init_io();
    init_serial();

    __bis_SR_register(GIE);
}

int main(void)
{
    init();

    while (1) {
            __bis_SR_register(LPM3_bits);
    }
}

#pragma vector=USCI_A0_VECTOR
__interrupt void serial_usb_isr() {
    switch(__even_in_range(UCA0IV, USCI_UART_UCTXIFG)) {
    case USCI_UART_UCRXIFG:
        // Receive buffer full; a byte is ready to read in UCA0RXBUF
        UCA1TXBUF = UCA0RXBUF;
        P1OUT |= BIT1;
        break;
    case USCI_UART_UCTXIFG:
        // Transmit buffer empty, ready to load another byte to send to UCA0TXBUF.
        P1OUT &= ~BIT0;
        break;
    }
}

#pragma vector=USCI_A1_VECTOR
__interrupt void serial_ir_isr() {
    switch(__even_in_range(UCA1IV, USCI_UART_UCTXIFG)) {
    case USCI_UART_UCRXIFG:
        // Receive buffer full; a byte is ready to read in UCA0RXBUF
        UCA0TXBUF = UCA1RXBUF;
        P1OUT |= BIT0;
        break;
    case USCI_UART_UCTXIFG:
        // Transmit buffer empty, ready to load another byte to send to UCA0TXBUF.
        P1OUT &= ~BIT1;
        break;
    }
}
