/**
 * ble_interface.cpp - BLE NUS telemetry implementation
 * Uses Adafruit Bluefruit (same API as ItsyBitsy proof-of-concept).
 */

#include "ble_interface.h"

// BLE objects are file-scoped (same pattern as ItsyBitsy)
static BLEService        _winchService(BLE_SERVICE_UUID);
static BLECharacteristic _txChar(BLE_CHAR_TX_UUID);
static BLECharacteristic _rxChar(BLE_CHAR_RX_UUID);

BLEInterface bleInterface;

// ─── RX callback (commands arriving from web app) ────────────────────────────
static void rxWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr,
                             uint8_t* data, uint16_t len) {
  Serial.print("[BLE] RX: ");
  for (uint16_t i = 0; i < len; i++) Serial.printf("%02X ", data[i]);
  Serial.println();
}

// ─── begin() ────────────────────────────────────────────────────────────────
void BLEInterface::begin() {
  Bluefruit.begin();
  Bluefruit.setName(BLE_DEVICE_NAME);
  Bluefruit.setTxPower(4);

  _winchService.begin();

  _txChar.setProperties(CHR_PROPS_NOTIFY);
  _txChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  _txChar.setFixedLen(20);
  _txChar.begin();

  _rxChar.setProperties(CHR_PROPS_WRITE);
  _rxChar.setPermission(SECMODE_NO_ACCESS, SECMODE_OPEN);
  _rxChar.setWriteCallback(rxWriteCallback);
  _rxChar.setFixedLen(20);
  _rxChar.begin();

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(_winchService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(160, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);

  Serial.println("[BLE] Advertising as '" BLE_DEVICE_NAME "'");
}

// ─── Setters ─────────────────────────────────────────────────────────────────
void BLEInterface::setState(uint8_t s) { _state = s; }

void BLEInterface::setLineState(char c) {
  switch (c) {
    case 'A': _lineState = 1; break;
    case 'S': _lineState = 2; break;
    default:  _lineState = 0; break; // 'R' or '-'
  }
}

void BLEInterface::setMetrics(uint16_t distance, float current_A, float voltage_V) {
  _distance_m  = distance;
  _amps_x10    = (uint16_t)(current_A * 10.0f + 0.5f);
  _vescVolt_dV = (uint16_t)(voltage_V * 10.0f + 0.5f);
}

void BLEInterface::setSignalQuality(float rssi_dBm, float snr_dB, float lossPercent) {
  _rssi    = (int8_t)rssi_dBm;
  _snr     = (int8_t)snr_dB;
  _lostPct = (uint8_t)(lossPercent > 100.0f ? 100 : lossPercent);
}

void BLEInterface::setBatteryLevel(float /*voltage*/, int percent) {
  _remoteBat = (uint8_t)(percent > 100 ? 100 : percent);
}

void BLEInterface::setWinchBattery(uint8_t pct)   { _winchBat  = pct; }
void BLEInterface::setTempController(uint8_t degC) { _temp      = degC; }
void BLEInterface::setRemoteCharging(bool c)       { _charging  = c; }
void BLEInterface::setSleepMins(uint8_t mins)      { _sleepMins = mins; }

// ─── sendTelemetry() ─────────────────────────────────────────────────────────
bool BLEInterface::sendTelemetry() {
  if (!Bluefruit.connected()) return false;

  uint8_t pkt[20] = {0};
  pkt[0]  = _state;
  pkt[1]  = (_distance_m >> 8) & 0xFF;
  pkt[2]  = _distance_m & 0xFF;
  pkt[3]  = _lineState;
  pkt[4]  = (_amps_x10 >> 8) & 0xFF;
  pkt[5]  = _amps_x10 & 0xFF;
  pkt[6]  = _temp;
  pkt[7]  = (uint8_t)_rssi;
  // pkt[8] = 0
  pkt[9]  = _lostPct;
  pkt[10] = _winchBat;
  pkt[11] = _remoteBat;
  pkt[12] = _charging ? 1 : 0;
  pkt[13] = (uint8_t)_snr;
  pkt[14] = (_vescVolt_dV >> 8) & 0xFF;
  pkt[15] = _vescVolt_dV & 0xFF;
  pkt[16] = _sleepMins;

  _txChar.notify(pkt, 20);
  return true;
}
