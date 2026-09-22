#pragma once
/*
 * lora_init.h — E22-900M30S bring-up helper (Fork Ikoka Nano)
 *
 * On this board SX126X_RESET doubles as the MT3608B boost enable, so the E22 is
 * completely unpowered until the reset line is released. RadioLib's begin()
 * performs its own ~1 ms reset pulse, which removes that power again mid-way;
 * on this hardware the module frequently will not answer the version-string
 * read afterwards and begin() returns -2 (CHIP_NOT_FOUND). Measured on the
 * bench board: the first attempt usually fails, and a real power cycle plus a
 * settle delay makes the next one succeed every time.
 *
 * So: retry begin() with a proper power cycle (RST low ~50 ms -> boost off,
 * high -> boost on, then let the rail settle) instead of a single shot.
 */
#include <Arduino.h>
#include <RadioLib.h>
#include "utilities.h"
#include "lorastruct.h"

#define LORA_RETRY_POWER_OFF_MS  50
#define LORA_RETRY_SETTLE_MS    200
#define LORA_BEGIN_ATTEMPTS       5

inline int16_t loraBeginWithRetry(SX1262& radio,
                                  int8_t  power       = LORA_OUTPUT_POWER_DBM,
                                  float   tcxoVolts   = LORA_TCXO_VOLTAGE_V,
                                  uint8_t maxAttempts = LORA_BEGIN_ATTEMPTS,
                                  bool    verbose     = false) {
  int16_t err = RADIOLIB_ERR_UNKNOWN;

  for (uint8_t attempt = 1; attempt <= maxAttempts; ++attempt) {
    err = radio.begin(LORA_FREQUENCY_MHZ, LORA_BANDWIDTH_KHZ,
                      LORA_SPREADING_FACTOR, LORA_CODING_RATE,
                      RADIOLIB_SX126X_SYNC_WORD_PRIVATE, power, 8, tcxoVolts);
    if (verbose) {
      Serial.printf("  begin(tcxo=%.1fV) attempt %u/%u -> %d %s\n",
                    tcxoVolts, attempt, maxAttempts, err,
                    err == RADIOLIB_ERR_NONE ? "OK" : "FAIL");
    }
    if (err == RADIOLIB_ERR_NONE) return err;

    // Full power cycle of the E22 rail (RST is the boost EN) and settle.
    pinMode(LORA_RST_PIN, OUTPUT);
    digitalWrite(LORA_RST_PIN, LOW);
    delay(LORA_RETRY_POWER_OFF_MS);
    digitalWrite(LORA_RST_PIN, HIGH);
    delay(LORA_RETRY_SETTLE_MS);
  }

  return err;
}
