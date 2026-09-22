#pragma once
#include <stdint.h>

/*────────── RF params for EU-868 ─────────*/
#define LORA_FREQUENCY_MHZ      868.1f
#define LORA_BANDWIDTH_KHZ      125.0f
#define LORA_SPREADING_FACTOR   9
#define LORA_CODING_RATE        6          // 4/6
#define LORA_OUTPUT_POWER_DBM   22
#define LORA_CURRENT_LIMIT_MA   140.0f  // RadioLib max; SX1262 draws ~118mA at 22dBm

/*  E22-900M30S notes (Ikoka Nano build only)
 *  - The 22 dBm above is the SX1262's own output, which is what the module's
 *    YP2233W PA then amplifies: +7.25 dB gain, measured ~29.4 dBm at the ANT
 *    pin with the SX1262 at its 22 dBm ceiling. Same number as the Wio build,
 *    ~7 dB more radiated. Do NOT transmit without an antenna fitted.
 *  - CURRENT_LIMIT is the SX1262's own OCP and is unaffected by the PA; the
 *    module as a whole peaks around 650 mA off the 5 V boost.
 *  - DIO3 supplies the module's 32 MHz TCXO (E22 manual section 4.2). RadioLib
 *    defaults DIO3 to 1.6 V; EBYTE E22 parts want 1.8 V, and it has to be set
 *    before the first calibration, i.e. passed into begin().
 */
#define LORA_TCXO_VOLTAGE_V     1.8f

/*────────── application timing ───────────*/
#define CMD_MAX_RETRIES         3
#define ACK_TIMEOUT_MS          200
#define METRICS_PERIOD_MS       1000     // winch sends telemetry once per second

/*────────── packed packet structs ────────*/
#pragma pack(push,1)

struct CmdPacket {            // 0x01  remote → winch
  uint8_t type  = 0x01;
  uint8_t seq;
  uint8_t state;              // 0-6
};

struct AckPacket {            // 0x03  winch → remote
  uint8_t type = 0x03;
  uint8_t seq;
  uint8_t pad0 = 0;
  uint8_t pad1 = 0;
};

struct MetricsPacket {        // 0x02  winch → remote (broadcast)
  uint8_t  type;
  uint8_t  seq;
  uint8_t  lastCmdSeq;
  uint16_t amps_x10;          // Amps * 10 (e.g. 153 = 15.3 A)
  uint16_t distance_m;        // Line distance in metres
  uint16_t vesc_mV;           // VESC voltage in millivolts
  char     lineState;         // 'R'=Ready, 'A'=Armed, 'S'=Stopped
  uint16_t scaled_amps_x10;  // compensated commanded amps × 10
  uint8_t  baseAmps[6];      // current amp table (state 1-6)
};

// MetricsPacket grew by scaled_amps_x10 + baseAmps[6] when drum compensation
// landed. The bench winch simulator (and sim/src) still send the original
// 10-byte frame, so the remote accepts anything from METRICS_BASE_LEN up and
// treats the absent tail as zero. The real winch sends the full 18 bytes.
#define METRICS_BASE_LEN  10   // sizeof() through lineState

struct ConfigPacket {         // 0x05  phone → winch (via remote relay)
  uint8_t type = 0x05;
  uint8_t seq;
  uint8_t amps[6];            // state 1-6, clamped 0-200
};

struct HeartbeatPacket {      // 0x04 (optional)
  uint8_t type = 0x04;
  uint8_t seq;
};

#pragma pack(pop)
