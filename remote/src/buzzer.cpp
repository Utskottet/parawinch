/**
 * buzzer.cpp - Buzzer implementation for XIAO nRF52840
 * Adapted from T3 remote: LEDC replaced with tone()/noTone().
 * All patterns identical to T3.
 */

#include "buzzer.h"
#include "utilities.h"

BuzzerController buzzer;

const uint16_t BuzzerController::stateBaseFrequency[7] = {
  2000,  // State 0
  2400,  // State 1
  2700,  // State 2
  3000,  // State 3
  3200,  // State 4 (resonant)
  3400,  // State 5
  3600   // State 6
};

const uint16_t BUTTON_BEEP_FREQ     = 3200;
const uint16_t BUTTON_BEEP_DURATION = 20;
const uint16_t SWEEP_DURATION       = 150;
const uint16_t TONE_END_DELAY       = 50;

BuzzerController::BuzzerController() :
  buzzerState(BUZZER_IDLE),
  buzzerEndTime(0),
  patternStepTime(0),
  patternStep(0),
  sweepStartFreq(0),
  sweepEndFreq(0),
  sweepCurrentFreq(0),
  sweepStep(0),
  sweepStepTimeMs(0),
  sweepLastStepTime(0),
  _seqState(0),
  _seqBeeping(false)
{}

void BuzzerController::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
  // Enable high drive mode (~50mA vs default ~10mA) for more volume
  NRF_P1->PIN_CNF[12] &= ~(7UL << 8);
  NRF_P1->PIN_CNF[12] |= (3UL << 8);
  noTone(BUZZER_PIN);
}

void BuzzerController::startTone(uint16_t frequency) {
  tone(BUZZER_PIN, frequency);
}

void BuzzerController::stopTone() {
  noTone(BUZZER_PIN);
}

void BuzzerController::playButtonPress() {
  if (buzzerState != BUZZER_IDLE && buzzerState != BUZZER_BEEP) return;
  buzzerState  = BUZZER_BEEP;
  buzzerEndTime = millis() + BUTTON_BEEP_DURATION;
  startTone(BUTTON_BEEP_FREQ);
}

void BuzzerController::playStateChange(uint8_t fromState, uint8_t toState) {
  fromState = (fromState > 6) ? 6 : fromState;
  toState   = (toState   > 6) ? 6 : toState;
  if (fromState == toState) return;

  if (toState == 1 && (fromState == 0 || fromState >= 2)) {
    playJumpToOne(); return;
  }
  if (toState == 0 && fromState == 1) {
    playJumpToZero(); return;
  }
  if (toState > fromState) {
    playUpwardSweep(stateBaseFrequency[fromState], stateBaseFrequency[toState], SWEEP_DURATION);
  } else {
    playDownwardSweep(stateBaseFrequency[fromState], stateBaseFrequency[toState], SWEEP_DURATION);
  }
}

void BuzzerController::playJumpToOne() {
  buzzerState   = BUZZER_PATTERN_TO_1;
  patternStep   = 0;
  patternStepTime = millis();
  startTone(3000);
}

void BuzzerController::playJumpToZero() {
  buzzerState   = BUZZER_PATTERN_TO_0;
  patternStep   = 0;
  patternStepTime = millis();
  startTone(3500);
}

void BuzzerController::playSleep() {
  // Obnoxious alarm: rapid 2800/800 Hz warble x8, then descending sweep
  buzzerState     = BUZZER_SLEEP;
  patternStep     = 0;
  patternStepTime = millis();
  startTone(2800);
}

void BuzzerController::playWake() {
  // Rising chime: 1200 → 2000 → 3500 Hz
  buzzerState     = BUZZER_WAKE;
  patternStep     = 0;
  patternStepTime = millis();
  startTone(1200);
}

void BuzzerController::setStateIndicator(uint8_t state) {
  _seqState = (state > 6) ? 6 : state;
  if (_seqState == 0) {
    _seqBeeping = false;
    if (buzzerState == BUZZER_IDLE) stopTone();
  }
}

void BuzzerController::update() {
  uint32_t now = millis();

  // ─── State indicator sequencer ────────────────────────────────────────────
  // 6 equal slots per 1000ms; slot=166ms, beep=120ms.
  // Slots 0..(state-1) active; rest silence. One-shot sounds override.
  if (_seqState > 0) {
    const uint32_t SLOT_MS = 166;
    const uint32_t BEEP_MS = 120;
    uint32_t posInCycle = now % 1000;
    uint8_t  slot       = (uint8_t)(posInCycle / SLOT_MS);
    if (slot > 5) slot  = 5;
    uint32_t posInSlot  = posInCycle % SLOT_MS;
    bool shouldBeep = (slot < _seqState) && (posInSlot < BEEP_MS);

    if (buzzerState == BUZZER_IDLE) {
      if (shouldBeep != _seqBeeping) {
        if (shouldBeep) startTone(stateBaseFrequency[_seqState]);
        else            stopTone();
        _seqBeeping = shouldBeep;
      }
    } else {
      _seqBeeping = false;  // one-shot playing — restart cleanly when done
    }
  }

  switch (buzzerState) {
    case BUZZER_IDLE:
      break;

    case BUZZER_BEEP:
      if (now >= buzzerEndTime) { stopTone(); buzzerState = BUZZER_IDLE; }
      break;

    case BUZZER_SWEEP_UP:
      if (now - sweepLastStepTime >= sweepStepTimeMs) {
        sweepLastStepTime = now;
        sweepCurrentFreq += sweepStep;
        if (sweepCurrentFreq >= sweepEndFreq) {
          tone(BUZZER_PIN, sweepEndFreq);
          buzzerState   = BUZZER_ENDING;
          buzzerEndTime = now + TONE_END_DELAY;
        } else {
          tone(BUZZER_PIN, sweepCurrentFreq);
        }
      }
      break;

    case BUZZER_SWEEP_DOWN:
      if (now - sweepLastStepTime >= sweepStepTimeMs) {
        sweepLastStepTime = now;
        sweepCurrentFreq += sweepStep; // sweepStep is negative
        if (sweepCurrentFreq <= sweepEndFreq) {
          tone(BUZZER_PIN, sweepEndFreq);
          buzzerState   = BUZZER_ENDING;
          buzzerEndTime = now + TONE_END_DELAY;
        } else {
          tone(BUZZER_PIN, sweepCurrentFreq);
        }
      }
      break;

    case BUZZER_PATTERN_TO_1:
      switch (patternStep) {
        case 0: if (now >= patternStepTime + 50) { startTone(1200); patternStep++; patternStepTime = now; } break;
        case 1: if (now >= patternStepTime + 50) { startTone(3000); patternStep++; patternStepTime = now; } break;
        case 2: if (now >= patternStepTime + 50) { startTone(1200); patternStep++; patternStepTime = now; } break;
        case 3: if (now >= patternStepTime + 80) { buzzerState = BUZZER_ENDING; buzzerEndTime = now + TONE_END_DELAY; } break;
      }
      break;

    case BUZZER_PATTERN_TO_0:
      if (now >= patternStepTime + 10) {
        patternStepTime = now;
        if (patternStep < 30) {
          float t = patternStep / 30.0f;
          uint16_t freq = 3500 - (uint16_t)(2700 * t * t);
          startTone(freq);
          patternStep++;
        } else {
          buzzerState   = BUZZER_ENDING;
          buzzerEndTime = now + TONE_END_DELAY;
        }
      }
      break;

    case BUZZER_SLEEP:
      // 8 warble steps (2800/800 Hz, 60 ms each) then descending sweep to 300 Hz
      switch (patternStep) {
        case 0: if (now >= patternStepTime + 60) { startTone(800);  patternStep++; patternStepTime = now; } break;
        case 1: if (now >= patternStepTime + 60) { startTone(2800); patternStep++; patternStepTime = now; } break;
        case 2: if (now >= patternStepTime + 60) { startTone(800);  patternStep++; patternStepTime = now; } break;
        case 3: if (now >= patternStepTime + 60) { startTone(2800); patternStep++; patternStepTime = now; } break;
        case 4: if (now >= patternStepTime + 60) { startTone(800);  patternStep++; patternStepTime = now; } break;
        case 5: if (now >= patternStepTime + 60) { startTone(2800); patternStep++; patternStepTime = now; } break;
        case 6: if (now >= patternStepTime + 60) { startTone(800);  patternStep++; patternStepTime = now; } break;
        case 7: if (now >= patternStepTime + 60) { playDownwardSweep(2800, 300, 500); } break;
      }
      break;

    case BUZZER_WAKE:
      // Rising chime: 1200 → gap → 2000 → gap → 3500 Hz
      switch (patternStep) {
        case 0: if (now >= patternStepTime + 70)  { stopTone();      patternStep++; patternStepTime = now; } break;
        case 1: if (now >= patternStepTime + 25)  { startTone(2000); patternStep++; patternStepTime = now; } break;
        case 2: if (now >= patternStepTime + 70)  { stopTone();      patternStep++; patternStepTime = now; } break;
        case 3: if (now >= patternStepTime + 25)  { startTone(3500); patternStep++; patternStepTime = now; } break;
        case 4: if (now >= patternStepTime + 130) { buzzerState = BUZZER_ENDING; buzzerEndTime = now + TONE_END_DELAY; } break;
      }
      break;

    case BUZZER_ENDING:
      if (now >= buzzerEndTime) { stopTone(); buzzerState = BUZZER_IDLE; }
      break;
  }
}

void BuzzerController::playUpwardSweep(uint16_t startFreq, uint16_t endFreq, uint16_t durationMs) {
  uint16_t freqDiff = endFreq - startFreq;
  uint8_t  numSteps = 20;
  sweepStartFreq    = startFreq;
  sweepEndFreq      = endFreq;
  sweepCurrentFreq  = startFreq;
  sweepStep         = freqDiff / numSteps;
  if (sweepStep < 1) sweepStep = 1;
  sweepStepTimeMs   = durationMs / numSteps;
  sweepLastStepTime = millis();
  buzzerState       = BUZZER_SWEEP_UP;
  startTone(sweepStartFreq);
}

void BuzzerController::playDownwardSweep(uint16_t startFreq, uint16_t endFreq, uint16_t durationMs) {
  uint16_t freqDiff = startFreq - endFreq;
  uint8_t  numSteps = 20;
  sweepStartFreq    = startFreq;
  sweepEndFreq      = endFreq;
  sweepCurrentFreq  = startFreq;
  sweepStep         = -((int16_t)freqDiff / numSteps);
  if (sweepStep > -1) sweepStep = -1;
  sweepStepTimeMs   = durationMs / numSteps;
  sweepLastStepTime = millis();
  buzzerState       = BUZZER_SWEEP_DOWN;
  startTone(sweepStartFreq);
}
