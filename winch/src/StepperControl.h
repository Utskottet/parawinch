#ifndef STEPPER_CONTROL_H
#define STEPPER_CONTROL_H

#include <M5Stack.h>
#include "LoRaComm.h"

class StepperControl {
public:
    StepperControl(LoRaComm& loraComm);
    void setup();
    void runStepper();
    void stopAndCenter();
    void update();  // Non-blocking update function
    void setPwmValue(int value);  // New function to adjust PWM value

private:
    enum MotorState { IDLE, RUNNING, CENTERING_TO_LIMIT, CENTERING_BACK };
    MotorState motorState = IDLE;

    LoRaComm& lora;  // Reference to LoRaComm instance
    const int pwmPin = 25;
    const int runStepperPin = 22;
    const int dirStepperPin = 21;
    const int limitSwitchPin = 35;

    int pwmFrequency = 20000;  // SPEED
    const int pwmChannel = 0;
    const int pwmResolution = 8;  // 8-bit resolution

    int pwmValue = 95;   //SPEED SETTING 0-255
    bool motorRunning = false;
    bool motorDirection = false;  // False for backward, true for forward
    bool limitHitDirection = false;  // Direction we were going when limit was hit

    unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
    const unsigned long debounceDelay = 50;  // the debounce time; increase if the output flickers

    unsigned long additionalRunTime = 7500;  // Time to run after hitting limit switch (in milliseconds)
    unsigned long centeringStartTime = 0;  // Start time for centering run

    void updatePWM(int value);
    void reverseFromLimit();
    bool isLimitPressed();
};

#endif
