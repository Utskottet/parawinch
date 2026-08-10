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

## Session Handoff (2026-08-10 evening)

### Changes shipped this session

**1. VESC Lisp `get-current` fix (committed af2fdc6, uploaded to VESC)**
- `docs/vesc-lisp/v2-direct-current.lisp` lines 13 and 88: `(get-current N)` queries a remote motor by CAN ID N — we don't have any. Dropped the arg to read the local motor. Fixed 0-A telemetry. Verified in flight log `winch_v2.10_2026-08-10-19-48-53.csv` — `vescA` now tracks `cmdA` closely.

**2. VESC Lisp ERPM low-state limit lowered to 1500**
- Line 78: `(conf-set 'l-max-erpm 1500)` for states 0–1 (was 5000). Tensioning states spin more slowly.

**3. Stepper limit-switch rewrite — DEFERRED, still under investigation**
- Previous agent's commit `881f459` introduced a `limitHitDirection` direction-guard that only allowed reversing on one end (`false != false` passes, `true != false` fails → second-end hit ignored).
- Replaced with a **release-edge gate** (`switchReleasedSinceHit` set only when pin reads HIGH). Cleaner logic, cycles correctly at both ends.
- Then added a **FALLING-edge GPIO interrupt** on `limitSwitchPin` as backup so a slow main loop can't miss the transition. ISR sets `g_limitInterruptFlag`; `isLimitPressed()` OR's the flag with the polled level.
- Added **display hysteresis** on drum-diameter (`abs >= 2`) so CAN encoder jitter doesn't fire `updateDisplay()` every loop and starve the stepper poll (SPI mutex is shared).
- Added an **on-screen diagnostic line** (cyan, Y=195): `S:motorState R:reversalCount D:msSinceLastReverse BT:buttonToggled`. Purpose: reading counters off the LCD is the only way to inspect a hang state, since opening a serial monitor toggles DTR/RTS and resets the ESP32 — that erases the hung state.
- **Status: still hangs occasionally.** Read one hang state showing `S:0 R:21 D:9072ms BT:0` → motor reached IDLE cleanly via the centering sequence, which means `buttonToggled` went to 0 without the user pressing Button A. Either a phantom button press or a mechanical/electrical issue is flipping BT. Investigation ongoing next session.

### Drum-comp validation — DONE
- Physical test using "tape line to drum + spin motor via VESC Tool" trick to fake encoder to high line-out without paying line out.
- At `lineOut=867m` command 20A base → cmdA shown 15A (theory says 20 × 0.78 ≈ 15.6, truncates to 15) ✓
- At `lineOut=435m`, state 1 base 20A → cmdA=17A on LCD (theory 20 × 0.90 = 18) ✓
- Log `winch_v2.10_2026-08-10-19-48-53.csv` shows compensation applied end-to-end (M5Stack computes → CAN to VESC → back via CAN → LoRa → BLE → phone).

### Open TODO (carry into next session)
1. **Stepper hang root cause (HIGH priority, safety)** — inspect why `buttonToggled` flips to 0 unexpectedly. Suspects: (a) M5Stack Button A wearing / phantom press, (b) mechanical noise on the level-wind limit-switch wiring inducing something upstream, (c) still-undiagnosed code bug. Next session: catch a hang, read LCD diag line, check if BT flipped on its own. Consider adding an on-screen counter for Button-A press events too.
2. **Remove the diagnostic LCD line** once stepper is solid — it's temporary.
3. **Field-test drum comp with real line-out** (not the tape-trick) at 500m+, compare force ratios to today's spoofed numbers.
4. **Verify VESC ERPM=1500 change** on real hardware feels right at tensioning states.
5. **Voltage telemetry**: `vesc_mV` still hardcoded 3700 — need a CAN ID from Lisp for real V.
6. **Hardware**: decoupling caps on Waveshare SX1262 VDD to fix supply desense.

### Diagnostic tips (learned this session)
- **Serial monitor on COM5 resets the ESP32** via CH9102 DTR/RTS. Kills any hung-state investigation. Use the on-LCD diagnostic line instead, or open the port with DTR/RTS disabled from PowerShell.
- **VESC Lisp on estop**: unclear if Lisp keeps running or halts. Test if you need reliable CAN traffic during estop scenarios.
- **VESC Tool RT sliders**: current slider is torque control (may stall under friction), RPM slider actually spins. Stop the LispBM script first or its 3-second watchdog zeros your manual command.

### Build & flash
- Winch: `cd winch && pio run --target upload --upload-port COM5` (port varies)
- Remote: `cd remote && pio run --target upload` (check port)
- VESC Lisp: VESC Tool → LispBM Editor → Stop → paste → Upload → Run (→ Flash for persistence)

## Open Items (long-standing, not this session)
- Hardware: decoupling caps on Waveshare SX1262 VDD to fix supply desense
- Voltage telemetry: vesc_mV hardcoded 3700, need CAN ID for real voltage
- Field test for drum compensation at real distance still pending
