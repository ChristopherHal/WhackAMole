#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>


//Globals to handle the Game State and the current "target" LED that lights
//Utlized to make a player lose the game and make sure they press the correct button
int GAME_STATE = 1;
int TARGET;

void Determine_Game_State(int button){
    //Set up to handle accidental clicks in the case of the wait between new lights being lit up
    //This is handled by the or GAME_STATE == 0
    if((GAME_STATE == -1 && TARGET == button) || GAME_STATE == 0) GAME_STATE = 0;
    else GAME_STATE = 1;
}

void PortE_ISR(){
    // Registers that we need in the ISR
    unsigned int volatile *pEGPIOICR = (unsigned int *) (PortE + GPIOICR);
    *pEGPIOICR |= pin5;
    //Sends the button that was pressed into the game state
    Determine_Game_State(0);
}

void PortB_ISR(){
    // Registers that we need in the ISR
    unsigned int volatile *pBGPIOICR = (unsigned int *) (PortB + GPIOICR);
    *pBGPIOICR |= pin5;
    //Sends the button that was pressed into the game state
    Determine_Game_State(1);
}

void PortC_ISR(){
    // Registers that we need in the ISR
    unsigned int volatile *pCGPIOICR = (unsigned int *) (PortC + GPIOICR);
    *pCGPIOICR |= pin5;
    //Sends the button that was pressed into the game state
    Determine_Game_State(2);
}

void Wait_For_Restart(){
    unsigned int volatile *pBGPIODATA = (unsigned int *) (PortB + GPIODATA);
    int volatile intial_value = *pBGPIODATA & pin0;
    while((*pBGPIODATA & pin0) == intial_value){
        //Wait till it switches
    }
    GAME_STATE = -1;
}

void turn_On_Random_Small_Led(){

    unsigned int volatile *pEGPIODATA = (unsigned int *) (PortE + GPIODATA);
    unsigned int volatile *pBGPIODATA = (unsigned int *) (PortB + GPIODATA);
    unsigned int volatile *pCGPIODATA = (unsigned int *) (PortC + GPIODATA);

    //Clear the smaller LEDS
    *pEGPIODATA &= ~pin3;
    *pBGPIODATA &= ~pin6;
    *pCGPIODATA &= ~pin6;

    //Randomly pick a LED to turn on
    TARGET = rand() % 3;

    //Turn on the correct small light (the "mole")
    switch(TARGET) {
        case 0:
            *pEGPIODATA |= pin3;
            break;
        case 1:
            *pBGPIODATA |= pin6;
            break;
        case 2:
            *pCGPIODATA |= pin6;
            break;
    }
}

void Game_Win(){
    unsigned int volatile *pDGPIODATA = (unsigned int *) (PortD + GPIODATA);
    int i, j;
    int timer = 200000;
    for(i = 0; i < 6; i++){

        *pDGPIODATA &= ~pin2;
        turn_On_Random_Small_Led();

        for(j = 0; j < timer; j++){
            //Wait to allow the flashing affect
        }

        *pDGPIODATA |= 0x7;
        turn_On_Random_Small_Led();

        for(j = 0; j < timer; j++){
            //Wait to allow the flashing affect
        }

        *pDGPIODATA &= ~pin1;
        turn_On_Random_Small_Led();

        for(j = 0; j < timer; j++){
            //Wait to allow the flashing affect
        }


    }

}


int main(void)
{
    /***********************************************************
     *Initializing the ports and pins that are going to be used*
     ***********************************************************/

    //Buttons that are infron the lights
    SetupDigitalGPIO(PortC, pin5, INPUT, INTERRUPT); // Button 3
    SetupDigitalGPIO(PortE, pin5, INPUT, INTERRUPT); // Button 1
    SetupDigitalGPIO(PortB, pin5, INPUT, INTERRUPT); // Button 2

    //The three small LEDS
    SetupDigitalGPIO(PortE, pin3, OUTPUT, NoINTERRUPT); // LED 1
    SetupDigitalGPIO(PortB, pin6, OUTPUT, NoINTERRUPT); // LED 2
    SetupDigitalGPIO(PortC, pin6, OUTPUT, NoINTERRUPT); // LED 3

    //The large LED
    SetupDigitalGPIO(PortD, pin1, OUTPUT, NoINTERRUPT); // Big LED R Set to 0 to allow the voltage drop to occur
    SetupDigitalGPIO(PortD, pin2, OUTPUT, NoINTERRUPT); // Big LED G Set to 0 to allow the voltage drop to occur
    SetupDigitalGPIO(PortD, pin3, OUTPUT, NoINTERRUPT); // Big LED B Set to 0 to allow the voltage drop to occur

    //Tilt Switch and Buzzer
    SetupDigitalGPIO(PortB, pin0, INPUT, NoINTERRUPT); //Game switch
    SetupDigitalGPIO(PortE, pin1, OUTPUT, NoINTERRUPT);//Buzzer


    // Set priority
    NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // priority 5

    //Enable
    NVIC_EN0_R |= 0x16;


    //Setting the pointers to the data of the ports
    unsigned int volatile *pEGPIODATA = (unsigned int *) (PortE + GPIODATA);
    unsigned int volatile *pBGPIODATA = (unsigned int *) (PortB + GPIODATA);
    unsigned int volatile *pCGPIODATA = (unsigned int *) (PortC + GPIODATA);
    unsigned int volatile *pDGPIODATA = (unsigned int *) (PortD + GPIODATA);

    //Declare the level
    float level;

    while (1)
    {
        //Set the game state and start the game
        if (GAME_STATE == 1){
            Wait_For_Restart();
            level = 1;
        }
        else {
            level += 0.5;
            GAME_STATE = -1;
        }

        //Change the big LED to Blue
        *pDGPIODATA |= 0x7;

        //Randomly choose a light/button (The "mole")
        TARGET = rand() % 3;

        //Turn on the correct small light (the "mole")
        switch(TARGET) {
            case 0:
                *pEGPIODATA |= pin3;
                break;
            case 1:
                *pBGPIODATA |= pin6;
                break;
            case 2:
                *pCGPIODATA |= pin6;
                break;
        }

        //Start timer to determine if button pressed in time
        //Increasing difficulty the later on in the game the player goes
        int timer = 2000000;
        int x = 0;
        while(GAME_STATE == -1){
            x++;
            if (x == (int)(timer / (int)(level > 10 ? 10 : level))){
                GAME_STATE = 1;
            }
        }

        //Flash light corresponding to game state
        switch(GAME_STATE) {
            case 1:
                //Change the Big LED to Red and turn the buzzer on
                *pDGPIODATA &= ~pin1;
                *pEGPIODATA |= pin1;
                break;
            case 0:
                *pDGPIODATA &= ~pin2;
                if (level == 15.5){
                        Game_Win();
                        GAME_STATE = 1;
                }
                break;
        }
        //Clear the smaller LEDS
        *pEGPIODATA &= ~pin3;
        *pBGPIODATA &= ~pin6;
        *pCGPIODATA &= ~pin6;
        for(x = 0; x < (int)(timer / (int)(level > 10 ? 10 : level)); x++) {
            //wait :)
        }
        //Turn off the buzzer
        *pEGPIODATA &= ~pin1;
    }
}
