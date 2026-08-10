# ParaWinch — Claude Code Context

## Repository
- **Mono repo:** https://github.com/Utskottet/parawinch

## Hardware
| Unit | MCU | LoRa module |
|---|---|---|
| Remote | Seeed XIAO nRF52840 | Wio SX1262 (SKU 102010710), B2B connector |
| Winch | ESP32 (M5Stack) | Waveshare SX1262 (Core1262-868M) |
| VESC | — | CAN bus to M5Stack, Lisp script for current ramping |

## RF Configuration (must match on all nodes)
- 868.1 MHz · SF9 · BW125 kHz · CR4/6 · 22 dBm
- OCP: 140 mA on XIAO/remote, disabled on winch (see known issues)

## LoRa Pin Mapping
**XIAO remote:**
```
CS=D4, DIO1=D1, BUSY=D3, RST=D2, RXEN=D5
SPI: SCK=D8, MISO=D9, MOSI=D10
```
**Winch ESP32:**
```
NSS=5, DIO1=34, RST=13, BUSY=26, SCK=18, MISO=19, MOSI=23
```

## Packet Protocol
```
0x01  CmdPacket      remote → winch   (type, seq, state 0-6)
0x02  MetricsPacket  winch → remote   (type, seq, lastCmdSeq, amps_x10, distance_m, vesc_mV, lineState, scaled_amps_x10, baseAmps[6])
0x03  AckPacket      winch → remote   (type, seq, pad, pad)
0x05  ConfigPacket   phone → winch    (type, seq, amps[6]) — relayed via BLE→remote→LoRa
```

## BLE Packet Layout (20 bytes, remote → phone)
```
[0]     state         uint8   0-6
[1-2]   distance_m    uint16  big-endian
[3]     lineState     uint8   0=READY, 1=ARMED, 2=STOPPED
[4-5]   amps_x10      uint16  big-endian (VESC actual)
[6]     temp          uint8   motor temp °C
[7]     rssi          int8
[8]     ampSlot       uint8   rotating 0-5, pairs with [19]
[9]     lostPct       uint8   packet loss 0-100%
[10]    winchBat      uint8   %
[11]    remoteBat     uint8   %
[12]    charging      uint8   0/1
[13]    snr           int8
[14-15] vescVolt_dV   uint16  big-endian (×10)
[16]    sleepMins     uint8
[17-18] scaledAmps_x10 uint16 big-endian (drum-compensated cmd amps × 10)
[19]    ampValue      uint8   baseAmps[ampSlot] for state (slot+1)
```
Note: BLE ATT MTU is 23 (20 payload). Do NOT exceed 20 bytes — causes data corruption.

## CAN Bus (M5Stack ↔ VESC)
- TWAI 500kbps, GPIO 17 (TX) / GPIO 16 (RX)
- M5Stack sends: `CAN_PACKET_SET_CURRENT` (EID, controller_id=1), int16 amps, every 200ms
- VESC sends: CAN SID 7 (distance i32), 8 (current i32), 9 (temp i32)
- VESC Lisp ramps current at 25A/s (0.4s ramp), 3s watchdog timeout → zero current

## Drum Compensation
- `scaledCurrent = baseCurrent × (drumDiam / 265.0)`, clamped [0, 200]
- Drum range: 150mm (core/empty) → 265mm (full), 1500m line
- Area model: `r = sqrt(rCore² + fraction × (rFull² - rCore²))`
- Factor ≤ 1.0 — only reduces current, never increases
- CAN sends scaledCurrent, not baseCurrents[state]

## Critical Firmware Fixes (already applied — do not revert)

### 1. RF switch pin — XIAO/Wio SX1262 only (~30 dB recovered)
```cpp
lora.setRfSwitchPins(LORA_RXEN_PIN, RADIOLIB_NC);  // in setup(), after begin()
```

### 2. OCP silent failure — both XIAO units
Fixed: `LORA_CURRENT_LIMIT_MA 140.0f` in `lorastruct.h`.

### 3. Waveshare SX1262 (winch) — RF switch
`setDio2AsRfSwitch(true)` called automatically by RadioLib. Do NOT add `setRfSwitchPins()`.

### 4. Winch OCP — supply desense (known hardware problem)
`setCurrentLimit()` commented out in winch `LoRaComm.cpp`. Needs hardware fix (decoupling caps).

### 5. BLE RX characteristic — variable length
Must use `setMaxLen(20)` not `setFixedLen(20)` for RX char, otherwise phone config writes (8 bytes) are rejected.

### 6. BLE TX characteristic — 20 byte limit
Must use `setFixedLen(20)`. 26-byte packets exceed default ATT MTU and cause data corruption.

## Project Structure
```
parawinch/
├── winch/src/         M5Stack ESP32 winch controller
├── remote/src/        XIAO nRF52840 remote (BLE + LoRa)
├── sim/src/           XIAO winch simulator
├── archive/           Old ItsyBitsy firmware
├── docs/              Specs, lisp, calibration, flight logs
├── index.html         Phone web app (GitHub Pages)
└── CLAUDE.md          This file
```

## Session Handoff (2026-08-10)

### Changes made this session (not yet compiled/flashed)

**1. Stepper level-wind hang fix (CRITICAL — safety issue)**
- `winch/src/StepperControl.cpp` + `.h` rewritten
- Root cause: `Serial.printf` with float formatting ran every 5ms in main loop (added during drum compensation work). Slowed loop enough that edge-based limit switch detection missed transitions.
- Fix part A: throttled drum debug printf to 1/sec, removed raw metrics hex dumps from LoRa task
- Fix part B: rewrote `checkLimitSwitch()` → `isLimitPressed()` — now level-based with direction guard instead of edge-based. Tracks `limitHitDirection` so it always moves AWAY from a pressed switch. Cannot double-toggle. Physically cannot hang even if loop is slow.
- Old `toggleMotorDirection()` replaced by `reverseFromLimit()` which records direction before reversing.
- **Test procedure:** hold limit switch button manually while stepper runs — old code hangs, new code should reverse and keep moving away.

**2. Removed debug Serial.printf spam from winch main.cpp**
- Removed raw metrics hex dump (after-cmd and periodic) from LoRa task
- Removed config debug prints
- Drum compensation printf throttled to once per second
- All functional code unchanged, only debug output removed

**3. VESC Lisp bug (NOT YET FIXED — do in next session)**
- `docs/vesc-lisp/v2-direct-current.lisp` line 88: `(get-current 2)` queries non-existent CAN ID 2
- Should be `(get-current)` with no argument for local motor current
- This causes intermittent 0A readings. The motor IS applying current at standstill (user confirmed tension is felt), but telemetry reports 0 because it's querying the wrong CAN address.
- Also fix line 13: `(get-current 1)` → `(get-current)` for consistency

### Build & flash instructions
- Winch: `cd winch && platformio run` then `platformio run --target upload` (COM7)
- Remote: `cd remote && platformio run` then upload (check COM port)
- VESC Lisp: paste into VESC Tool > LispBM Editor > Upload

### Test plan for next session
1. Build winch firmware, verify compiles clean
2. Flash to M5Stack
3. Test stepper: hold limit switch while running — must not hang
4. Test stepper: add `delay(100)` temporarily in loop() to simulate slow loop — must still reverse at switches
5. Fix VESC Lisp `get-current` bug, upload via VESC Tool
6. Test current reading: command amps while holding line stationary — should now show non-zero amps
7. Run scaling test with line out, grab CSV log from phone app

### Ground test analysis (2026-08-10 logs)
- Log 1 (34m out): scaling factor ~0.99, working correctly
- Log 2 (204m out): scaling factor ~0.96, 1-2A lower than log 1 — correct
- Real scaling difference shows at 750m+ (factor 0.82) and 1500m (factor 0.57)
- Base currents configured as {0, 10, 29, 34, 35, 36, 38} (from phone config, not defaults)

## Open Items
- Hardware: decoupling caps on Waveshare SX1262 VDD to fix supply desense
- Voltage telemetry: vesc_mV hardcoded 3700, need CAN ID for real voltage
- ERPM limiting: no max speed set in Lisp — low states spin too fast
- Field test for drum compensation at distance pending
