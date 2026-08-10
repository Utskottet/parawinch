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

    // Diagnostic getters — read-only observers, safe to call from anywhere.
    uint8_t  diagState() const     { return (uint8_t)motorState; }
    uint32_t diagReversals() const { return reversalCount; }
    uint32_t diagLastRevMs() const { return lastReverseMs; }

private:
    enum MotorState { IDLE, RUNNING, CENTERING_TO_LIMIT, CENTERING_BACK };
    MotorState motorState = IDLE;
    uint32_t reversalCount = 0;
    uint32_t lastReverseMs = 0;

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
    bool switchReleasedSinceHit = true;  // gate re-triggering on the switch release edge

    unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
    const unsigned long debounceDelay = 50;  // the debounce time; increase if the output flickers

    unsigned long additionalRunTime = 7500;  // Time to run after hitting limit switch (in milliseconds)
    unsigned long centeringStartTime = 0;  // Start time for centering run

    void updatePWM(int value);
    void reverseFromLimit();
    bool isLimitPressed();
};

#endif
