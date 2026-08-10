#include "StepperControl.h"

// FALLING-edge ISR flag — set the instant a limit is hit, so we never miss
// a press even if loop() is stalled in display refresh or CAN wait.
namespace {
    volatile bool g_limitInterruptFlag = false;
    void IRAM_ATTR limitSwitchISR() { g_limitInterruptFlag = true; }
}

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
    attachInterrupt(digitalPinToInterrupt(limitSwitchPin), limitSwitchISR, FALLING);
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
    motorDirection = !motorDirection;
    digitalWrite(dirStepperPin, motorDirection ? HIGH : LOW);
    switchReleasedSinceHit = false;
    reversalCount++;
    lastReverseMs = millis();
}

bool StepperControl::isLimitPressed() {
    bool pressed = (digitalRead(limitSwitchPin) == LOW);

    if (!pressed) {
        switchReleasedSinceHit = true;
    }

    // Consume ISR flag atomically. Either the ISR fired since last poll, or the
    // pin is currently LOW — both count as a hit worth acting on.
    noInterrupts();
    bool isrFired = g_limitInterruptFlag;
    g_limitInterruptFlag = false;
    interrupts();

    if (!(isrFired || pressed)) {
        return false;
    }

    if (!switchReleasedSinceHit) {
        return false;
    }

    if (millis() - lastDebounceTime < debounceDelay) {
        return false;
    }
    lastDebounceTime = millis();

    return true;
}
