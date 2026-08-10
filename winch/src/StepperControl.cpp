#include "StepperControl.h"

StepperControl::StepperControl(LoRaComm& loraComm) : lora(loraComm) {}

void StepperControl::setup() {
    M5.begin();
    Serial.begin(115200);  // Initialize serial monitor

    // Configure the PWM settings
    ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
    ledcAttachPin(pwmPin, pwmChannel);
    ledcWrite(pwmChannel, pwmValue);  // Set initial PWM duty cycle

    // Setup other GPIO pins
    pinMode(runStepperPin, OUTPUT);
    pinMode(dirStepperPin, OUTPUT);
    pinMode(limitSwitchPin, INPUT_PULLUP);
}

void StepperControl::runStepper() {
    if (motorState != RUNNING) {
        motorState = RUNNING;
        motorRunning = true;
        digitalWrite(runStepperPin, HIGH);
    }

    if (isLimitPressed()) {
        reverseFromLimit();
    }
}

void StepperControl::stopAndCenter() {
    if (motorRunning) {
        motorRunning = false;
        motorState = CENTERING_TO_LIMIT;
        digitalWrite(runStepperPin, HIGH);  // Ensure motor is running
    }
}

void StepperControl::update() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = 0;

    switch (motorState) {
        case RUNNING:
            if (isLimitPressed()) {
                reverseFromLimit();
            }
            break;

        case CENTERING_TO_LIMIT:
            if (isLimitPressed()) {
                reverseFromLimit();
                motorState = CENTERING_BACK;
                centeringStartTime = millis();
            }
            break;

        case CENTERING_BACK:
            elapsedTime = currentTime - centeringStartTime;
            if (elapsedTime < additionalRunTime) {
                if (isLimitPressed()) {
                    reverseFromLimit();
                }
            } else {
                motorState = IDLE;
                digitalWrite(runStepperPin, LOW);
                motorRunning = false;
            }
            break;

        case IDLE:
        default:
            break;
    }
}


void StepperControl::updatePWM(int value) {
    pwmValue = value;
    ledcWrite(pwmChannel, pwmValue);
}

void StepperControl::setPwmValue(int value) {
    updatePWM(value);
}

void StepperControl::reverseFromLimit() {
    // Record which direction caused the limit hit, then reverse
    limitHitDirection = motorDirection;
    motorDirection = !motorDirection;
    digitalWrite(dirStepperPin, motorDirection ? HIGH : LOW);
}

bool StepperControl::isLimitPressed() {
    bool pressed = (digitalRead(limitSwitchPin) == LOW);

    if (!pressed) {
        return false;
    }

    // Debounce
    if (millis() - lastDebounceTime < debounceDelay) {
        return false;
    }
    lastDebounceTime = millis();

    // If switch is pressed and we're ALREADY moving away from it, don't reverse again
    if (motorDirection != limitHitDirection) {
        return false;
    }

    return true;
}
