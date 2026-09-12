/**
 * buzzer.h - Buzzer control for XIAO nRF52840
 * Uses tone()/noTone() instead of ESP32 LEDC.
 * Patterns and API identical to T3 remote.
 */

#pragma once
#include <Arduino.h>

// Buzzer states
enum BuzzerState {
  BUZZER_IDLE,
  BUZZER_BEEP,
  BUZZER_SWEEP_UP,
  BUZZER_SWEEP_DOWN,
  BUZZER_PATTERN_TO_1,
  BUZZER_PATTERN_TO_0,
  BUZZER_SLEEP,
  BUZZER_WAKE,
  BUZZER_ENDING
};

class BuzzerController {
public:
  BuzzerController();

  void begin();

  void playButtonPress();
  void playStateChange(uint8_t fromState, uint8_t toState);
  void playJumpToOne();
  void playJumpToZero();
  void playSleep();   // obnoxious alarm — device going to sleep
  void playWake();    // rising chime — device woke from sleep
  void setStateIndicator(uint8_t state);

  // Call regularly in main loop
  void update();

  bool isPlaying() const { return buzzerState != BUZZER_IDLE; }

private:
  void startTone(uint16_t frequency);
  void stopTone();
  void playUpwardSweep(uint16_t startFreq, uint16_t endFreq, uint16_t durationMs);
  void playDownwardSweep(uint16_t startFreq, uint16_t endFreq, uint16_t durationMs);

  BuzzerState buzzerState;
  uint32_t buzzerEndTime;
  uint32_t patternStepTime;
  uint8_t  patternStep;

  uint16_t sweepStartFreq;
  uint16_t sweepEndFreq;
  uint16_t sweepCurrentFreq;
  int16_t  sweepStep;
  uint8_t  sweepStepTimeMs;
  uint32_t sweepLastStepTime;

  uint8_t  _seqState;
  bool     _seqBeeping;

  static const uint16_t stateBaseFrequency[7];
};

extern BuzzerController buzzer;
