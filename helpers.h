/*
 * helpers.h
 *
 *  Created on: Mar 15, 2018
 *      Author: Chance Tarver
 *  Updated on: Nov 11, 2020
 *      Author: Christopher Hallock-Solomon
 */

#ifndef HELPERS_H_
#define HELPERS_H_


// Base addresses of each port
#define PortA       0x40004000
#define PortB       0x40005000
#define PortC       0x40006000
#define PortD       0x40007000
#define PortE       0x40024000
#define PortF       0x40025000

//  General-Purpose Input/Output Run Mode Clock Gating Control. Each bit corresponds to a Port.
#define RCGCGPIO    0x400FE608
#define ClocksA     0x01
#define ClocksB     0x02
#define ClocksC     0x04
#define ClocksD     0x08
#define ClocksE     0x10
#define ClocksF     0x20

// Define the bitmask for each pin
#define pin0     0x01
#define pin1     0x02
#define pin2     0x04
#define pin3     0x08
#define pin4     0x10
#define pin5     0x20
#define pin6     0x40

//Other defines for code readability
#define OUTPUT  1
#define INPUT   0
#define INTERRUPT    1
#define NoINTERRUPT  0

// Offsets corresponding to registers we may need to configure
#define GPIODATA    0x3FC   // pg662 : GPIO Data
#define GPIODIR     0x400   // pg663 : GPIO Direction
#define GPIOIS      0x404   // pg664 : GPIO Interrupt Sense
#define GPIOIBE     0x408   // pg665 : GPIO Interrupt Both Edges
#define GPIOIEV     0x40C   // pg666 : GPIO Interrupt Event     -   0: falling edge or Low level is trigger.    1: rising edge or High level is trigger
#define GPIOIM      0x410   // pg667 : GPIO Interrupt Mask      -   0: the pin is masked.                       1: the interrupt is sent to the controller
#define GPIOICR     0x41C   // pg670 : GPIO Interrupt Clear     -   0: no affect                                1: the corresponding interrupt is cleared
#define GPIOAFSEL   0x420   // pg671 : GPIO Alternative Function Select - 0: pin functions as GPIO              1: Function as something else depending on port
#define GPIOPUR     0x510   // pg677 : GPIO Pull-Up Select      -   0: turn off pull up resistor                1: turn on pull up for corresponding pin
#define GPIOPDR     0x514   // pg679 : GPIO Pull-Down Select    -   0: turn off pull down resistor              1: turn on pull down for corresponding pin
#define GPIODEN     0x51C   // pg682 : GPIO Digital Enable      -   0: disable digital functions                1: enable pin's digital functions
#define GPIOLOCK    0x520   // pg684 : GPIO Lock. A write of the value 0x4C4F.434B unlocks the GPIO Commit (GPIOCR) register for write access.
#define GPIOKEY     0x4C4F434B // pg684. Special key for the GPIOLOCK register
#define GPIOCR      0x524   // pg685 : GPIO Commit              -   0: Corresponding GPIOAFSEL, GPIOPUR, GPIOPDR, or GPIODEN bits cannot be written. 1: They can

#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))  // pg142 IRQ 0 to 31 Set Enable Register

#define NVIC_PRI0_R             (*((volatile unsigned long *)0xE000E41C))  // pg152 IRQ 0 to 3 Priority Register
#define NVIC_PRI7_R             (*((volatile unsigned long *)0xE000E41C))  // pg152 IRQ 28 to 31 Priority Register  pg 104 has interrupt assignments. GPIO Port F is Interrupt #30. Bits 23:21

void ErrorFault();

void WaitForInterrupt(void);  // low power mode

void SetupDigitalGPIO(unsigned int port, unsigned int pinMask, int direction,
                        int useInterrupts)
{
    /*Function for setting up a digital GPIO Pin

     This function will do all the register configs for a give port and pin.

     Examples
     -------
     SetupOnDigitalGPIO(PortF, pin3, INPUT, INTERRUPT) // Set up an input with interrupts on PF3.
     SetupOnDigitalGPIO(PortE, pin4, OUTPUT, NoINTERRUPT) // Set up an output with no interrupts on PE4.
     Notes
     -----
     We assume a pull down resistor. See like 96 and change it to false
     We also assume an edge based trigger.
     It is best the header with the #defines for each port and pin are included.

     Inputs
     ------
     (*((volatile unsigned long *) port  - Base address of the port being used.
     (*((volatile unsigned long *) pin   - Bit mask of the pin being used
     int direction - Boolean flag for direction of the pin. 1 = Output. 0 = input
     int UseInterrupts - Boolean flag to use interrupts. 1 = use them. 0 = don't
     */

    //Choosing if it is pull down or pull up
    //Change to not 1 for pull up and 1 for pull down
    //Could be an input into the function
    int pull_down_res = 1;

    //Define the generic pointers
    unsigned int volatile *pRCGCGPIO = (unsigned int *) RCGCGPIO;
    unsigned int volatile *pGPIOLOCK = (unsigned int *) (port + GPIOLOCK);
    unsigned int volatile *pGPIOCR = (unsigned int *) (port + GPIOCR);
    unsigned int volatile *pGPIODIR = (unsigned int *) (port + GPIODIR);
    unsigned int volatile *pGPIOAFSEL = (unsigned int *) (port + GPIOAFSEL);
    unsigned int volatile *pGPIOPDR = (unsigned int *) (port + GPIOPDR);
    unsigned int volatile *pGPIOPUR = (unsigned int *) (port + GPIOPUR);
    unsigned int volatile *pGPIODEN = (unsigned int *) (port + GPIODEN);
    unsigned int volatile *pGPIODATA = (unsigned int *) (port + GPIODATA);

    // Define the pointers for interrupt configuration
    unsigned int volatile *pGPIOIS = (unsigned int *) (port + GPIOIS);
    unsigned int volatile *pGPIOIBE = (unsigned int *) (port + GPIOIBE);
    unsigned int volatile *pGPIOIEV = (unsigned int *) (port + GPIOIEV);
    unsigned int volatile *pGPIOIM = (unsigned int *) (port + GPIOIM);
    unsigned int volatile *pGPIOICR = (unsigned int *) (port + GPIOICR);

    // activate the clocks for the port

    int clocks;
    switch (port)
    {
    case PortA:
        clocks = ClocksA;
        break;
    case PortB:
        clocks = ClocksB;
        break;
    case PortC:
        clocks = ClocksC;
        break;
    case PortD:
        clocks = ClocksD;
        break;
    case PortE:
        clocks = ClocksE;
        break;
    case PortF:
        clocks = ClocksF;
        break;
    default:
        ErrorFault();//ERROR
    }

    *pRCGCGPIO |= clocks;
    while ((*pRCGCGPIO & clocks) == 1)
        ;

    *pGPIOLOCK = GPIOKEY;
    *pGPIOCR |= pinMask;
    if (direction == 0)
    {
        *pGPIODIR &= ~pinMask;
        if(pull_down_res  == 1){
            *pGPIOPDR |= pinMask;
        }
        else{
            *pGPIOPUR |= pinMask;
        }

    }
    else if (direction == 1)
    {
        *pGPIODIR |= pinMask;
    }
    else
    {
        *pGPIODIR |= pinMask; // TODO. Add an exception to handle this.
    }

    *pGPIOAFSEL &= ~pinMask;
    *pGPIODEN |= pinMask;

    if (useInterrupts)
    {
        *pGPIOIS &= ~pinMask;  // Edge-sensitive (default setting)
        *pGPIOIBE &= ~pinMask;  // Not both edges (default setting)
        *pGPIOIEV &= ~pinMask;  // Falling edge event (default setting)
        *pGPIOICR |= pinMask;   // clear flag
        *pGPIOIM |= pinMask;   // enable interrupt. Unmask it
    }
}


void WaitForInterrupt(void)
{
    __asm ("    WFI\n"
            "    BX     LR\n");
}

void ErrorFault(){
    while(1){
        //An error occurred within the set up the GPIO Run Mode Clock, check your port within the function call
    }
}

#endif /* HELPERS_H_ */
