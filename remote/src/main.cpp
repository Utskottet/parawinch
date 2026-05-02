#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <Button2.h>
#include <nrf_gpio.h>
#include "utilities.h"
#include "lorastruct.h"
#include "buzzer.h"
#include "ble_interface.h"

#define DEBUG
#ifdef DEBUG
  #define DBG(x)          Serial.println(x)
  #define DBGF(fmt, ...)  Serial.printf((fmt "\n"), ##__VA_ARGS__)
#else
  #define DBG(x)
  #define DBGF(fmt, ...)
#endif

// ─── LoRa ────────────────────────────────────────────────────────────────────
Module    radioMod(LORA_CS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, SPI);
SX1262    lora(&radioMod);
static bool loraReady = false;

volatile bool radioIRQ = false;
void dio1ISR() { radioIRQ = true; }   // no IRAM_ATTR on nRF52840

// ─── Packet loss tracking (20-second window, same as T3) ─────────────────────
static uint8_t  lastMetricsSeq          = 0;
static uint16_t metricsReceivedThisWindow = 0;
static uint16_t metricsLostThisWindow   = 0;
static float    packetLossPercent       = 0.0f;
static uint32_t windowStartTime        = 0;
const  uint32_t PACKET_LOSS_WINDOW_MS  = 20000;

// ─── State tracking (T3 convention) ──────────────────────────────────────────
uint8_t txSeq          = 0;
uint8_t awaitingSeq    = 0;
uint8_t desiredState   = 0;
uint8_t confirmedState = 0;

volatile bool metricsDirty = false;
uint32_t buttonEventCounter = 0;

// ─── Deep sleep ───────────────────────────────────────────────────────────────
static const uint32_t SLEEP_TIMEOUT_MS = 30UL * 60UL * 1000UL;  // 30 minutes
static uint32_t lastActivityTime       = 0;

void enterDeepSleep() {
  DBGF("[SLEEP] Entering System OFF — %lu ms idle", millis() - lastActivityTime);
  Serial.flush();

  // Play obnoxious sleep alarm and block until it finishes
  buzzer.playSleep();
  while (buzzer.isPlaying()) {
    buzzer.update();
    delay(1);
  }
  noTone(BUZZER_PIN);

  // Put LoRa to sleep (prevents ~4 mA draw in System OFF)
  if (loraReady) lora.sleep();

  // Configure both buttons as wake sources (active LOW, pullup already on)
  nrf_gpio_cfg_sense_input(g_ADigitalPinMap[BTN_UP_PIN], NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
  nrf_gpio_cfg_sense_input(g_ADigitalPinMap[BTN_DN_PIN], NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

  // Enter System OFF — never returns; wake = full reset via setup()
  sd_power_system_off();
}

// ─── DOWN button state ────────────────────────────────────────────────────────
struct SimpleButtonState {
  bool     lastRawState;
  bool     lastStableState;
  uint32_t lastChangeTime;
  uint32_t pressStartTime;
  uint32_t lastClickTime;
  uint32_t clickCount;
  bool     isPressed;
  bool     pressConsumed;
} downBtn = {true, true, 0, 0, 0, 0, false, false};

const uint32_t SIMPLE_DEBOUNCE_MS  = 20;
const uint32_t MIN_PRESS_MS        = 30;
const uint32_t DOUBLE_CLICK_WINDOW = 300;
const uint32_t MULTI_CLICK_WINDOW  = 800;

// ─── UP button ────────────────────────────────────────────────────────────────
Button2 btnUp(BTN_UP_PIN);

// ─── Forward declarations ─────────────────────────────────────────────────────
bool sendCmd(uint8_t st);
void processDownButton();
void processMetricsPacket(const MetricsPacket* m);

// ─── Remote battery (XIAO nRF52840) ──────────────────────────────────────────
// PIN_VBAT  = 32 = P0.31  battery sense (1MΩ:510kΩ divider output)
// pin 14    = P0.14       enable divider, active LOW
// pin 23    = P0.23       charge state, LOW = charging
// pin 22    = P0.22       charge current, LOW=100mA HIGH=50mA
#define VBAT_ENABLE_PIN  14   // P0.14, OUTPUT LOW to connect divider
#define CHG_STATE_PIN    23   // P0.23, INPUT, LOW = charging
#define CHG_CURR_PIN     22   // P0.22, OUTPUT, LOW=100mA

float readRemoteBatteryVoltage() {
  // 10-bit ADC, nRF52840 SAADC internal ref ~3.6V, 1MΩ:510kΩ divider
  int raw = analogRead(PIN_VBAT);
  return raw * 3.6f / 1023.0f * (1510.0f / 510.0f);
}

int voltageToPercent(float v) {
  if (v >= 4.20f) return 100;
  if (v <= 3.00f) return 0;
  return (int)((v - 3.00f) / (4.20f - 3.00f) * 100.0f);
}

// ─── MetricsPacket handler (called from both sendCmd loop and main loop) ──────
void processMetricsPacket(const MetricsPacket* m) {
  uint32_t now = millis();

  // Initialise window on first packet
  if (metricsReceivedThisWindow == 0) {
    windowStartTime = now;
    lastMetricsSeq  = m->seq;
  }

  // Count gaps as lost packets
  if (metricsReceivedThisWindow > 0) {
    uint8_t expected = lastMetricsSeq + 1;
    if (m->seq != expected) {
      metricsLostThisWindow += (m->seq - expected) & 0xFF;
    }
  }
  metricsReceivedThisWindow++;
  lastMetricsSeq = m->seq;

  // Recalculate loss at end of each window
  if (now - windowStartTime >= PACKET_LOSS_WINDOW_MS) {
    uint16_t total = metricsReceivedThisWindow + metricsLostThisWindow;
    packetLossPercent = (total > 0) ? (metricsLostThisWindow * 100.0f / total) : 0.0f;
    metricsReceivedThisWindow = 0;
    metricsLostThisWindow     = 0;
    windowStartTime           = now;
  }

  bleInterface.setLineState(m->lineState);
  bleInterface.setMetrics(m->distance_m, m->amps_x10 / 10.0f, m->vesc_mV / 1000.0f);
  bleInterface.setSignalQuality(lora.getRSSI(), lora.getSNR(), packetLossPercent);
  metricsDirty = true;
}

// ─── sendCmd() ── immediate BLE feedback, then LoRa ACK/retry if radio ready ─
bool sendCmd(uint8_t st) {
  uint8_t oldState = confirmedState;

  // Always give instant audio + BLE feedback regardless of LoRa state
  confirmedState = st;
  if (confirmedState == 0 && oldState == 0) buzzer.playStateChange(1, 0);
  else                                       buzzer.playStateChange(oldState, confirmedState);
  bleInterface.setState(confirmedState);
  bleInterface.sendTelemetry();

  if (!loraReady) {
    DBGF("[CMD] LoRa offline — local confirm only  %u→%u", oldState, st);
    return true;
  }

  // ─── LoRa ACK/retry loop ─────────────────────────────────────────────────
  confirmedState = oldState;  // revert until ACK confirms
  bleInterface.setState(confirmedState);

  CmdPacket pkt{};
  pkt.type  = 0x01;
  pkt.seq   = ++txSeq;
  pkt.state = st;

  DBGF("[CMD] %u→%u seq:%u", oldState, st, pkt.seq);

  for (uint8_t attempt = 1; attempt <= CMD_MAX_RETRIES; ++attempt) {
    DBGF("[CMD] attempt %u/%u", attempt, CMD_MAX_RETRIES);

    lora.transmit(reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));
    lora.startReceive();
    awaitingSeq = pkt.seq;
    uint32_t deadline = millis() + ACK_TIMEOUT_MS;

    while (awaitingSeq && (int32_t)(millis() - deadline) < 0) {
      if (radioIRQ) {
        radioIRQ = false;
        uint8_t buf[16];
        size_t  len = sizeof(buf);

        if (lora.readData(buf, len) == RADIOLIB_ERR_NONE && len) {
          if (buf[0] == 0x03 && len >= sizeof(AckPacket)) {
            auto* ack = reinterpret_cast<AckPacket*>(buf);
            if (ack->seq == awaitingSeq) {
              awaitingSeq    = 0;
              confirmedState = st;
              if (confirmedState == 0 && oldState == 0) buzzer.playStateChange(1, 0);
              else                                       buzzer.playStateChange(oldState, confirmedState);
              bleInterface.setState(confirmedState);
              bleInterface.sendTelemetry();
              DBGF("[CMD] ACK  confirmed:%u", confirmedState);
              return true;
            }
          } else if (buf[0] == 0x02 && len >= sizeof(MetricsPacket)) {
            processMetricsPacket(reinterpret_cast<MetricsPacket*>(buf));
          }
        }
        lora.startReceive();
      }
      buzzer.update();
    }

    if (!awaitingSeq) return true;
    DBGF("[CMD] timeout attempt %u", attempt);
  }

  DBGF("[CMD] FAILED — winch unreachable");
  bleInterface.setState(confirmedState);
  bleInterface.sendTelemetry();
  return false;
}

// ─── UP button handler ────────────────────────────────────────────────────────
void onUpClick(Button2& b) {
  lastActivityTime = millis();
  buttonEventCounter++;
  buzzer.playButtonPress();
  if (desiredState < 6) {
    uint8_t old = desiredState++;
    DBGF("[BTN] UP #%lu  %u→%u", buttonEventCounter, old, desiredState);
    sendCmd(desiredState);
  } else {
    DBGF("[BTN] UP #%lu  at MAX", buttonEventCounter);
  }
}

// ─── DOWN button handler (same reliable logic as T3) ─────────────────────────
void processDownButton() {
  uint32_t now    = millis();
  bool     curRaw = digitalRead(BTN_DN_PIN);

  if (curRaw != downBtn.lastRawState)
    downBtn.lastChangeTime = now;

  if ((now - downBtn.lastChangeTime) >= SIMPLE_DEBOUNCE_MS) {
    if (curRaw != downBtn.lastStableState) {
      downBtn.lastStableState = curRaw;

      if (!curRaw) {
        downBtn.pressStartTime = now;
        downBtn.isPressed      = true;
        downBtn.pressConsumed  = false;
      } else if (downBtn.isPressed) {
        uint32_t dur = now - downBtn.pressStartTime;
        downBtn.isPressed = false;

        if (dur >= MIN_PRESS_MS && !downBtn.pressConsumed) {
          downBtn.pressConsumed = true;
          uint32_t gap = now - downBtn.lastClickTime;

          if (gap <= MULTI_CLICK_WINDOW && downBtn.lastClickTime > 0)
            downBtn.clickCount++;
          else
            downBtn.clickCount = 1;

          downBtn.lastClickTime = now;

          if (downBtn.clickCount >= 2) {
            lastActivityTime = millis();
            buttonEventCounter++;
            DBGF("[BTN] DN_EMERGENCY #%lu  desired:%u→0", buttonEventCounter, desiredState);
            desiredState = 0;
            sendCmd(0);
            buzzer.playJumpToZero();
            downBtn.clickCount = 0;
          }
        }
      }
    }
  }

  // Single-click: process after double-click window expires
  if (downBtn.clickCount == 1 && (now - downBtn.lastClickTime) > DOUBLE_CLICK_WINDOW) {
    lastActivityTime = millis();
    buttonEventCounter++;
    if (desiredState > 0) {
      uint8_t old = desiredState--;
      DBGF("[BTN] DN_SINGLE #%lu  %u→%u", buttonEventCounter, old, desiredState);
    } else {
      DBGF("[BTN] DN_SINGLE #%lu  at ZERO", buttonEventCounter);
    }
    sendCmd(desiredState);
    buzzer.playButtonPress();
    downBtn.clickCount = 0;
  }

  downBtn.lastRawState = curRaw;
}

// ─── setup() ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(200);
  DBG("=== XIAO nRF52840 Winch Remote ===");

  // Battery divider enable (active LOW), charge current and sense
  pinMode(VBAT_ENABLE_PIN, OUTPUT);
  digitalWrite(VBAT_ENABLE_PIN, LOW);   // connect divider permanently
  pinMode(CHG_CURR_PIN, OUTPUT);
  digitalWrite(CHG_CURR_PIN, LOW);      // 100mA charging
  pinMode(CHG_STATE_PIN, INPUT);

  // Buttons
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DN_PIN, INPUT_PULLUP);
  downBtn.lastRawState    = digitalRead(BTN_DN_PIN);
  downBtn.lastStableState = downBtn.lastRawState;
  downBtn.lastChangeTime  = millis();
  btnUp.setClickHandler(onUpClick);

  // Buzzer
  buzzer.begin();

  // BLE
  bleInterface.begin();
  bleInterface.setState(confirmedState);

  // LoRa SPI (XIAO defaults: D8=SCK, D9=MISO, D10=MOSI)
  SPI.begin();
  pinMode(LORA_CS_PIN, OUTPUT);
  digitalWrite(LORA_CS_PIN, HIGH);

  DBG("[LoRa] initialising...");
  uint32_t loraStart = millis();
  int16_t  loraErr   = lora.begin(LORA_FREQUENCY_MHZ);
  DBGF("[LoRa] begin() took %lu ms, code %d", millis() - loraStart, loraErr);

  if (loraErr == RADIOLIB_ERR_NONE) {
    lora.setBandwidth(LORA_BANDWIDTH_KHZ);
    lora.setSpreadingFactor(LORA_SPREADING_FACTOR);
    lora.setCodingRate(LORA_CODING_RATE);
    lora.setOutputPower(LORA_OUTPUT_POWER_DBM);
    lora.setCurrentLimit(LORA_CURRENT_LIMIT_MA);
    lora.setDio2AsRfSwitch(true);
    lora.setRfSwitchPins(LORA_RXEN_PIN, RADIOLIB_NC);  // D5 HIGH=RX, LOW=TX
    lora.setDio1Action(dio1ISR);
    lora.startReceive();
    loraReady = true;
    DBG("[LoRa] ready  868.1 MHz  SF9  BW125  CR4/6  22dBm");
  } else {
    DBGF("[LoRa] init failed (code %d) — buttons/BLE still work", loraErr);
  }
  DBG("UP: increment state  |  DN single: decrement  |  DN double: emergency→0");

  lastActivityTime = millis();
  buzzer.playWake();
}

// ─── loop() ──────────────────────────────────────────────────────────────────
void loop() {
  uint32_t now = millis();

  buzzer.setStateIndicator(confirmedState);
  btnUp.loop();
  processDownButton();
  buzzer.update();

  // LoRa receive (metrics from winch, idle mode)
  if (radioIRQ) {
    radioIRQ = false;
    uint8_t buf[16];
    size_t  len = sizeof(buf);

    if (lora.readData(buf, len) == RADIOLIB_ERR_NONE) {
      if (len >= sizeof(MetricsPacket) && buf[0] == 0x02) {
        processMetricsPacket(reinterpret_cast<MetricsPacket*>(buf));
      }
    }
    lora.startReceive();
  }

  // Send BLE notification when new metrics arrived
  if (metricsDirty) {
    bleInterface.sendTelemetry();
    metricsDirty = false;
  }

  // Remote battery update every 10 s
  static uint32_t lastBatUpdate = 0;
  if (now - lastBatUpdate >= 10000) {
    float v      = readRemoteBatteryVoltage();
    int   pct    = voltageToPercent(v);
    bool  chrg   = (digitalRead(CHG_STATE_PIN) == LOW);
    DBGF("[BAT] %.2fV  %d%%  charging:%d", v, pct, chrg);
    bleInterface.setBatteryLevel(v, pct);
    bleInterface.setRemoteCharging(chrg);
    lastBatUpdate = now;
  }

  // Heartbeat BLE send every 2 s (keeps screen live when no LoRa traffic)
  static uint32_t lastHeartbeat = 0;
  if (now - lastHeartbeat >= 2000) {
    uint32_t elapsed  = now - lastActivityTime;
    uint8_t  minsLeft = (elapsed >= SLEEP_TIMEOUT_MS) ? 0
                      : (uint8_t)((SLEEP_TIMEOUT_MS - elapsed) / 60000UL);
    bleInterface.setSleepMins(minsLeft);
    bleInterface.sendTelemetry();
    lastHeartbeat = now;
  }

  // Deep sleep after 30 min of inactivity — only when state is 0
  if (desiredState == 0 && (now - lastActivityTime) >= SLEEP_TIMEOUT_MS) {
    enterDeepSleep();
  }
}
