/*------------------------------------------------------------------------------
 * Initial Build::
 * File:        lab10_p1.c
 * Description:     Plots acceleration in a 3D space onto a graph. IF the magnitude
 *              of the components reaches two times earth's gravity, a warning
 *              light will go off
 *
 * Input:       3D Vector Components from an Accelorometer
 * Output:      2D Graph/blinking LED
 * Author(s):   Polickoski, Nick
 * Date:        October 22, 2023
 *----------------------------------------------------------------------------*/

// Preprocessor Directives
#include <msp430.h> 
#include <stdio.h>
#include <string.h>
#include <math.h>



// Function Prototypes
void UART_setup(void);
void TimerA_setup(void);
void ADC_setup(void);
void WatchdogTimer_setup(void);
void LED_setup(void);
void Switch_setup(void);
void TimerB_setup(void);

void UART_putCharacter(char c);
void sendData(void);



// Global Variables
volatile long int ADCXval = 0, ADCYval = 0, ADCZval = 0;// position values
volatile float aX = 0, aY = 0, aZ = 0;                  // acceleration values

volatile unsigned char crashFlag = 0;                   // for crashes
volatile double magnitude = 0;                          // for part #2



// Call to Main
void main(void)
{
    WatchdogTimer_setup();
    _EINT();

    LED_setup();
    Switch_setup();

    TimerA_setup();                                     // Setup timer to send ADC data
    TimerB_setup();

    ADC_setup();                                        // Setup ADC
    UART_setup();                                       // Setup UART for RS-232

    while (1)
    {
        ADC12CTL0 |= ADC12SC;                           // Start conversions
        __bis_SR_register(LPM0_bits + GIE);             // Enter LPM0
    }

	return;
}



//// Interrupt Definitions
// ADC
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
    ADCXval = ADC12MEM0;                                // Move results, IFG is cleared
    ADCYval = ADC12MEM1;                                //
    ADCZval = ADC12MEM2;                                //

    __bic_SR_register_on_exit(LPM0_bits);               // Exit LPM0

    return;
}


// Timer A
#pragma vector = TIMERA0_VECTOR
__interrupt void TA0_ISR()
{
    sendData();                                         // Send data to serial app
    __bic_SR_register_on_exit(LPM0_bits);               // Exit LPM0

    return;
}


// Timer B0
#pragma vector = TIMERB0_VECTOR
__interrupt void TB0_ISR()
{
    TB0CCTL0 &= ~CCIFG;                                 // clear interrupt flag

    P2OUT ^= BIT1;                                      // toggle LED on/off

    return;
}


// Watchdog
#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR()
{
    IFG1 &= ~WDTIFG;
    static unsigned char count = 0;


    // If crash flag hasn't been activated -> do nothing
    if (!crashFlag)
    {
        return;
    }


    // If 3 secs pass
    if (count == 2)
    {
        TB0CCTL0 &= ~CCIE;                          // clear Timer B interrupt flag

        P2OUT &= ~BIT1;                             // turn off LED

        count = 0;                                  // reset counter and crash flag for next detection
        crashFlag = 0;

        return;
    }


    // If Switch #2 = pressed
    if (!(P1IN & BIT1))
    {
        unsigned int i;
        for (i = 2000; i > 0; i--);                 // ~20ms debounce

        if (!(P1IN & BIT1))                         // 2nd switch #2 check
        {
            count++;
        }
    }


    // If Switch #2 = not pressed
    if (P1IN & BIT1)
    {
        unsigned int i;
        for (i = 2000; i > 0; i--);                 // ~20ms debounce

        if (P1IN & BIT1)                            // 2nd switch #2 check
        {
            count = 0;
        }
    }

    return;
}



//// Function Definitions
void UART_setup(void)
{
    UCA0CTL1 |= UCSWRST;                                // Set software reset during initialization
    P2SEL |= BIT4 | BIT5;                               // Set UC0TXD and UC0RXD to transmit and receive
    UCA0CTL1 |= UCSSEL_2;                               // Clock source SMCLK

    UCA0BR0 = 9;                                        // 1048576 Hz / 115200 = 9 R 1
    UCA0BR1 = 0;
    UCA0MCTL = 0x02;                                    // Modulation -> 2 * (1)

    UCA0CTL1 &= ~UCSWRST;                               // Clear software reset

    return;
}


void TimerA_setup(void)
{
    TACCR0 = 3277;                                      // 3277 / 32768 Hz = 0.1s
    TACTL = TASSEL_1 + MC_1;                            // ACLK, up mode
    TACCTL0 = CCIE;                                     // Enabled interrupt

    return;
}


void ADC_setup(void)
{
    int i = 0;                                          // iterator for for-loop

    P6DIR &= ~BIT3 + ~BIT7 + ~BIT5;                     // Configure P6.3 and P6.7 as input pins
    P6SEL |= BIT3 + BIT7 + BIT5;                        // Configure P6.3 and P6.7 as analog pins

    ADC12CTL0 = ADC12ON + SHT0_6 + MSC;                 // configure ADC converter
    ADC12CTL1 = SHP + CONSEQ_1;                         // Use sample timer, single sequence

    ADC12MCTL0 = INCH_3;                                // ADC A3 pin - Stick X-axis
    ADC12MCTL1 = INCH_7;                                // ADC A7 pin - Stick Y-axis
    ADC12MCTL2 = INCH_5 + EOS;                          // ADC A5 pin - Stick Z-axis

    // EOS - End of Sequence for Conversions
    ADC12IE |= 0x02;                                    // Enable ADC12IFG.1
    for (i = 0; i < 0x3600; i++);                       // Delay for reference start-up
    ADC12CTL0 |= ENC;                                   // Enable conversions

    return;
}


void WatchdogTimer_setup()
{
    WDTCTL = WDTPW | WDT_ADLY_1000;                     // set WDT to go off every 1 sec

    IE1 |= WDTIE;                                       // enable WDT interrupts
    IFG1 &= ~WDTIFG;                                    // clear interrupt flag

    return;
}


void LED_setup()
{
    P2DIR |= BIT1;                                      // set P2.1 as an output (LED)
    P2OUT &= ~BIT1;                                     // initialize LED = off

    return;
}


void Switch_setup()
{
    P1DIR &= ~BIT1;                                     // set P1.1 as an input (switch)
    P1OUT |= BIT1;                                      // proper I/O shit

    return;
}


void TimerB_setup()
{
    TB0CCR0 = 32768 / 4;                                // must be a 4th of frequency to blink LED
    TBCTL = TASSEL_1 + MC_1;                            // ACLK, up mode
    TB0CCTL0 &= ~CCIFG;                                 // clear interrupt flag

    return;
}


void UART_putCharacter(char c)
{
    while (!(IFG2 & UCA0TXIFG));                        // USCI_A0 TX buffer ready?
    UCA0TXBUF = c;                                      // Put character into tx buffer

    return;
}


void sendData(void)
{
    int i;                                              // iterator for all for-loops

    aX = (ADCXval * 3.0/4095 - 1.5) / 0.3;              // Calculate percentage outputs
    aY = (ADCYval * 3.0/4095 - 1.5) / 0.3;              //
    aZ = (ADCZval * 3.0/4095 - 1.5) / 0.3;              //


    // Error Handeling for Airbag Deployment
    magnitude = sqrt(aX*aX + aY*aY + aZ*aZ);            // finding magnitude
    if (magnitude >= 2)                                 // critical point
    {
        crashFlag = 1;                                  // flag set
        TB0CCTL0 |= CCIE;                               // enable interrupt
    }

    // Use character pointers to send one byte at a time
    char *Xptr = (char *)&aX;
    char *Yptr = (char *)&aY;
    char *Zptr = (char *)&aZ;

    UART_putCharacter(0x55);                            // Send header

    for(i = 0; i < 4; i++)                              // Send x percentage - one byte at a time
    {
        UART_putCharacter(Xptr[i]);
    }

    for(i = 0; i < 4; i++)                              // Send y percentage - one byte at a time
    {
        UART_putCharacter(Yptr[i]);
    }

    for(i = 0; i < 4; i++)                              // Send y percentage - one byte at a time
    {
        UART_putCharacter(Zptr[i]);
    }

    return;
}

