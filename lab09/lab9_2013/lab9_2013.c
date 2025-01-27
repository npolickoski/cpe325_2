/*------------------------------------------------------------------------------
 * Initial Build::
 * File:        lab9_2013.c
 * Description:     changes the frequency of a blinking light depending on values
 *              values sent into it via the SPI protcoll
 *
 * Input:       numeric values for the duty cycle
 * Output:      - numeric values for the blink rate to the 4618
 *              - change in blink rate of LED
 * Author(s):   Polickoski, Nick
 * Date:        October 12, 2023
 *
 * lacasauah.edu <- Dr. Mel website
 *----------------------------------------------------------------------------*/

//// Preprocessor Directives
// Libraries
#include <msp430.h>
#include <stdio.h>


// Macros & Functions
#define SET_BUSY_FLAG() P1OUT |= 0x10
#define RESET_BUSY_FLAG() P1OUT &= ~0x10

#define SET_LED() P1OUT |= 0x01
#define RESET_LED() P1OUT &= ~0x01

unsigned char blinkNumber = 0;                      // total number of blinks (caps at 127)
unsigned char DutyValue;                            // change in duty cycle value that gets set over




// Function Prototypes
void SPI_setup();
void SPI_initComm();
void initLED();
void initSystem();
void initTimerA();



// Call To Main
void main(void)
{
    // Initialize States
    initSystem();
    initLED();                                      // LED state initialization
    SPI_setup();                                    // Setup USI module in SPI mode
    SPI_initComm();                                 // Initialization communication
    initTimerA();                                   // timer A initialization


    // Conditions Check
    for (;;)
    {
        _BIS_SR(LPM0_bits + GIE);                  // Enter LPM0 with interrupt

        switch (DutyValue)
        {
            case 200:                               // "-" condition
                blinkNumber = 0;
                break;

            case 255:                               // "?" condition
                USISRL = blinkNumber;
                break;

            case 100:                               // turn on LED blinking condition
            case 0:
                TA0CCTL0 &= ~CCIE;                  // unplug computer
                TA0CCTL1 &= ~CCIE;                  // unplug computer 2: electric boogaloo
                SET_LED();                          // turn on eternally
                break;

//            case 0:                                 // turn off LED blinking condition
//                TA0CCTL1 |= CCIE;                   // CCR0 triggers interrupt
//                TA0CCTL0 &= ~CCIE;                  // unplug computer
//                break;

            default:
                TA0CCR1 = (32768 * DutyValue) / 100;// set new duty cycle

                TA0CCTL0 |= CCIE;                   // CCR0 triggers interrupt
                TA0CCTL1 |= CCIE;                   // CCR0 triggers interrupt
                break;
        }

        USISRL = blinkNumber;                       // proper updating so no value is skipped

        RESET_BUSY_FLAG();                          // Clears busy flag - ready for new communication
    }

    return;
}



//// Interrupt Definitions
// User Input
#pragma vector = USI_VECTOR
__interrupt void USI_ISR(void)
{
    SET_BUSY_FLAG();                                // Set busy flag - busy with new communication

    DutyValue = USISRL;                             // Read new command
    USICNT = 8;                                     // Load bit counter for next TX

    _BIC_SR_IRQ(LPM0_bits);                         // Exit from LPM0 on RETI

    return;
}


// Timer A0
#pragma vector = TIMERA0_VECTOR
__interrupt void Timer_A0()
{
    TA0CCTL0 &= ~CCIFG;                             // Clear the flag
    SET_LED();                                      // turn on LED1

    if (blinkNumber < 127)
    {
        blinkNumber++;
    }

    USISRL = blinkNumber;                           // proper updating so no value is skipped

    return;
}


// Timer A1
#pragma vector = TIMERA1_VECTOR
__interrupt void Timer_A1()
{
    TA0CCTL1 &= ~CCIFG;                             // Clear the flag
    RESET_LED();                                     // turn off LED1

    return;
}



// Function Definitions
void SPI_setup()
{
    USICTL0 |= USISWRST;                            // Set UCSWRST -- needed for re-configuration process
    USICTL0 |= USIPE5 + USIPE6 + USIPE7 + USIOE;    // SCLK-SDO-SDI port enable,MSB first

    USICTL1 = USIIE;                                // USI Counter Interrupt enable
    USICTL0 &= ~USISWRST;                           // **Initialize USCI state machine**

    return;
}


void SPI_initComm()
{
    USICNT = 8;                                     // Load bit counter, clears IFG
    USISRL = DutyValue;                             // Set duty val state

    RESET_BUSY_FLAG();                              // Reset busy flag

    return;
}


void initLED()
{
    P1DIR |= BIT0;                                  // P1.0 as output - LED3
    P1DIR |= BIT4;                                  // P1.4 as output - Busy flag

    SET_LED();                                      // initalize LED = on

    return;
}


void initSystem()
{
    WDTCTL = WDTPW + WDTHOLD;                       // Stop watchdog timer

    BCSCTL1 = CALBC1_1MHZ;                          // Set DCO
    DCOCTL = CALDCO_1MHZ;

    return;
}


void initTimerA()
{
    TA0CTL |= TASSEL_2 | MC_1 | ID_3;               // SMCLK is clock source; UP mode; divide by 8

    TA0CCTL0 |= CCIE;                               // CCR0 triggers interrupt
    TA0CCTL1 |= CCIE;                               // CCR1 triggers interrupt

    TA0CCR0 = 32768;                                // Set TA0 (and maximum) count value
    TA0CCR1 = 32768 / 4;                            // Set TA0 middle count value

    TA0CCTL0 &= ~CCIFG;                             // clear CCR0 interrupt flag
    TA0CCTL1 &= ~CCIFG;                             // clear CCR1 interrupt flag
}

