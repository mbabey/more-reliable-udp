//
// Created by prabh on 11/9/22.
//

#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

// Need to install wiring for wiring library.
#include "Game.h"
#include <softPwm.h>
#include <stdbool.h>
#include <stdio.h>
#include <wiringPi.h>

typedef unsigned char uchar;
typedef unsigned int  uint;

/**
 * lightSwitch
 * <p>
 * Activate or turn off light on player's controller.
 * </p>
 * @param state True/False for if light should be on.
 */
void lightSwitch(bool state);

/**
 * controllerSetup
 * <p>
 * The setup needed for controller wiring.
 * </p>
 * @return 0 on success, -1 on failure
 */
int controllerSetup(void);

/**
 * get_ADC_Result
 * <p>
 * Get the result of analog-to-digital conversion on a specified channel of an ADC0834
 * </p>
 * @param channel - the channel to read
 * @return the converted voltage level
 */
uchar get_ADC_Result(uint channel);

/**
 * useController
 * <p>
 * Handle movement and button pressed.
 * </p>
 * @param currentCursor - the current cursor position
 * @param btn - pointer to boolean dictating button press
 * @return the updated cursor position
 */
int useController(int currentCursor, volatile bool *btn);

/**
 * adjustHorizontal
 * <p>
 * Read the channel corresponding to horizontal position and update the a cursor position appropriately.
 * </p>
 * @param joystickX - the X channel of the ADC
 * @param currentCursor - the current cursor position
 * @return - the updated cursor position
 */
int adjustHorizontal(int joystickX, int currentCursor);

/**
 * adjustVertical
 * <p>
 * Read the channel corresponding to vertical position and update the a cursor position appropriately.
 * </p>
 * @param joystickY - the Y channel of the ADC
 * @param currentCursor - the current cursor position
 * @return - the updated cursor position
 */
int adjustVertical(int joystickY, int currentCursor);

#endif //GAME_CONTROLLER_H
