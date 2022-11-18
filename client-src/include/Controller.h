//
// Created by prabh on 11/9/22.
//

#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

// Need to install wiring for wiring library.
#include <wiringPi.h>
#include <stdio.h>
#include <softPwm.h>
#include <stdbool.h>
#include "Game.h"

typedef unsigned char uchar;
typedef unsigned int uint;

#define     ADC_CS    0
#define     ADC_CLK   1
#define     ADC_DIO   2
#define     BtnPin    3
#define     LIGHT_TURN 4 //GPIO23

// TODO: MAKE LIGHT ON WORK.
/**
 * Activate or turn off light on player's controller.
 * @param state True/False for if light should be on.
 */
void lightSwitch(bool state);

//TODO: MAKE ANALOG STICK WORK
/**
 * The setup needed for controller wiring.
 */
void controllerSetup();

/**
 * Main funciton for getting directions.
 * @param channel
 * @return
 */
uchar get_ADC_Result(uint channel);

/**
 * Handle movement and button pressed.
 * @param game
 * @param btn
 * @return
 */
int useController(int cursorCurrent, bool* btn);

int adjustHorizontal(int joystickX, int currentCursor);

int adjustVertical(int joystickY, int currentCursor);



//TODO: MAKE BUTTON PRESS WORK.

#endif //GAME_CONTROLLER_H
