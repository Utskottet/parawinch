#pragma once
#include <stdint.h>

/*────────── RF params for EU-868 ─────────*/
#define LORA_FREQUENCY_MHZ      868.1f
#define LORA_BANDWIDTH_KHZ      125.0f
#define LORA_SPREADING_FACTOR   9
#define LORA_CODING_RATE        6          // 4/6
#define LORA_OUTPUT_POWER_DBM   22
#define LORA_CURRENT_LIMIT_MA   140.0f  // RadioLib max; SX1262 draws ~118mA at 22dBm

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
};

struct HeartbeatPacket {      // 0x04 (optional)
  uint8_t type = 0x04;
  uint8_t seq;
};

#pragma pack(pop)
