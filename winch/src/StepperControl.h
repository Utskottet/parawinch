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
    void clearJam();  // operator reset for a latched limitJam fault

    // Stall watchdog. Call once per loop. Trips when no endswitch arrives
    // within a traverse plus margin while the carriage is being driven, which
    // means the level wind has stopped moving.
    void updateStallWatchdog();
    bool isStalled() const { return stallLatched; }
    void clearStall();

    // Diagnostic getters — read-only observers, safe to call from anywhere.
    uint8_t  diagState() const     { return (uint8_t)motorState; }
    uint32_t diagReversals() const { return reversalCount; }
    uint32_t diagLastRevMs() const { return lastReverseMs; }
    uint32_t diagRejected() const  { return rejectedCount; }
    bool     diagJam() const       { return limitJam; }
    uint8_t  diagWatchdog() const  { return stallLatched ? 2 : (stallBreaking ? 1 : 0); }
    uint32_t diagSaves() const     { return silentSaves; }

private:
    enum MotorState { IDLE, RUNNING, CENTERING_TO_LIMIT, CENTERING_BACK };
    MotorState motorState = IDLE;
    uint32_t reversalCount = 0;
    uint32_t lastReverseMs = 0;
    uint32_t rejectedCount = 0;   // presses thrown away by the lockout — should stay near 0

    LoRaComm& lora;  // Reference to LoRaComm instance
    const int pwmPin = 25;
    const int runStepperPin = 22;
    const int dirStepperPin = 21;
    // NOTE: GPIO 35 is input-only and has NO internal pull-up/pull-down (true of
    // GPIO 34-39 on every ESP32). INPUT_PULLUP is silently ignored here, so the
    // line MUST have an external pull-up to 3V3 — see setup().
    const int limitSwitchPin = 35;

    int pwmFrequency = 20000;  // SPEED
    const int pwmChannel = 0;
    const int pwmResolution = 8;  // 8-bit resolution

    int pwmValue = 95;   //SPEED SETTING 0-255
    bool motorRunning = false;
    bool motorDirection = false;  // False for backward, true for forward

    // --- Limit switch debounce ---------------------------------------------
    // Both endswitches share limitSwitchPin. A raw sample is only believed once
    // it has held the same value for debounceMs; a reversal fires only on a
    // confirmed released->pressed edge of that debounced level. This makes the
    // "one reversal per physical press" rule structural instead of relying on a
    // separate gate flag that switch bounce could reopen.
    bool limitStable      = false;  // debounced level: true = pressed
    bool limitLastSample  = false;  // last raw sample, pending confirmation
    bool limitEvent       = false;  // one-shot: confirmed press edge this poll
    bool limitPressPending = false; // press seen during lockout, fire when it expires
    unsigned long limitSampleChangedMs = 0;  // when limitLastSample last flipped
    unsigned long limitPressedSinceMs  = 0;  // when the debounced level went pressed

    const unsigned long debounceMs        = 25;    // sample must hold this long
    const unsigned long reversalLockoutMs = 400;   // no 2nd reversal inside this window
    const unsigned long stuckLimitMs      = 3000;  // held this long => jammed on the stop

    bool    limitJam      = false;  // latched fault: could not back off the switch
    uint8_t jamRecoveries = 0;      // forced reversals attempted while stuck

    // --- Stall watchdog -----------------------------------------------------
    // Endswitch-to-endswitch traverse measured 15.51 s on 2026-08-14. A stall
    // is invisible until the next hit fails to arrive, so worst case detection
    // is one traverse plus the margin. Fixed rather than learned: the carriage
    // runs at a constant PWM speed, so the period does not drift.
    const unsigned long traverseNominalMs = 15510;
    const unsigned long traverseMarginMs  = 1500;
    const unsigned long stallTripMs       = 17010;  // nominal + margin
    const unsigned long breakDwellMs      = 250;    // let the rotor settle
    const unsigned long breakConfirmMs    = 2000;   // agreed 2 s window
    const int recoveryPwmValue = 45;                // from 95
    const int recoveryPwmFreq  = 6000;              // from 20000

    bool     stallLatched   = false;  // hard fault: VESC must be stopped
    bool     stallBreaking  = false;  // break sequence in progress
    bool     breakRestarted = false;  // dwell finished, motor re-driven
    unsigned long breakStartMs   = 0;
    uint32_t breakRevAtStart     = 0; // reversalCount when the break began
    unsigned long watchArmedMs   = 0; // when the watchdog last became armed
    uint32_t silentSaves         = 0; // recoveries that saved the tow silently

    unsigned long additionalRunTime = 7500;  // Time to run after hitting limit switch (in milliseconds)
    unsigned long centeringStartTime = 0;  // Start time for centering run

    void updatePWM(int value);
    void reverseFromLimit();
    void pollLimit();   // single debounced sample per loop; sets limitEvent
    void restoreSpeed();  // undo the recovery PWM settings
};

#endif
