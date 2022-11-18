//
// Created by prabh on 11/9/22.
//

#include "../include/Controller.h"

// Initial set up for wiring.
void controllerSetup(){

    if(wiringPiSetup() == -1){ //when initialize wiring failed,print messageto screen
        printf("setup wiringPi failed !");
        //TODO: ADD ERROR HANDLING HERE.
        //return 1;
    }

    //Setting modes for the analog stick.
    pinMode(BtnPin,  INPUT);
    pullUpDnControl(BtnPin, PUD_UP);
    pinMode(ADC_CS,  OUTPUT);
    pinMode(ADC_CLK, OUTPUT);

    // Setting mode for button.
    pinMode(LIGHT_TURN, OUTPUT);

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
    digitalWrite(ADC_DIO,1);    delayMicroseconds(2);
    digitalWrite(ADC_CLK,1);    delayMicroseconds(2);
//Single End mode
    digitalWrite(ADC_CLK,0);
    digitalWrite(ADC_DIO,1);    delayMicroseconds(2);
    digitalWrite(ADC_CLK,1);    delayMicroseconds(2);
    // ODD
    digitalWrite(ADC_CLK,0);
    digitalWrite(ADC_DIO,odd);  delayMicroseconds(2);
    digitalWrite(ADC_CLK,1);    delayMicroseconds(2);
    //Select
    digitalWrite(ADC_CLK,0);
    digitalWrite(ADC_DIO,sel);    delayMicroseconds(2);
    digitalWrite(ADC_CLK,1);
    digitalWrite(ADC_DIO,1);    delayMicroseconds(2);
    digitalWrite(ADC_CLK,0);
    digitalWrite(ADC_DIO,1);    delayMicroseconds(2);
    for(i=0;i<8;i++)
    {
        digitalWrite(ADC_CLK,1);    delayMicroseconds(2);
        digitalWrite(ADC_CLK,0);    delayMicroseconds(2);
        pinMode(ADC_DIO, INPUT);
        dat1=dat1<<1 | digitalRead(ADC_DIO);
    }
    for(i=0;i<8;i++)
    {
        dat2 = dat2 | ((uchar)(digitalRead(ADC_DIO))<<i);
        digitalWrite(ADC_CLK,1);    delayMicroseconds(2);
        digitalWrite(ADC_CLK,0);    delayMicroseconds(2);
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


int useController(int currentCursor, bool *btn){
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
            delay(150);
            return tempCursor;
        }

        delay(200);
    }

    // If just moved then just send position.
    lightSwitch(false);
    return  tempCursor;
}

int adjustVertical(int joystickY, int currentCursor) {
    int up = 0;
    int down = 245;
    int tempCursor = currentCursor;

    if(joystickY == up && tempCursor - 3 >= 0)
        return tempCursor - 3;

    else if(joystickY >= down && tempCursor + 3 <= 8)
        return tempCursor + 3;

    else
        return tempCursor;
}

//TODO: NO MORE TELEPORTING EDGES GING LEFT AND RIGHT.
int adjustHorizontal(int joystickX, int currentCursor) {
    int left = 245;
    int right = 0;
    int tempCursor = currentCursor;

    if(joystickX >= left && tempCursor - 1 >= 0 && tempCursor != 6 && tempCursor != 3)
        return currentCursor - 1;

    else if(joystickX == right && tempCursor + 1 <= 8 && tempCursor != 2 && tempCursor != 5)
        return currentCursor + 1;

    else
        return tempCursor;
}


