/**
 * selftest/main.cpp — Fork Ikoka Nano hardware bring-up firmware
 *
 * Standalone diagnostic build for the Ikoka Nano remote PCB
 * (XIAO nRF52840 Plus + EBYTE E22-900M30S). It shares nothing with the
 * remote firmware except utilities.h/lorastruct.h, so it can be used to
 * *verify* those before trusting them.
 *
 * Build:  pio run -e selftest --target upload
 * Drive:  any serial terminal at 115200, single-letter commands, '?' for help.
 *
 * It answers four questions the schematic alone cannot:
 *   1. does the E22 come up at all (boost rail, TCXO, SPI, BUSY)?
 *   2. which BSP pin is behind each silkscreen pad on the Plus (Seeed's own
 *      pinout diagram and BSP disagree about P1.03 vs P1.07)?
 *   3. does the RF switch work — i.e. does RXEN on D7 actually enable RX?
 *   4. how much better is the PA link than the Wio build, measured?
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

#include "../utilities.h"
#include "../lorastruct.h"

// ─── Radio ───────────────────────────────────────────────────────────────────
Module radioMod(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, SPI);
SX1262 lora(&radioMod);

static bool    radioUp      = false;
static int8_t  txPower      = LORA_OUTPUT_POWER_DBM;
static float   tcxoVolts    = LORA_TCXO_VOLTAGE_V;
static uint8_t rxenPin      = LORA_RXEN_PIN;
static bool    listening    = false;

volatile bool radioIRQ = false;
void dio1ISR() { radioIRQ = true; }

// ─── Pin candidates ──────────────────────────────────────────────────────────
// The Plus exposes nine extra pads, D11..D19 = BSP index 30..38. Seeed's
// published pinout diagram has two errors in this range; the BSP and the board
// schematic agree with each other. D11/D12/D13 are not disputed, P1.03 vs P1.07
// (silkscreen D17 vs D19) is.
struct PinCand { uint8_t bsp; const char* label; };
static const PinCand kExtraPins[] = {
  { 30, "pad D11 (P0.15)  J1-1 BUTTON_1" },
  { 31, "pad D12 (P0.19)  J1-2 BUTTON_2" },
  { 32, "pad D13 (P1.01)  J1-3 BUTTON_3" },
  { 33, "pad D14 (P0.09)  TP4   [NFC1]"  },
  { 34, "pad D15 (P0.10)  TP6   [NFC2]"  },
  { 35, "pad D16 (P0.31)  TP5   [VBAT]"  },
  { 36, "pad D19 (P1.07)  J4   PIEZO_B   <- BSP macro says D17" },
  { 37, "pad D18 (P1.05)  J4   PIEZO_A"  },
  { 38, "pad D17 (P1.03)  J1-4 BUTTON_4  <- BSP macro says D19" },
};
static const size_t kExtraCount = sizeof(kExtraPins) / sizeof(kExtraPins[0]);

// ─── Small serial helpers ────────────────────────────────────────────────────
static void hr() { Serial.println(F("--------------------------------------------------")); }

static void flushIn() { while (Serial.available()) Serial.read(); }

// Blocking y/n prompt. Returns true for yes.
static bool askYesNo(const char* question) {
  flushIn();
  Serial.printf("  %s [y/n] ", question);
  for (;;) {
    if (Serial.available()) {
      int c = Serial.read();
      if (c == 'y' || c == 'Y') { Serial.println(F("y")); return true;  }
      if (c == 'n' || c == 'N') { Serial.println(F("n")); return false; }
    }
    delay(5);
  }
}

static const char* portName(uint32_t nrfPin) { return (nrfPin < 32) ? "P0" : "P1"; }

// ═══════════════════════════════════════════════════════════════════════════
//  Test: pin map dump
// ═══════════════════════════════════════════════════════════════════════════
static void testPinMap() {
  hr();
  Serial.println(F("BSP PIN MAP  (what this build believes)"));
  Serial.printf("  PINS_COUNT       %d\n", PINS_COUNT);
  Serial.printf("  LORA  CS=D%-2d DIO1=D%-2d BUSY=D%-2d RST=D%-2d RXEN=D%-2d\n",
                LORA_CS_PIN, LORA_DIO1_PIN, LORA_BUSY_PIN, LORA_RST_PIN, LORA_RXEN_PIN);
  Serial.printf("  SPI   SCK=D%-2d MISO=D%-2d MOSI=D%-2d\n",
                LORA_SCK_PIN, LORA_MISO_PIN, LORA_MOSI_PIN);
  Serial.printf("  BTN   UP=%u DOWN=%u     BUZZER  tone=%u return=%u\n",
                BTN_UP_PIN, BTN_DN_PIN, BUZZER_PIN, BUZZER_GND_PIN);
  Serial.println(F("  Plus extra pads:"));
  for (size_t i = 0; i < kExtraCount; ++i) {
    uint32_t nrf = g_ADigitalPinMap[kExtraPins[i].bsp];
    Serial.printf("    BSP %-2u -> %s.%02lu   %s\n",
                  kExtraPins[i].bsp, portName(nrf), (unsigned long)(nrf & 31),
                  kExtraPins[i].label);
  }
  hr();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Test: button scan — press each button, see which pad answers
// ═══════════════════════════════════════════════════════════════════════════
static void testButtons() {
  hr();
  Serial.println(F("BUTTON SCAN — press each button in turn. Any key to stop."));
  Serial.println(F("Pads are pulled up; a press to J1-5 (GND) shows as LOW."));

  for (size_t i = 0; i < kExtraCount; ++i) pinMode(kExtraPins[i].bsp, INPUT_PULLUP);
  delay(5);

  bool prev[kExtraCount];
  for (size_t i = 0; i < kExtraCount; ++i) prev[i] = digitalRead(kExtraPins[i].bsp);

  flushIn();
  while (!Serial.available()) {
    for (size_t i = 0; i < kExtraCount; ++i) {
      bool now = digitalRead(kExtraPins[i].bsp);
      if (now != prev[i]) {
        prev[i] = now;
        Serial.printf("  %-6s  BSP %-2u  %s\n", now ? "RELEASE" : "PRESS",
                      kExtraPins[i].bsp, kExtraPins[i].label);
      }
    }
    delay(2);
  }
  flushIn();

  // Leave the piezo pads as outputs again so a later beep test behaves.
  pinMode(BUZZER_PIN, OUTPUT);      digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUZZER_GND_PIN, OUTPUT);  digitalWrite(BUZZER_GND_PIN, LOW);
  Serial.println(F("BUTTON SCAN done."));
  hr();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Test: piezo identification
//  The piezo bridges two pads through 100R per leg. Driving one leg only works
//  if the other leg is a hard ground, so floating each candidate in turn tells
//  us which BSP pin is really behind the J4 wiring.
// ═══════════════════════════════════════════════════════════════════════════
// Same high-drive trick buzzer.cpp uses (~50 mA vs the default ~10 mA).
// pinMode() rewrites PIN_CNF, so this has to come after it and before tone().
static void setHighDrive(uint8_t arduinoPin) {
  uint32_t nrfPin = g_ADigitalPinMap[arduinoPin];
  NRF_GPIO_Type* port = (nrfPin < 32) ? NRF_P0 : NRF_P1;
  uint32_t bit = nrfPin & 31;
  port->PIN_CNF[bit] &= ~(7UL << 8);
  port->PIN_CNF[bit] |=  (3UL << 8);   // H0H1
}

static void beep(uint8_t tonePin, int gndPin, int floatPin, uint16_t ms) {
  if (floatPin >= 0) pinMode(floatPin, INPUT);            // no pull: truly open
  if (gndPin   >= 0) {
    pinMode(gndPin, OUTPUT);
    setHighDrive(gndPin);
    digitalWrite(gndPin, LOW);
  }
  pinMode(tonePin, OUTPUT);
  setHighDrive(tonePin);
  // Warble rather than a flat tone -- far easier to pick out of a quiet room,
  // and it rules out "maybe that was something else".
  uint32_t end = millis() + ms;
  while ((int32_t)(millis() - end) < 0) {
    tone(tonePin, 3200); delay(120);
    tone(tonePin, 2000); delay(120);
  }
  noTone(tonePin);
  digitalWrite(tonePin, LOW);
}

static void testPiezo() {
  hr();
  Serial.println(F("PIEZO IDENTIFICATION"));
  Serial.println(F("Listen for a 3200 Hz tone after each prompt."));
  Serial.println(F("Candidates: BSP 37 = P1.05, BSP 36 = P1.07, BSP 38 = P1.03"));

  Serial.println(F("\nSTEP 1 — control: tone on BSP 37, BOTH 36 and 38 held LOW"));
  pinMode(36, OUTPUT); setHighDrive(36); digitalWrite(36, LOW);
  beep(37, 38, -1, 2400);
  bool step1 = askYesNo("heard it?");

  uint8_t tonePin = 37;
  if (!step1) {
    Serial.println(F("\n  BSP 37 is not the drive leg. Trying the others."));
    Serial.println(F("STEP 1b — tone on BSP 36, 37 and 38 LOW"));
    pinMode(37, OUTPUT); setHighDrive(37); digitalWrite(37, LOW);
    beep(36, 38, -1, 2400);
    if (askYesNo("heard it?")) { tonePin = 36; }
    else {
      Serial.println(F("STEP 1c — tone on BSP 38, 36 and 37 LOW"));
      pinMode(36, OUTPUT); setHighDrive(36); digitalWrite(36, LOW);
      beep(38, 37, -1, 2400);
      if (askYesNo("heard it?")) { tonePin = 38; }
      else {
        Serial.println(F("\n  RESULT: no tone on any candidate — check the J4 piezo"));
        Serial.println(F("          wiring and R4/R5 before trusting anything else."));
        hr();
        return;
      }
    }
  }

  // Now find the return leg: float one candidate at a time.
  uint8_t candA = (tonePin == 36) ? 37 : 36;
  uint8_t candB = (tonePin == 38) ? 37 : 38;

  Serial.printf("\nSTEP 2 — tone on BSP %u, BSP %u held LOW, BSP %u FLOATING\n",
                tonePin, candA, candB);
  beep(tonePin, candA, candB, 2400);
  bool aIsReturn = askYesNo("heard it?");

  Serial.printf("\nSTEP 3 — tone on BSP %u, BSP %u held LOW, BSP %u FLOATING\n",
                tonePin, candB, candA);
  beep(tonePin, candB, candA, 2400);
  bool bIsReturn = askYesNo("heard it?");

  hr();
  Serial.println(F("PIEZO RESULT"));
  if (aIsReturn && !bIsReturn) {
    Serial.printf("  drive leg  = BSP %u\n  return leg = BSP %u\n", tonePin, candA);
  } else if (bIsReturn && !aIsReturn) {
    Serial.printf("  drive leg  = BSP %u\n  return leg = BSP %u\n", tonePin, candB);
  } else if (aIsReturn && bIsReturn) {
    Serial.println(F("  Both candidates worked as the return leg. That should be"));
    Serial.println(F("  impossible with one floating — retry, listening carefully."));
  } else {
    Serial.println(F("  Neither candidate worked alone, but the control tone did."));
    Serial.println(F("  Retry; if it repeats, the return path is somewhere else."));
  }
  Serial.printf("  utilities.h currently says  BUZZER_PIN=%u  BUZZER_GND_PIN=%u\n",
                BUZZER_PIN, BUZZER_GND_PIN);
  hr();

  pinMode(BUZZER_PIN, OUTPUT);      digitalWrite(BUZZER_PIN, LOW);
  pinMode(BUZZER_GND_PIN, OUTPUT);  digitalWrite(BUZZER_GND_PIN, LOW);
}

// Replayable single configurations, so a leg can be re-tested as many times as
// it takes to be sure. Drive is always BSP 37 (P1.05, confirmed on hardware);
// the question is only which pin completes the circuit.
static void beepConfig(char which) {
  switch (which) {
    case '1':
      Serial.println(F("  [1] tone BSP 37 | BSP 36 LOW | BSP 38 FLOATING"));
      Serial.println(F("      sound here => the return leg is BSP 36 (P1.07)"));
      beep(37, 36, 38, 2400);
      break;
    case '2':
      Serial.println(F("  [2] tone BSP 37 | BSP 38 LOW | BSP 36 FLOATING"));
      Serial.println(F("      sound here => the return leg is BSP 38 (P1.03)"));
      beep(37, 38, 36, 2400);
      break;
    case '3':
      Serial.println(F("  [3] control: tone BSP 37 | BSP 36 and 38 both LOW"));
      pinMode(36, OUTPUT); setHighDrive(36); digitalWrite(36, LOW);
      beep(37, 38, -1, 2400);
      break;
  }
  Serial.println(F("      done"));
}

// Play the remote's own button beep through the configured pins.
static void testBeepConfigured() {
  Serial.printf("Beeping on BUZZER_PIN=%u with BUZZER_GND_PIN=%u held LOW...\n",
                BUZZER_PIN, BUZZER_GND_PIN);
  beep(BUZZER_PIN, BUZZER_GND_PIN, -1, 1600);
  Serial.println(F("done"));
}

// ═══════════════════════════════════════════════════════════════════════════
//  Test: E22 bring-up and SPI wiring
// ═══════════════════════════════════════════════════════════════════════════
static void decodeDeviceErrors(uint16_t e) {
  Serial.printf("  device errors 0x%04X", e);
  if (e == 0) { Serial.println(F("  (none)")); return; }
  Serial.print(F("  ->"));
  if (e & 0x0001) Serial.print(F(" RC64K_CALIB"));
  if (e & 0x0002) Serial.print(F(" RC13M_CALIB"));
  if (e & 0x0004) Serial.print(F(" PLL_CALIB"));
  if (e & 0x0008) Serial.print(F(" ADC_CALIB"));
  if (e & 0x0010) Serial.print(F(" IMG_CALIB"));
  if (e & 0x0020) Serial.print(F(" XOSC_START(TCXO!)"));
  if (e & 0x0040) Serial.print(F(" PLL_LOCK"));
  if (e & 0x0100) Serial.print(F(" PA_RAMP"));
  Serial.println();
}

static bool radioInit(bool verbose) {
  if (verbose) {
    hr();
    Serial.println(F("E22 BRING-UP"));
  }

  // 1. Rail off: RST low also disables the MT3608B boost (EN tied to reset).
  pinMode(LORA_RST_PIN, OUTPUT);
  digitalWrite(LORA_RST_PIN, LOW);
  pinMode(LORA_RXEN_PIN, OUTPUT);
  digitalWrite(LORA_RXEN_PIN, LOW);
  pinMode(LORA_BUSY_PIN, INPUT);
  delay(50);
  if (verbose) Serial.printf("  rail OFF (RST low): BUSY reads %s\n",
                             digitalRead(LORA_BUSY_PIN) ? "HIGH" : "LOW");

  // 2. Rail on, time the module's boot.
  uint32_t t0 = millis();
  digitalWrite(LORA_RST_PIN, HIGH);
  while (digitalRead(LORA_BUSY_PIN) && (millis() - t0) < 500) { /* spin */ }
  uint32_t busyMs = millis() - t0;
  if (verbose) {
    if (busyMs >= 500) Serial.println(F("  rail ON:  BUSY never went low  <-- FAIL (boost? module?)"));
    else               Serial.printf("  rail ON:  BUSY low after %lu ms\n", (unsigned long)busyMs);
  }

  // 3. SPI + RadioLib.
  SPI.begin();
  pinMode(LORA_CS_PIN, OUTPUT);
  digitalWrite(LORA_CS_PIN, HIGH);
  delay(LORA_BOOT_SETTLE_MS);

  int16_t err = lora.begin(LORA_FREQUENCY_MHZ, LORA_BANDWIDTH_KHZ,
                           LORA_SPREADING_FACTOR, LORA_CODING_RATE,
                           RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
                           txPower, 8, tcxoVolts);
  if (verbose) Serial.printf("  begin(tcxo=%.1fV) -> %d %s\n", tcxoVolts, err,
                             err == RADIOLIB_ERR_NONE ? "OK" : "FAIL");
  if (err != RADIOLIB_ERR_NONE) { radioUp = false; return false; }

  lora.setBandwidth(LORA_BANDWIDTH_KHZ);
  lora.setSpreadingFactor(LORA_SPREADING_FACTOR);
  lora.setCodingRate(LORA_CODING_RATE);
  lora.setOutputPower(txPower);
  lora.setCurrentLimit(LORA_CURRENT_LIMIT_MA);
  lora.setDio2AsRfSwitch(true);
  lora.setRfSwitchPins(rxenPin, RADIOLIB_NC);
  lora.setDio1Action(dio1ISR);

  if (verbose) {
    // 4. SPI read/write proof: the sync word registers must read back what
    //    RadioLib just wrote, and a scribble must stick and then revert.
    uint8_t sync[2] = {0, 0};
    lora.readRegister(RADIOLIB_SX126X_REG_LORA_SYNC_WORD_MSB, sync, 2);
    Serial.printf("  sync word regs  0x%02X 0x%02X  (expect 0x14 0x24 = private)\n",
                  sync[0], sync[1]);

    uint8_t scribble[2] = {0xA5, 0x5A}, back[2] = {0, 0};
    lora.writeRegister(RADIOLIB_SX126X_REG_LORA_SYNC_WORD_MSB, scribble, 2);
    lora.readRegister(RADIOLIB_SX126X_REG_LORA_SYNC_WORD_MSB, back, 2);
    lora.writeRegister(RADIOLIB_SX126X_REG_LORA_SYNC_WORD_MSB, sync, 2);
    bool spiOk = (back[0] == 0xA5 && back[1] == 0x5A);
    Serial.printf("  SPI write/read   0x%02X 0x%02X  %s\n", back[0], back[1],
                  spiOk ? "OK (MOSI+MISO+SCK+NSS all good)" : "FAIL");

    Serial.printf("  status byte      0x%02X\n", lora.getStatus());
    decodeDeviceErrors(lora.getDeviceErrors());
    Serial.printf("  config           %.1f MHz  SF%d  BW%.0f  CR4/%d  %d dBm SX1262\n",
                  LORA_FREQUENCY_MHZ, LORA_SPREADING_FACTOR, LORA_BANDWIDTH_KHZ,
                  LORA_CODING_RATE, txPower);
    Serial.printf("                   -> roughly %d dBm at the antenna (PA +7.25 dB)\n",
                  txPower + 7);
    Serial.printf("  RF switch        RXEN=D%d via MCU, TXEN via module DIO2\n", rxenPin);
    hr();
  }

  lora.startReceive();
  radioUp = true;
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Test: link to the winch / simulator
// ═══════════════════════════════════════════════════════════════════════════
static uint8_t pingSeq = 0;
static int16_t  g_lastTxErr = 0;
static uint32_t g_lastTxMs  = 0;

// Sends CmdPacket state 0 (STOP — safe against the real winch too) and waits
// for the 0x03 ACK. Returns round-trip ms, or -1 on timeout.
static int32_t pingOnce(float* rssi, float* snr) {
  CmdPacket pkt{};
  pkt.type  = 0x01;
  pkt.seq   = ++pingSeq;
  pkt.state = 0;

  radioIRQ = false;
  uint32_t t0 = millis();
  g_lastTxErr = lora.transmit(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));
  g_lastTxMs  = millis() - t0;
  lora.startReceive();

  uint32_t deadline = millis() + ACK_TIMEOUT_MS * 4;   // generous for a lab test
  while ((int32_t)(millis() - deadline) < 0) {
    if (radioIRQ) {
      radioIRQ = false;
      uint8_t buf[32];
      size_t  len = sizeof(buf);
      if (lora.readData(buf, len) == RADIOLIB_ERR_NONE && len) {
        if (buf[0] == 0x03 && len >= sizeof(AckPacket) &&
            reinterpret_cast<AckPacket*>(buf)->seq == pkt.seq) {
          if (rssi) *rssi = lora.getRSSI();
          if (snr)  *snr  = lora.getSNR();
          return (int32_t)(millis() - t0);
        }
      }
      lora.startReceive();
    }
  }
  lora.startReceive();
  return -1;
}

static void testPing(uint16_t count) {
  if (!radioUp) { Serial.println(F("radio not up — run 'r' first")); return; }
  hr();
  Serial.printf("LINK TEST — %u pings to the winch/sim at %d dBm SX1262 (~%d dBm ANT)\n",
                count, txPower, txPower + 7);

  uint16_t ok = 0;
  float    rssiSum = 0, snrSum = 0;
  int32_t  rttSum = 0, rttWorst = 0;

  for (uint16_t i = 0; i < count; ++i) {
    float rssi = 0, snr = 0;
    int32_t rtt = pingOnce(&rssi, &snr);
    if (rtt >= 0) {
      ok++; rssiSum += rssi; snrSum += snr; rttSum += rtt;
      if (rtt > rttWorst) rttWorst = rtt;
      Serial.printf("  %3u  ACK  %4ld ms  RSSI %6.1f dBm  SNR %5.1f dB\n",
                    i + 1, (long)rtt, rssi, snr);
    } else {
      Serial.printf("  %3u  ---  no ACK   (tx code %d, tx took %lu ms)\n",
                    i + 1, g_lastTxErr, (unsigned long)g_lastTxMs);
    }
    delay(150);
  }

  hr();
  Serial.printf("  %u/%u ACKed (%.0f%%)\n", ok, count, count ? ok * 100.0f / count : 0.0f);
  if (ok) {
    Serial.printf("  mean RSSI %.1f dBm   mean SNR %.1f dB\n", rssiSum / ok, snrSum / ok);
    Serial.printf("  mean RTT  %ld ms     worst %ld ms\n", (long)(rttSum / ok), (long)rttWorst);
    Serial.println(F("  (RSSI here is the far end's reply, so it reads the other"));
    Serial.println(F("   node's TX through this board's LNA — the receive half."));
    Serial.println(F("   The far end's own log shows what the PA is doing.)"));
  }
  hr();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Test: does TX actually complete? (needs no peer)
//  transmit() returns TX_TIMEOUT if the module never raises TxDone, which is
//  what a supply collapse mid-burst looks like. PA_RAMP in the device errors
//  means the PA never ramped at all.
// ═══════════════════════════════════════════════════════════════════════════
static void testTxBurst(uint8_t count) {
  if (!radioUp) { Serial.println(F("radio not up -- run 'r' first")); return; }
  hr();
  Serial.printf("TX BURST -- %u transmits at %d dBm SX1262 (~%d dBm ANT)\n",
                count, txPower, txPower + 7);
  Serial.println(F("No peer needed: this only asks whether OUR transmit finished."));
  Serial.println(F("  code 0  = TxDone raised, the burst completed"));
  Serial.println(F("  code -5 = TX_TIMEOUT, no TxDone -- supply sag or PA fault"));

  CmdPacket pkt{};
  pkt.type = 0x01; pkt.state = 0;
  uint8_t ok = 0;
  for (uint8_t i = 0; i < count; ++i) {
    pkt.seq = ++pingSeq;
    uint32_t t0 = millis();
    int16_t err = lora.transmit(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));
    uint32_t ms = millis() - t0;
    uint16_t de = lora.getDeviceErrors();
    if (err == RADIOLIB_ERR_NONE) ok++;
    Serial.printf("  %2u  code %-3d  %3lu ms  status 0x%02X  devErr 0x%04X%s\n",
                  i + 1, err, (unsigned long)ms, lora.getStatus(), de,
                  (de & 0x0100) ? "  <-- PA_RAMP" : "");
    delay(200);
  }
  lora.startReceive();
  hr();
  Serial.printf("  %u/%u transmits completed\n", ok, count);
  if (ok == count) {
    Serial.println(F("  Our TX path is fine. Airtime at SF9 for a 3-byte payload is"));
    Serial.println(F("  about 107 ms, and every burst raised TxDone -- so the 5 V rail"));
    Serial.println(F("  held up. Silence on the link is the far end, not us."));
  } else {
    Serial.println(F("  TX is NOT completing. Retry after 'p5' (low power) -- if low"));
    Serial.println(F("  power completes and full power does not, the supply cannot"));
    Serial.println(F("  carry the PA burst: use a battery, not just USB."));
  }
  hr();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Test: RF switch — does RXEN on D7 actually gate the LNA?
// ═══════════════════════════════════════════════════════════════════════════
static void testRfSwitch() {
  if (!radioUp) { Serial.println(F("radio not up — run 'r' first")); return; }
  hr();
  Serial.println(F("RF SWITCH TEST"));
  Serial.println(F("Sends 5 pings with RXEN driven normally, then 5 with RXEN"));
  Serial.println(F("forced LOW (RX path disabled). The second set must fail —"));
  Serial.println(F("if both sets pass, RXEN is not the pin we think it is."));

  Serial.println(F("\n  [A] RXEN under RadioLib control:"));
  uint8_t okA = 0;
  for (int i = 0; i < 5; ++i) if (pingOnce(nullptr, nullptr) >= 0) okA++;
  Serial.printf("      %u/5 ACKed\n", okA);

  Serial.println(F("  [B] RXEN pinned LOW:"));
  lora.setRfSwitchPins(RADIOLIB_NC, RADIOLIB_NC);
  pinMode(rxenPin, OUTPUT);
  digitalWrite(rxenPin, LOW);
  uint8_t okB = 0;
  for (int i = 0; i < 5; ++i) {
    digitalWrite(rxenPin, LOW);
    if (pingOnce(nullptr, nullptr) >= 0) okB++;
  }
  Serial.printf("      %u/5 ACKed\n", okB);

  lora.setRfSwitchPins(rxenPin, RADIOLIB_NC);
  lora.startReceive();

  hr();
  if (okA > 0 && okB == 0)      Serial.printf("  PASS — D%d is RXEN and the switch works.\n", rxenPin);
  else if (okA > 0 && okB > 0)  Serial.printf("  INCONCLUSIVE — RX works with D%d low. Either it is not\n"
                                              "  RXEN, or the link is strong enough to leak through.\n"
                                              "  Retry with the nodes further apart or at lower power.\n", rxenPin);
  else                          Serial.println(F("  FAIL — no ACKs even with RXEN driven. Fix the link first."));
  hr();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Test: listen / carrier / battery
// ═══════════════════════════════════════════════════════════════════════════
static void serviceListen() {
  if (!radioIRQ) return;
  radioIRQ = false;
  uint8_t buf[40];
  size_t  len = sizeof(buf);
  int16_t st  = lora.readData(buf, len);
  if (st == RADIOLIB_ERR_NONE && len) {
    Serial.printf("  RX type 0x%02X  %2u B  RSSI %6.1f  SNR %5.1f  |",
                  buf[0], (unsigned)len, lora.getRSSI(), lora.getSNR());
    for (size_t i = 0; i < len && i < 16; ++i) Serial.printf(" %02X", buf[i]);
    if (buf[0] == 0x02 && len >= sizeof(MetricsPacket)) {
      auto* m = reinterpret_cast<MetricsPacket*>(buf);
      Serial.printf("   metrics seq:%u amps:%.1f dist:%u", m->seq,
                    m->amps_x10 / 10.0f, m->distance_m);
    }
    Serial.println();
  } else if (st == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println(F("  RX CRC error"));
  }
  lora.startReceive();
}

static void testCarrier(uint16_t secs) {
  if (!radioUp) { Serial.println(F("radio not up — run 'r' first")); return; }
  Serial.printf("CW CARRIER on %.1f MHz for %u s at %d dBm SX1262 (~%d dBm ANT).\n",
                LORA_FREQUENCY_MHZ, secs, txPower, txPower + 7);
  Serial.println(F("ANTENNA MUST BE FITTED. Measure with an SDR or power meter."));
  lora.transmitDirect();
  uint32_t end = millis() + secs * 1000UL;
  while ((int32_t)(millis() - end) < 0) delay(10);
  lora.standby();
  lora.startReceive();
  Serial.println(F("carrier off"));
}

static void testBattery() {
  pinMode(VBAT_ENABLE, OUTPUT);
  digitalWrite(VBAT_ENABLE, LOW);
  pinMode(22, OUTPUT); digitalWrite(22, LOW);   // HICHG: 100 mA
  pinMode(23, INPUT);                           // ~CHG:  LOW = charging
  delay(10);
  int   raw = analogRead(PIN_VBAT);
  float v   = raw * 3.6f / 1023.0f * (1510.0f / 510.0f);
  Serial.printf("  VBAT raw %d -> %.2f V   charging: %s\n",
                raw, v, digitalRead(23) ? "no" : "yes");
}

// ═══════════════════════════════════════════════════════════════════════════
//  Menu
// ═══════════════════════════════════════════════════════════════════════════
static void help() {
  hr();
  Serial.println(F("IKOKA NANO SELF-TEST"));
  Serial.println(F("  i  pin map this build is using"));
  Serial.println(F("  b  button scan   (press each button, any key to stop)"));
  Serial.println(F("  z  piezo identification  (interactive, answers y/n)"));
  Serial.println(F("  a  beep on the configured buzzer pins"));
  Serial.println(F("  1  piezo: 36 LOW, 38 floating   (repeatable)"));
  Serial.println(F("  2  piezo: 38 LOW, 36 floating   (repeatable)"));
  Serial.println(F("  3  piezo: both LOW  -- control  (repeatable)"));
  Serial.println(F("  r  E22 bring-up: rail, BUSY, SPI, TCXO, registers"));
  Serial.println(F("  d  dump status + device errors"));
  Serial.println(F("  t  one ping to the winch/sim"));
  Serial.println(F("  T  20 pings, with loss/RSSI/RTT summary"));
  Serial.println(F("  x  RF switch test (proves RXEN)"));
  Serial.println(F("  w  TX burst -- does OUR tx complete? (no peer needed)"));
  Serial.println(F("  l  listen mode toggle — dumps every packet heard"));
  Serial.println(F("  c  5 s CW carrier   (ANTENNA REQUIRED)"));
  Serial.println(F("  v  battery voltage"));
  Serial.println(F("  p<n>  set SX1262 power, dBm  (e.g. p10, p22)"));
  Serial.println(F("  k<n>  set TCXO volts x10     (k16 = 1.6 V, k18 = 1.8 V)"));
  Serial.println(F("  A  run the full non-interactive sequence"));
  hr();
}

static void runAll() {
  testPinMap();
  radioInit(true);
  testBattery();
  if (radioUp) { testPing(10); testRfSwitch(); }
  Serial.println(F("\nStill to do by hand:  'b' buttons,  'z' piezo."));
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 4000) delay(10);
  delay(200);

  Serial.println();
  hr();
  Serial.println(F("Fork Ikoka Nano — hardware self-test"));
  Serial.println(F("XIAO nRF52840 Plus + EBYTE E22-900M30S (SX1262 + PA + LNA)"));
  Serial.println(F("*** FIT THE ANTENNA BEFORE ANY TX COMMAND — this board puts"));
  Serial.println(F("*** ~1 W into the connector and the PA does not like an open."));
  help();
  Serial.print(F("> "));
}

void loop() {
  if (listening) serviceListen();

  if (!Serial.available()) return;
  int c = Serial.read();
  if (c == '\r' || c == '\n' || c == ' ') return;

  // Commands with a numeric argument read the rest of the line.
  int arg = -1;
  if (c == 'p' || c == 'k') {
    char num[8] = {0};
    uint8_t n = 0;
    uint32_t deadline = millis() + 300;
    while ((int32_t)(millis() - deadline) < 0 && n < sizeof(num) - 1) {
      if (!Serial.available()) continue;
      int d = Serial.read();
      if (d < '0' || d > '9') break;
      num[n++] = (char)d;
      deadline = millis() + 100;
    }
    if (n) arg = atoi(num);
  }

  Serial.println((char)c);
  switch (c) {
    case '?': case 'h': help(); break;
    case 'i': testPinMap(); break;
    case 'b': testButtons(); break;
    case 'z': testPiezo(); break;
    case 'a': testBeepConfigured(); break;
    case '1': case '2': case '3': beepConfig((char)c); break;
    case 'r': radioInit(true); break;
    case 'd':
      if (!radioUp) { Serial.println(F("radio not up")); break; }
      Serial.printf("  status 0x%02X\n", lora.getStatus());
      decodeDeviceErrors(lora.getDeviceErrors());
      break;
    case 't': testPing(1);  break;
    case 'T': testPing(20); break;
    case 'x': testRfSwitch(); break;
    case 'w': testTxBurst(8); break;
    case 'l':
      listening = !listening;
      Serial.println(listening ? F("listening — press 'l' again to stop")
                               : F("listen off"));
      if (listening && radioUp) lora.startReceive();
      break;
    case 'c': testCarrier(5); break;
    case 'v': testBattery(); break;
    case 'p':
      if (arg >= 0 && arg <= 22) {
        txPower = (int8_t)arg;
        if (radioUp) lora.setOutputPower(txPower);
        Serial.printf("  SX1262 power = %d dBm  (~%d dBm at the antenna)\n",
                      txPower, txPower + 7);
      } else Serial.println(F("  usage: p0 .. p22"));
      break;
    case 'k':
      if (arg >= 10 && arg <= 33) {
        tcxoVolts = arg / 10.0f;
        Serial.printf("  TCXO = %.1f V — run 'r' to re-init with it\n", tcxoVolts);
      } else Serial.println(F("  usage: k16, k18, k22 ... (volts x10)"));
      break;
    case 'A': runAll(); break;
    default:  Serial.println(F("  ? for help")); break;
  }
  Serial.print(F("> "));
}
