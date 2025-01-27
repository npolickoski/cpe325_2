/*------------------------------------------------------------------------------
 * Initial Build::
 * File:        lab10_p3.c
 * Description: Output wave to osciloscope and change depending on button presses
 *
 * Input:       Switch presses
 * Output:      Change to wave (triangle, sine, double amplitude)
 * Author(s):   Polickoski, Nick
 * Date:        October 22, 2023
 *----------------------------------------------------------------------------*/

// Preprocessor Directives
#include <msp430fg4618.h>
#include "TriangleWaveLUT_512.h"                    // triangle-wave input file
#include "SineWaveLUT_512.h"                        // sine-wave input file



// Function Prototypes
void TimerA_setup(void);
void DAC_setup(void);
void Switch_setup(void);

void FirstSwtich(void);
void SecondSwtich(void);



// Global Variables
unsigned int* wavePtr = TriangleWaveLUT;            // initalize pointer to triangle-wave values (will change)
int amplitudeConst = 1;                             // for change in amplitude



// Call to Main
void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                       // stop watch dog timer

    TimerA_setup();                                 // set timer to uniformly distribute the samples
    DAC_setup();                                    // setup DAC
    Switch_setup();                                 // setup switches

    unsigned int i;
    for (i = 0; ; i = (i + 1) % 512)
    {
        __bis_SR_register(LPM0_bits + GIE);         // Enter LPM0, interrupts enabled

        DAC12_0DAT = amplitudeConst * (wavePtr[i] / 2);     // display triangle-wave
    }

    return;
}



//// Interrupt Definitions
// Timer A
#pragma vector = TIMERA0_VECTOR
__interrupt void TA0_ISR(void)
{
    __bic_SR_register_on_exit(LPM0_bits);           // Exit LPMx, interrupts enabled

    return;
}


// Switches
#pragma vector = PORT1_VECTOR
__interrupt void Switches_ISR(void)
{
    // Switch #1
    if (!(P1IN & BIT0))                             // if (switch #1 = ON) (falling edge)
    {
        P1IFG &= ~BIT0;                             // clear interrupt flag

        unsigned int i;
        for (i = 2000; i > 0; i--);                 // ~20ms debounce

        if (!(P1IN & BIT0) && (P1IN & BIT1))        // 2nd switch check - switch #1
        {
            wavePtr = SineWaveLUT;                  // change to sine-wave
            P1IES &= ~BIT0;                         // change interrupt to go from low to high
        }
    }
    else if (P1IN & BIT0)                           // if (switch #1 = OFF) (rising edge)
    {
        P1IFG &= ~BIT0;                             // clear interrupt flag

        unsigned int i;
        for (i = 2000; i > 0; i--);                 // ~20ms debounce

        if ((P1IN & BIT0) && (P1IN & BIT1))         // 2nd switch check - switch #2
        {
            wavePtr = TriangleWaveLUT;              // reset pointer to triangle-wave values
            P1IES |= BIT0;                          // change interrupt to go from high to low
        }
    }


    // Switch #2
    if (!(P1IN & BIT1))                             // if (switch #2 = ON)
    {
        P1IFG &= ~BIT1;                             // clear interrupt flag

        unsigned int i;
        for (i = 2000; i > 0; i--);                 // ~20ms debounce

        if (!(P1IN & BIT1))                         // 2nd switch check - switch #2
        {
            amplitudeConst = 2;                     // double ampiltude
            P1IES &= ~BIT1;                         // change interrupt to go from low to high
        }
    }
    else if (P1IN & BIT1)                           // if (switch #2 = OFF)
    {
        P1IFG &= ~BIT1;                             // clear interrupt flag

        unsigned int i;
        for (i = 2000; i > 0; i--);                 // ~20ms debounce

        if (P1IN & BIT1)                            // 2nd switch check - switch #2
        {
            amplitudeConst = 1;                     // reset amplitude
            P1IES |= BIT1;                          // change interrupt to go from high to low
        }
    }

    return;
}



//// Function Definitions
void TimerA_setup(void)
{
    TACTL = TASSEL_2 + MC_1;                        // SMCLK, UP mode
    TACCR0 = 41;                                    // Sets Timer Freq -> (1048576)/(50 * 256)
    TACCTL0 = CCIE;                                 // CCR0 interrupt enabled

    return;
}


void DAC_setup(void)
{
    ADC12CTL0 = REF2_5V + REFON;                    // Turn on 2.5V internal ref volage

    unsigned int i = 0;
    for (i = 50000; i > 0; i--);                    // Delay to allow Ref to settle

    DAC12_0CTL = DAC12IR + DAC12AMP_5 + DAC12ENC;   //Sets DAC12

    return;
}


void Switch_setup(void)
{
    // Switch #1
    P1DIR &= ~BIT0;                                 // input selection
    P1OUT |= BIT0;                                  // formatting shit (proper I/O)
    P1IE |= BIT0;                                   // enable interrupts
    P1IES |= BIT0;                                  // initialize interrupt from high to low

    // Switch #2
    P1DIR &= ~BIT1;                                 // input selection
    P1OUT |= BIT1;                                  // formatting shit (proper I/O)
    P1IE |= BIT1;                                   // enable interrupts
    P1IES |= BIT1;                                  // initialize interrupt from high to low

    return;
}

