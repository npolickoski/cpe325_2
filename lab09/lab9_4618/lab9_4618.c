/*------------------------------------------------------------------------------
 * Initial Build::
 * File:        lab9_4618.c
 * Description:     communicates with both the terminal through asynchrnous
 *              communication using UART as well as the F2013 through
 *              synchronous communication using SPI to send values
 * Input:       Inputted characters from the terminal/blink count from F2013
 * Output:      Duty values to to the F2013/blink count values to the terminal
 * Author(s):   Polickoski, Nick
 * Date:        October 12, 2023
 *
 * lacasauah.edu <- Dr. Mel website
 *----------------------------------------------------------------------------*/

//// Preprocessor Directives
// Libraries
#include <msp430.h>
#include "msp430fg4618.h"
#include <stdio.h>


// Global Variables
char lineReset[] = "\r\n";                          // line reset


// Function Prototypes
void UART_initialize(void);
void UART_sendCharacter(char c);
char UART_getCharacter(void);
void UART_sendString(char* string);
void UART_getLine(char* buffer, int limit);

void processString(char* buffer, int limit);
void handleDash();
void handleQuestion(char* buffer, int limit);
void handleNumbers(int cycle);
void handleInvalid();

void SPI_setup(void);
unsigned char SPI_getState(void);
void SPI_setState(unsigned char State);



// Call To Main
void main(void)
{
    // Stop Watchdog Timer/UART Initialization
    WDTCTL = WDTPW + WDTHOLD;
    UART_initialize();
    SPI_setup();
    char buffer[500];                                   // user typing string

    for (;;)
    {
        UART_sendString("Enter Duty Cycle Rate (0 - 100), - to reset rate, ? for current rate: ");
        UART_getLine(buffer, 500);
        UART_sendString(lineReset);

        processString(buffer, 500);
    }

    return;
}



// Function Definitions
void UART_initialize(void)
{
    UCA0CTL1 |= UCSWRST;                                // Set software reset during initialization
    P2SEL |= BIT4 | BIT5;                               // Set UC0TXD and UC0RXD to transmit and receive
    UCA0CTL1 |= UCSSEL_2;                               // Clock source SMCLK

    UCA0BR0 = 18;                                       // 1048576 Hz / 57600 = 18 R 1
    UCA0BR1 = 0;
    UCA0MCTL = 0x02;                                    // Modulation (2d = 1 * 2))

    UCA0CTL1 &= ~UCSWRST;                               // Clear software reset

    return;
}


void UART_sendCharacter(char c)
{
    while (!(IFG2 & UCA0TXIFG));                        // USCI_A0 TX buffer ready?
    UCA0TXBUF = c;                                      // Put character into tx buffer

    return;
}


char UART_getCharacter()
{
    while (!(IFG2 & UCA0RXIFG));                         // Wait for a new character
    // New character is here in UCA0RXBUF
    while (!(IFG2 & UCA0TXIFG));                         // USCI_A0 TX buffer ready?
    UCA0TXBUF = UCA0RXBUF;                              // TXBUF <= RXBUF (echo)

    return UCA0RXBUF;
}


void UART_sendString(char* string)
{
    int i;
    for (i = 0; string[i] != 0; i++)                    // iterates through all characters in string and sends em
    {
        UART_sendCharacter(string[i]);
    }

    return;
}


void UART_getLine(char* buffer, int limit)
{
    char c;                                             // temp

    int i;
    for (i = 0; i < (limit - 1); i++)                   // break #1: limit reached (1 less than limit to allow space for NULL)
    {
        c = UART_getCharacter();                        // retrieve character
        if (c == '\r')                                  // break #2: carriage return reached
        {
            break;
        }

        buffer[i] = c;                                  // adds characters to buffer
    }

    buffer[i] = 0;                                      // terminate with NULL character

    return;
}


void SPI_setup(void)
{
    UCB0CTL0 = UCMSB + UCMST + UCSYNC;                  // Sync. mode, 3-pin SPI, Master mode, 8-bit data
    UCB0CTL1 = UCSSEL_2 + UCSWRST;                      // SMCLK and Software reset

    UCB0BR0 = 0x02;                                     // Data rate = SMCLK/2 ~= 500kHz
    UCB0BR1 = 0x00;

    P3SEL |= BIT1 + BIT2 + BIT3;                        // P3.1,P3.2,P3.3 option select
    UCB0CTL1 &= ~UCSWRST;                               // **Initialize USCI state machine**
}


unsigned char SPI_getState(void)
{
    while(P3IN & 0x01);                                 // Verifies busy flag (master in; slave out)

    IFG2 &= ~UCB0RXIFG;
    UCB0TXBUF = 255;                                    // Dummy write to start SPI (255 max value)

    while (!(IFG2 & UCB0RXIFG));                        // USCI_B0 TX buffer ready?

    return UCB0RXBUF;
}


void SPI_setState(unsigned char State)
{
    while(P3IN & 0x01);                                 // Verifies busy flag

    IFG2 &= ~UCB0RXIFG;
    UCB0TXBUF = State;                                  // Write new state

    while (!(IFG2 & UCB0RXIFG));                        // USCI_B0 TX buffer ready?

    return;
}


void processString(char* buffer, int limit)
{
    if (!strcmp(buffer, "-"))                           // for resetting blink count
    {
        handleDash();
    }
    else if (!strcmp(buffer, "?"))                      // for getting current blink count
    {
        handleQuestion(buffer, limit);
    }
    else
    {
        int dutyCycle = atoi(buffer);

        if (!strcmp(buffer, "0") || (dutyCycle > 0 && dutyCycle <= 100)) // for numbers that change the duty cycle
        {
            handleNumbers(dutyCycle);
        }
        else                                            // for any other invalid value
        {
            handleInvalid();
        }
    }

    return;
}


void handleDash()
{
    SPI_setState(200);
    UART_sendString("Current Blinks Reset");
    UART_sendString(lineReset);

    return;
}


void handleQuestion(char* buffer, int limit)
{
    UART_sendString("Current Blinks: ");
    int currBlinkRate = SPI_getState();

    snprintf(buffer, limit, "%d", currBlinkRate);

    UART_sendString(buffer);
    UART_sendString(lineReset);

    return;
}


void handleNumbers(int cycle)
{
    SPI_setState(cycle);
    //UART_sendString("penis");
    UART_sendString(lineReset);

    return;
}


void handleInvalid()
{
    UART_sendString("Invalid duty cycle entered");
    UART_sendString(lineReset);

    return;
}

