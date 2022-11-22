//
// Created by prabh on 11/9/22.
//

#include "../include/Controller.h"

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


// Initial set up for wiring.
int controllerSetup(void){

    if(wiringPiSetup() == -1){ //when initialize wiring failed,print messageto screen
        printf("setup wiringPi failed !");
        //TODO: ADD ERROR HANDLING HERE.
        return -1;
    }

    //Setting modes for the analog stick.
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
    uchar dat1=0, dat2=0;
    int sel = channel > 1 & 1;
    int odd = channel & 1;
    pinMode(ADC_DIO, OUTPUT);
    digitalWrite(ADC_CS, 0);
    // Start bit
    digitalWrite(ADC_CLK,0);
    digitalWrite(ADC_DIO,1);    delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK,1);    delayMicroseconds(CLOCK_PERIOD);
//Single End mode
    digitalWrite(ADC_CLK,0);
    digitalWrite(ADC_DIO,1);    delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK,1);    delayMicroseconds(CLOCK_PERIOD);
    // ODD
    digitalWrite(ADC_CLK,0);
    digitalWrite(ADC_DIO,odd);  delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK,1);    delayMicroseconds(CLOCK_PERIOD);
    //Select
    digitalWrite(ADC_CLK,0);
    digitalWrite(ADC_DIO,sel);    delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK,1);
    digitalWrite(ADC_DIO,1);    delayMicroseconds(CLOCK_PERIOD);
    digitalWrite(ADC_CLK,0);
    digitalWrite(ADC_DIO,1);    delayMicroseconds(CLOCK_PERIOD);
    for(i=0;i<8;i++)
    {
        digitalWrite(ADC_CLK,1);    delayMicroseconds(CLOCK_PERIOD);
        digitalWrite(ADC_CLK,0);    delayMicroseconds(CLOCK_PERIOD);
        pinMode(ADC_DIO, INPUT);
        dat1=dat1<<1 | digitalRead(ADC_DIO);
    }
    for(i=0;i<8;i++)
    {
        dat2 = dat2 | ((uchar)(digitalRead(ADC_DIO))<<i);
        digitalWrite(ADC_CLK,1);    delayMicroseconds(CLOCK_PERIOD);
        digitalWrite(ADC_CLK,0);    delayMicroseconds(CLOCK_PERIOD);
    }
    digitalWrite(ADC_CS,1);
    pinMode(ADC_DIO, OUTPUT);
    return(dat1==dat2) ? dat1 : 0;
}

void lightSwitch(bool state) {
    if(state)
        digitalWrite(LIGHT_TURN, LOW);

    else
        digitalWrite(LIGHT_TURN, HIGH);
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

    if(joystickY <= up && currentCursor - ROW_SHIFT >= GRID_BOUNDARY_TOP_LEFT)
    { return currentCursor - ROW_SHIFT; }

    if(joystickY >= down && currentCursor + ROW_SHIFT <= GRID_BOUNDARY_BOTTOM_RIGHT)
    { return currentCursor + ROW_SHIFT; }
    
    return currentCursor;
}

//TODO: NO MORE TELEPORTING EDGES GING LEFT AND RIGHT.
int adjustHorizontal(int joystickX, int currentCursor) {
    int left = ANALOG_V_UPPER_BOUND;
    int right = ANALOG_V_LOWER_BOUND;

    if(joystickX >= left && currentCursor - COL_SHIFT >= GRID_BOUNDARY_TOP_LEFT && currentCursor != GRID_BOUNDARY_MID_LEFT && currentCursor != GRID_BOUNDARY_BOTTOM_LEFT)
    { return currentCursor - COL_SHIFT; }
    
    if(joystickX <= right && currentCursor + COL_SHIFT <= GRID_BOUNDARY_BOTTOM_RIGHT && currentCursor != GRID_BOUNDARY_TOP_RIGHT && currentCursor != GRID_BOUNDARY_MID_RIGHT)
    { return currentCursor + COL_SHIFT; }
    
    return currentCursor;
}
