# ikoka-remote — remote firmware for the Fork Ikoka Nano PCB

Port of [`../remote`](../remote) (XIAO nRF52840 + Wio SX1262) to the Fork Ikoka
Nano board: **Seeed XIAO nRF52840 Plus + EBYTE E22-900M30S**.

The protocol, state machine, button semantics, buzzer patterns, BLE layout and
RF parameters are **unchanged** — this remote is interchangeable with the Wio
one from the winch's and the phone app's point of view. Every difference is
forced by the hardware and is listed below.

## Environments

| env        | what it is |
|---|---|
| `ikoka`    | the remote firmware |
| `selftest` | standalone hardware bring-up / diagnostics, serial menu at 115200 |

```
pio run -e ikoka    --target upload
pio run -e selftest --target upload
pio device monitor -b 115200
```

## Hardware mapping (from the KiCad netlist, `U1` pad → net)

| XIAO Plus | BSP idx | nRF pin | net | function |
|---|---|---|---|---|
| D0  | 0  | P0.02 | `SX126X_CS`    | E22 pin 19 NSS |
| D1  | 1  | P0.03 | `SX126X_DIO1`  | E22 pin 13 DIO1 (IRQ) |
| D2  | 2  | P0.28 | `SX126X_BUSY`  | E22 pin 14 BUSY |
| D3  | 3  | P0.29 | `SX126X_RESET` | E22 pin 15 NRST **+ MT3608B boost EN** |
| D7  | 7  | P1.12 | `SX126X_RXEN`  | E22 pin 6 RXEN |
| D8/D9/D10 | 8/9/10 | P1.13/14/15 | `SPI_*` | SCK / MISO / MOSI |
| D11 | 30 | P0.15 | `BUTTON_1` | J1-1 — **UP** |
| D12 | 31 | P0.19 | `BUTTON_2` | J1-2 — **DOWN** |
| D13 | 32 | P1.01 | `BUTTON_3` | J1-3 — spare |
| D17 | **38** | P1.03 | `BUTTON_4` | J1-4 — spare |
| D18 | 37 | P1.05 | `PIEZO_A` via R4 100R | J4-1 — tone output |
| D19 | **36** | P1.07 | `PIEZO_B` via R5 100R | J4-2 — piezo return, parked LOW |

The **BSP index** column is what the code must use, and for D17/D19 it is *not*
what Seeed's `D17`/`D19` macros give you — see the warning below.

J1-5 is GND: buttons are active-low with internal pullups.

E22 **TXEN (pin 7) is bonded to E22 DIO2 (pin 8)** on the PCB, so the SX1262
drives its own transmit enable and there is no MCU pin for it.

> **The `D17` and `D19` macros in Seeed's BSP do not match the pads printed on
> the board.** `variant.h` defines `D17 = 36 (P1.07)` and `D19 = 38 (P1.03)`,
> but the pad silkscreened D19 is physically P1.07 and the pad silkscreened D17
> is P1.03 — that pair is transposed. D18 (P1.05) is unaffected.
>
> This was measured, not assumed. Driving the piezo's first leg (KiCad pad
> "D18") works on BSP 37 as expected, but the circuit only completes when
> **BSP 36** is grounded; grounding BSP 38 is silent. Since the KiCad netlist
> names pads by silkscreen, `utilities.h` defines `PAD_D17`/`PAD_D19` as raw
> indices and confines the transposition to that one table. **Never use the bare
> `D17`/`D19` macros on this board.**
>
> Re-check with the `selftest` build: `1` grounds BSP 36 (sounds), `2` grounds
> BSP 38 (silent), `b` scans every pad for button presses.

## Differences from `../remote`

Everything else in `main.cpp`, `buzzer.cpp`, `ble_interface.*` and
`lorastruct.h` is byte-identical to the Wio build.

1. **`utilities.h` — rewritten.** All pin assignments move (table above).
2. **`platformio.ini` — `seeed-xiao-afruitnrf52-nrf52840-plus`** and a second
   env for the self-test.
3. **TCXO voltage.** DIO3 powers the E22's 32 MHz TCXO (E22 manual §4.2).
   RadioLib defaults DIO3 to 1.6 V; EBYTE E22 parts want 1.8 V, and it must be
   applied before `begin()`'s first calibration — so `begin()` is now called
   with its full argument list instead of frequency alone. Every other argument
   is RadioLib's own default and is re-applied by the existing setters below it.
4. **E22 rail sequencing.** `SX126X_RESET` is also the MT3608B boost `EN`
   (R3 300k pulls it down), so the module is unpowered until reset is released:
   `setup()` drives RST high and waits `LORA_BOOT_SETTLE_MS` before the first
   SPI transaction, and parks RXEN low so it does not float during boot.
5. **Deep sleep drops the rail.** `enterDeepSleep()` pulls RST low after
   `lora.sleep()`, which cuts the boost and takes the module to zero draw
   instead of its own sleep current. nRF52 System OFF retains GPIO state.
6. **Piezo drive.** J4 gives the piezo a pin per leg instead of one pin to
   ground, so `buzzer.begin()` parks `BUZZER_GND_PIN` low as the return path.
   High-drive mode is now resolved through `g_ADigitalPinMap` rather than a
   hard-coded `NRF_P1->PIN_CNF[12]`, so it follows whatever `BUZZER_PIN` is.
   All frequencies and patterns are unchanged.
7. **Battery comments corrected.** Same code, same BSP pin numbers; the port
   assignments in the old comment (P0.22 / P0.23) were wrong on both boards —
   pin 22 is P0.13 `HICHG` and pin 23 is P0.17 `~CHG`. `PIN_VBAT` is used
   symbolically and moves from index 32 to 35 on the Plus variant.

## Output power

`LORA_OUTPUT_POWER_DBM` stays at **22 dBm** — that is the SX1262's own output,
which is what the module's YP2233W PA then amplifies. Measured PA gain is
+7.25 dB, giving **~29.4 dBm (≈870 mW) at the antenna**, about 7 dB more than
the Wio build for the same firmware number. `setCurrentLimit()` still guards
only the SX1262's internal PA and is unchanged at 140 mA; the module as a whole
peaks near 650 mA off the 5.2 V boost.

**Never transmit without an antenna on the E22's IPEX connector.**

## Verified on hardware (2026-09-12, board on COM6)

| Item | Result |
|---|---|
| E22 5 V boost rail + reset gating | BUSY falls immediately on RST release |
| SPI — all four lines | sync word reads `0x14 0x24`; `0xA5 0x5A` scribble/read-back/restore OK |
| TCXO at 1.8 V on DIO3 | `device errors 0x0000` — no `XOSC_START_ERR` |
| `begin()` | `0` (was `-2 CHIP_NOT_FOUND` before the fixes below) |
| TX completes at full power | 8/8 transmits, 114–115 ms each, no `PA_RAMP`, no `TX_TIMEOUT` |
| Buttons | J1-1 → BSP 30 (D11/P0.15), J1-2 → BSP 31 (D12/P0.19) |
| Piezo | drive BSP 37 (P1.05); return **BSP 36 (P1.07)** — grounding BSP 38 is silent |
| Battery sense | 3.7–4.1 V, tracks plausibly |
| Real firmware end-to-end | `[BTN] UP → [CMD] → attempt 1/3` retry loop, LoRa online |
| **RF link** | **not yet verified — no live peer was available** |

Measured airtime of 114–115 ms matches the ~107 ms theoretical airtime for
SF9 / BW125 / CR4/6 with a 3-byte payload plus RadioLib overhead, which
independently confirms the radio is modulating a full frame. Note that `TxDone`
only proves the *SX1262* transmitted — the PA sits outside the chip's
monitoring, so confirming the +7 dB actually reaches the antenna needs a
receiver or a power meter.

Observed during the TX bursts: the battery ADC reading fell from 4.01 V to
3.72 V across six 1 W transmits with no cell fitted. The transmits still
completed, but fit a real LiPo before field use — peak draw is ~800 mA out of
VBUS and USB ports are not obliged to supply that.

## Self-test commands

```
i  pin map this build is using       t  one ping to winch/sim
b  button scan (press each button)   T  20 pings + loss/RSSI/RTT summary
z  piezo identification (y/n)        x  RF switch test — proves RXEN
a  beep on configured buzzer pins    l  listen mode, dumps every packet
r  E22 bring-up: rail, BUSY, SPI,    c  5 s CW carrier (ANTENNA REQUIRED)
   TCXO, register read/write         v  battery voltage
d  status + device errors            p<n>/k<n>  set power dBm / TCXO volts x10
A  full non-interactive sequence
```

`r` is the one that matters most: it cycles the boost rail, times how long BUSY
takes to fall, runs `begin()`, reads the sync-word registers back, scribbles and
restores them to prove all four SPI lines, and decodes the SX1262 device-error
register — bit 5 there is `XOSC_START_ERR`, i.e. the TCXO did not start.
