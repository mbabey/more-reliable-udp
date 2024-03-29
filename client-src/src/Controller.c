//
// Created by prabh on 11/9/22.
//

#include "../include/Controller.h"

/**
 * WiringPi mapping for Raspberry Pi GPIO pins.
 */
#define ADC_CS    0  // GPIO17
#define ADC_CLK   1  // GPIO18
#define ADC_DIO   2  // GPIO27
#define BtnPin    3  // GPIO22
#define LIGHT_TURN 4 // GPIO23

/**
 * Bounds for registering analog channel outputs.
 */
#define ANALOG_V_LOWER_BOUND 10
#define ANALOG_V_UPPER_BOUND 245

/**
 * Delay time for sampling analog signal (ms).
 */
#define ANALOG_INPUT_DELAY 150

/**
 * Clock period (mics).
 */
#define CLOCK_PERIOD 2

/**
 * Values to move cursor around the board.
 */
#define ROW_SHIFT 3
#define COL_SHIFT 1

/**
 * Board boundaries to limit cursor movement.
 */
#define GRID_BOUNDARY_TOP_LEFT 0
#define GRID_BOUNDARY_TOP_RIGHT 2
#define GRID_BOUNDARY_MID_LEFT 3
#define GRID_BOUNDARY_MID_RIGHT 5
#define GRID_BOUNDARY_BOTTOM_LEFT 6
#define GRID_BOUNDARY_BOTTOM_RIGHT 8

/**
 * Eight.
 */
#define EIGHT 8

// Initial set up for wiring.
int controllerSetup(void){

    if(wiringPiSetup() == -1){ //when initialize wiring failed,print message to screen
        printf("setup wiringPi failed !");
        return -1;
    }

    //Setting modes for the analog stick, provided by SunFounder & WiringPi.
    pinMode(BtnPin,  INPUT);
    pullUpDnControl(BtnPin, PUD_UP);
    pinMode(ADC_CS,  OUTPUT);
    pinMode(ADC_CLK, OUTPUT);

    // Setting mode for button.
    pinMode(LIGHT_TURN, OUTPUT);

    return 0;
}

// For converting information from analog stick.
uchar get_ADC_Result(uint channel)
{
    uchar i;
    uchar dat1 = 0;
    uchar dat2 = 0;
    int sel = (channel > 1) & 1; // NOLINT(hicpp-signed-bitwise, bugprone-narrowing-conversion, cppcoreguidelines-narrowing-conversions) : WiringPi default
    int odd = channel & 1; // NOLINT(hicpp-signed-bitwise) : WiringPi default

    // Reading for the analog stick, provided by SunFounder & WiringPi.
    pinMode(ADC_DIO, OUTPUT);
    digitalWrite(ADC_CS, 0);
    // Start bit
    digitalWrite(ADC_CLK, 0);
    digitalWrite(ADC_DIO, 1);    delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK, 1);    delayMicroseconds(CLOCK_PERIOD);
    //Single End mode
    digitalWrite(ADC_CLK, 0);
    digitalWrite(ADC_DIO, 1);    delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK, 1);    delayMicroseconds(CLOCK_PERIOD);
    // ODD
    digitalWrite(ADC_CLK, 0);
    digitalWrite(ADC_DIO, odd);  delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK, 1);    delayMicroseconds(CLOCK_PERIOD);
    //Select
    digitalWrite(ADC_CLK, 0);
    digitalWrite(ADC_DIO, sel);    delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK, 1);
    digitalWrite(ADC_DIO, 1);    delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK, 0);
    digitalWrite(ADC_DIO, 1);    delayMicroseconds(CLOCK_PERIOD);
    for (i = 0; i < EIGHT; i++)
    {
        digitalWrite(ADC_CLK, 1);    delayMicroseconds(CLOCK_PERIOD);
        digitalWrite(ADC_CLK, 0);    delayMicroseconds(CLOCK_PERIOD);
        pinMode(ADC_DIO, INPUT);
        dat1=dat1<<1 | digitalRead(ADC_DIO); // NOLINT(hicpp-signed-bitwise) : WiringPi default
    }
    for (i = 0; i < EIGHT; i++)
    {
        dat2 = dat2 | ((uchar) (digitalRead(ADC_DIO)) << i); // NOLINT(hicpp-signed-bitwise) : WiringPi default
        digitalWrite(ADC_CLK, 1);    delayMicroseconds(CLOCK_PERIOD);
        digitalWrite(ADC_CLK, 0);    delayMicroseconds(CLOCK_PERIOD);
    }
    digitalWrite(ADC_CS, 1);
    pinMode(ADC_DIO, OUTPUT);
    return (dat1 == dat2) ? dat1 : 0;
}

void lightSwitch(bool state) {
    // Change output on light, digitalWrite() provided by SunFounder & WiringPi.
    if (state)
    {
        digitalWrite(LIGHT_TURN, LOW);
    } else
    {
        digitalWrite(LIGHT_TURN, HIGH);
    }
}


int useController(int currentCursor, volatile bool *btn){
    int tempCursor = currentCursor;
    
    lightSwitch(true);

    // While no movement.
    while(tempCursor == currentCursor) {

        // adjust analog input and make it game equivalent.
        tempCursor = adjustHorizontal(get_ADC_Result(1), tempCursor);
        tempCursor = adjustVertical(get_ADC_Result(0), tempCursor);

        // Update button was pressed and return current position.
        if(!digitalRead(BtnPin))
        {
            lightSwitch(false);
            *btn = true;
            delay(ANALOG_INPUT_DELAY);
            return tempCursor;
        }

        delay(ANALOG_INPUT_DELAY);
    }

    // If just moved then just send position.
    lightSwitch(false);
    { return tempCursor; }
}

int adjustVertical(int joystickY, int currentCursor) {
    int up = ANALOG_V_LOWER_BOUND;
    int down = ANALOG_V_UPPER_BOUND;

    // Adjusting for moving up a row and insuring wont go out of bounds of game array.
    if(joystickY <= up && currentCursor - ROW_SHIFT >= GRID_BOUNDARY_TOP_LEFT) // NOLINT(-Wstrict-overflow) : overflow will not occur in value range
    { return currentCursor - ROW_SHIFT; }

    // Adjusting for moving down a row and insuring wont go out of bounds of game array.
    if(joystickY >= down && currentCursor + ROW_SHIFT <= GRID_BOUNDARY_BOTTOM_RIGHT) // NOLINT(-Wstrict-overflow) : overflow will not occur in value range
    { return currentCursor + ROW_SHIFT; }
    
    return currentCursor;
}

int adjustHorizontal(int joystickX, int currentCursor) {
    int left = ANALOG_V_UPPER_BOUND;
    int right = ANALOG_V_LOWER_BOUND;

    // Adjusting for moving to the left column and insuring wont go out of bound of game array.
    if(joystickX >= left && currentCursor - COL_SHIFT >= GRID_BOUNDARY_TOP_LEFT && currentCursor != GRID_BOUNDARY_MID_LEFT && currentCursor != GRID_BOUNDARY_BOTTOM_LEFT) // NOLINT(-Wstrict-overflow) : overflow will not occur in value range
    { return currentCursor - COL_SHIFT; }

    // Adjusting for moving to the right column and insuring wont go out of bound of game array.
    if(joystickX <= right && currentCursor + COL_SHIFT <= GRID_BOUNDARY_BOTTOM_RIGHT && currentCursor != GRID_BOUNDARY_TOP_RIGHT && currentCursor != GRID_BOUNDARY_MID_RIGHT) // NOLINT(-Wstrict-overflow) : overflow will not occur in value range
    { return currentCursor + COL_SHIFT; }
    
    return currentCursor;
}
