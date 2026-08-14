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

    // Give the driver a defined direction from the start. Previously dirStepperPin
    // was never written until the first reversal, so the first traverse ran on
    // whatever level the pin happened to power up at.
    digitalWrite(dirStepperPin, motorDirection ? HIGH : LOW);
    digitalWrite(runStepperPin, LOW);

    // Plain INPUT, not INPUT_PULLUP: GPIO 35 is input-only with no internal
    // pull resistor, so INPUT_PULLUP would just be a misleading no-op. The
    // switch line needs an external pull-up (4k7 to 3V3) plus a 100nF cap to
    // GND at the ESP32 end, otherwise it floats whenever both switches are
    // open and picks up noise from the stepper driver.
    pinMode(limitSwitchPin, INPUT);

    // Seed the debouncer with the current level so a line that is already
    // pressed at boot does not read as a fresh press edge.
    limitLastSample = (digitalRead(limitSwitchPin) == LOW);
    limitStable = limitLastSample;
    limitSampleChangedMs = millis();
    limitPressedSinceMs = millis();

    // No GPIO interrupt on the limit line. A latched edge flag turns a
    // microsecond noise glitch into an accepted press, and the carriage sits on
    // the switch for hundreds of milliseconds, so polling never misses a real
    // hit even when the display refresh stalls the loop.
}

void StepperControl::runStepper() {
    // A latched jam must survive. main.cpp calls runStepper() every loop while
    // the state is non-zero, so without this guard the jam handler's IDLE + run
    // pin LOW would be undone on the very next iteration and the motor would go
    // straight back to grinding into the end stop.
    if (limitJam || stallLatched) return;

    if (motorState != RUNNING) {
        motorState = RUNNING;
        motorRunning = true;
        digitalWrite(runStepperPin, HIGH);
    }
    // Limit handling lives solely in update() — polling here as well meant two
    // independent reversals could fire in a single loop iteration.
}

void StepperControl::stopAndCenter() {
    if (limitJam || stallLatched) return;  // same reason as runStepper()

    if (motorRunning) {
        motorRunning = false;
        motorState = CENTERING_TO_LIMIT;
        digitalWrite(runStepperPin, HIGH);  // Ensure motor is running
    }
}

// Operator reset after the carriage has been freed by hand. Clears the latch and
// re-seeds the debouncer from the current level so the switch the carriage may
// still be sitting on does not read as a fresh press.
void StepperControl::clearJam() {
    limitJam = false;
    jamRecoveries = 0;
    limitPressPending = false;
    limitLastSample = (digitalRead(limitSwitchPin) == LOW);
    limitStable = limitLastSample;
    limitSampleChangedMs = millis();
    limitPressedSinceMs = millis();
}

void StepperControl::update() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = 0;

    pollLimit();  // exactly one debounced sample per loop

    switch (motorState) {
        case RUNNING:
            if (limitEvent) {
                reverseFromLimit();
            }
            break;

        case CENTERING_TO_LIMIT:
            if (limitEvent) {
                reverseFromLimit();
                motorState = CENTERING_BACK;
                centeringStartTime = millis();
            }
            break;

        case CENTERING_BACK:
            elapsedTime = currentTime - centeringStartTime;
            if (elapsedTime < additionalRunTime) {
                if (limitEvent) {
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
    reversalCount++;
    lastReverseMs = millis();
}

void StepperControl::restoreSpeed() {
    ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
    ledcWrite(pwmChannel, pwmValue);
}

void StepperControl::clearStall() {
    stallLatched = false;
    stallBreaking = false;
    breakRestarted = false;
    restoreSpeed();
    watchArmedMs = millis();
    lastReverseMs = millis();  // give it a full traverse before judging again
}

// Trips when no endswitch arrives within a traverse plus margin while line is
// actually being wound — the carriage has stopped while the drum keeps filling.
// Integer-only and non-blocking: this runs every loop and must not disturb the
// limit poll. It never calls pollLimit(); it watches reversalCount instead.
void StepperControl::updateStallWatchdog() {
    const unsigned long now = millis();

    if (stallLatched) return;  // latched until clearStall()

    // Armed whenever the carriage is being driven in normal traversing mode.
    //
    // Deliberately NOT gated on the drum turning. The level wind runs at a
    // constant speed independent of the drum, so if it is being driven and has
    // not touched a switch in a traverse plus margin, something is wrong no
    // matter what the drum is doing. An earlier version gated on adjustedLineOut
    // changing, but that value is in whole metres: on a slow reel-in it ticks
    // over slower than the 2 s window, which re-armed the deadline continuously
    // and meant the watchdog could never fire.
    //
    // CENTERING_* are excluded: centring legitimately drives up to a full
    // traverse hunting for a switch, which would look identical to a stall.
    const bool shouldWatch = (motorState == RUNNING) && !limitJam;

    if (!shouldWatch) {
        watchArmedMs = now;  // hold the deadline out while disarmed
        if (stallBreaking) {
            stallBreaking = false;
            breakRestarted = false;
            restoreSpeed();
        }
        return;
    }

    // Deadline runs from the later of the last reversal and the moment the
    // watchdog armed, so a long idle period cannot cause an instant trip.
    const unsigned long ref = (lastReverseMs > watchArmedMs) ? lastReverseMs : watchArmedMs;

    if (!stallBreaking) {
        if (now - ref <= stallTripMs) return;

        // Overdue. Begin the break sequence: cut the pulses and let the rotor
        // settle before trying again.
        stallBreaking = true;
        breakRestarted = false;
        breakStartMs = now;
        breakRevAtStart = reversalCount;
        digitalWrite(runStepperPin, LOW);
        return;
    }

    // --- break sequence in progress ---

    if (reversalCount != breakRevAtStart) {
        // An endswitch arrived: the carriage is moving again. Silent save.
        stallBreaking = false;
        breakRestarted = false;
        restoreSpeed();
        silentSaves++;
        return;
    }

    if (!breakRestarted && (now - breakStartMs) >= breakDwellMs) {
        breakRestarted = true;
        // Reverse away from whatever it jammed against — driving further into
        // an obstruction can never clear it — and retry at low speed, where a
        // stepper makes far more torque. Both PWM parameters are lowered
        // because which one this driver honours is still unverified.
        motorDirection = !motorDirection;
        digitalWrite(dirStepperPin, motorDirection ? HIGH : LOW);
        ledcSetup(pwmChannel, recoveryPwmFreq, pwmResolution);
        ledcWrite(pwmChannel, recoveryPwmValue);
        digitalWrite(runStepperPin, HIGH);
        return;
    }

    if ((now - breakStartMs) >= (breakDwellMs + breakConfirmMs)) {
        // No endswitch inside the 2 s window. Latch, so main.cpp forces state 0
        // and the VESC is commanded to zero on the next CAN tick.
        stallBreaking = false;
        breakRestarted = false;
        restoreSpeed();
        stallLatched = true;
        digitalWrite(runStepperPin, LOW);
        motorRunning = false;
        motorState = IDLE;
    }
}

// Samples the shared endswitch line, debounces it, and raises limitEvent once
// per confirmed press. Also watches for a switch that never releases, which
// means the carriage has driven into the end stop and stalled.
void StepperControl::pollLimit() {
    limitEvent = false;

    const unsigned long now = millis();
    const bool raw = (digitalRead(limitSwitchPin) == LOW);  // LOW = pressed

    if (raw != limitLastSample) {
        // Unsettled — restart the stability timer and believe nothing yet.
        limitLastSample = raw;
        limitSampleChangedMs = now;
        return;
    }

    if (now - limitSampleChangedMs < debounceMs) {
        return;  // held, but not long enough to trust
    }

    if (raw == limitStable) {
        if (!limitStable) return;  // steady released — nothing to do

        // A press that arrived during the lockout is honoured as soon as the
        // lockout expires. Without this the edge would be swallowed and the
        // carriage would keep driving into the switch until the stuck-switch
        // watchdog fired three seconds later.
        if (limitPressPending && (now - lastReverseMs) >= reversalLockoutMs) {
            limitPressPending = false;
            limitEvent = true;
            return;
        }

        // Steady pressed. If it has been held far longer than a reversal needs
        // to clear it, the carriage is stalled on the stop. Only meaningful
        // while the motor is actually being driven — otherwise booting with the
        // carriage parked on a switch would latch a jam and kill the level wind.
        if (motorState != IDLE && (now - limitPressedSinceMs) > stuckLimitMs) {
            limitPressedSinceMs = now;  // re-arm before each attempt
            if (jamRecoveries < 3) {
                jamRecoveries++;
                reverseFromLimit();  // try to drive back off the switch
            } else if (!limitJam) {
                limitJam = true;
                digitalWrite(runStepperPin, LOW);  // give up rather than grind
                motorRunning = false;
                motorState = IDLE;
            }
        }
        return;
    }

    limitStable = raw;

    if (!limitStable) {
        // Released. A press edge cannot occur again until this happens, which
        // is what enforces one reversal per physical press.
        jamRecoveries = 0;
        limitPressPending = false;
        return;
    }

    // Confirmed press edge.
    limitPressedSinceMs = now;

    if (now - lastReverseMs < reversalLockoutMs) {
        // Too soon after the previous reversal to be a genuine second end.
        // Hold it rather than discard it — see limitPressPending above.
        rejectedCount++;
        limitPressPending = true;
        return;
    }

    limitEvent = true;
}
