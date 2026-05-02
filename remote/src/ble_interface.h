/**
 * ble_interface.h - BLE NUS telemetry interface for XIAO nRF52840
 * Replaces the e-paper DisplayController from T3 remote.
 * Setter API mirrors DisplayController exactly for easy Phase 2 porting.
 *
 * BLE 20-byte packet layout (TX notify → phone):
 *  [0]     state       uint8   0-6
 *  [1-2]   distance_m  uint16  big-endian, metres
 *  [3]     lineState   uint8   0=READY, 1=ARMED, 2=STOPPED
 *  [4-5]   amps_x10    uint16  big-endian  (÷10 for display in Amps)
 *  [6]     temp        uint8   °C controller temperature
 *  [7]     rssi        int8    cast to uint8
 *  [8]     reserved    0
 *  [9]     lostPct     uint8   packet loss 0-100%
 *  [10]    winchBat    uint8   winch battery %
 *  [11]    remoteBat   uint8   remote battery %
 *  [12]    charging    uint8   0/1
 *  [13]    snr         int8    cast to uint8
 *  [14-15] vescVolt_dV uint16  big-endian, voltage×10 (e.g. 480 = 48.0V)
 *  [16-19] reserved    0
 */

#pragma once
#include <Arduino.h>
#include <bluefruit.h>

#define BLE_DEVICE_NAME   "Winch"
#define BLE_SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHAR_TX_UUID  "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_CHAR_RX_UUID  "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

class BLEInterface {
public:
  void begin();

  // ── Same setter API as T3's DisplayController ────────────────────────
  void setState(uint8_t newState);
  void setLineState(char c);               // 'R', 'A', 'S', '-'
  void setMetrics(uint16_t distance, float current_A, float voltage_V);
  void setSignalQuality(float rssi_dBm, float snr_dB, float lossPercent);
  void setBatteryLevel(float voltage, int percent); // remote battery

  // ── ItsyBitsy features kept ───────────────────────────────────────────
  void setWinchBattery(uint8_t pct);
  void setTempController(uint8_t degC);
  void setRemoteCharging(bool charging);
  void setSleepMins(uint8_t mins);   // minutes until deep sleep (byte [16])

  bool sendTelemetry();
  bool isConnected() const { return Bluefruit.connected(); }

private:
  uint8_t  _state       = 0;
  uint8_t  _lineState   = 0;
  uint16_t _distance_m  = 0;
  uint16_t _amps_x10    = 0;
  uint8_t  _temp        = 0;
  int8_t   _rssi        = 0;
  int8_t   _snr         = 0;
  uint8_t  _lostPct     = 0;
  uint8_t  _winchBat    = 0;
  uint8_t  _remoteBat   = 0;
  bool     _charging    = false;
  uint16_t _vescVolt_dV = 0;
  uint8_t  _sleepMins   = 30;
};

extern BLEInterface bleInterface;
