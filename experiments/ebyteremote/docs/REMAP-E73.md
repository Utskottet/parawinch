# U5 swap: MDBT50Q-1MV2 → EBYTE E73-2G4M08S1C

**The MCU is the E73. It was never the MDBT50Q.** The handoff buried this under a
heading called "Fallback MCU"; the real decision gate was "MDBT50Q vs E73 from JLC
stock check", and MDBT50Q stock is 0. If any other document in this project says
MDBT50Q, that document is stale.

Part: `E73-2G4M08S1C` · LCSC **`C356849`** · JLC stock 1525 · extended · ~$8.13/1,
~$6.36/30 · 13.0 × 18.0 mm · onboard antenna · 43 pads.

Pinout below was read from the EasyEDA symbol + package for `C356849`, not assumed.
"inset" = pad sits 2.1 mm under the module body (reflow / JLC assembly only, not
reachable with an iron). "edge" = castellated.

Only these blocks change: `U5` and its labels. **USB-C, ESD, TP4056, boost, LDO,
buttons, LEDs, buzzer and battery are untouched** — their net *names* are all
identical, only the module pad each name lands on moves.

---

## Step 0 — purge the fabricated symbol library FIRST

There is a **fake `E73-2G4M08S1C` symbol** registered in KiCad's *global* library
list. It has **33 invented pins** (P0.11, P0.14, P0.19, P0.21, P0.23, P0.25, P0.27,
P1.01 — none of which exist on this module), an **empty Footprint field**, and no
datasheet. It was hand-written by an earlier session, not imported from anything.
If you wire from it you will produce a board that cannot work.

It also contains fabricated `TPS61023` and `E22-900M30S` stubs. **Your schematic
does not use any of them** — every real part resolves to `parawinch:` or
`E22-900M30S:` — so removing it breaks nothing.

1. Undo / delete the E73 symbol you just placed.
2. Eeschema → *Preferences → Manage Symbol Libraries → Global* tab → select the
   row named **`ParaWinchRemote`** → delete the row → OK.
3. Move both copies of the file out of the way (identical, md5
   `89fe2322…`):
   - `C:\Users\Edvin buregren\Documents\EbyteWinchRemote\ParaWinchRemote.kicad_sym`
   - `C:\Program Files\KiCad\10.0\share\kicad\symbols\ParaWinchRemote.kicad_sym`
     ← needs admin. **Nothing project-specific belongs in KiCad's install
     directory**; while it sits there it can contaminate any other project.
4. **Close KiCad completely.** `easyeda2kicad` appends to `lib/parawinch.kicad_sym`
   and `lib/parawinch.pretty/`; KiCad caches those on open.
5. Back up: copy `ParaWinchRemote.kicad_sch` somewhere outside the project folder.

## Step 1 — import the E73 symbol + footprint

```
easyeda2kicad --full --lcsc_id=C356849 --output ./lib/parawinch
```

Gives you symbol `parawinch:E73-2G4M08S1C` and footprint
`parawinch:WIRELM-SMD_E73-2G4M08S1C` (+ 3D model).

**Verify before trusting it — the pin count must be 43.** If you see 33, you are
still looking at the fake symbol from Step 0. Open `lib/parawinch.kicad_sym`, find
the `E73-2G4M08S1C` symbol, and spot-check pin 1 = `P1.11`, pin 19 = `VCC`,
pin 37 = `SWD`, pin 43 = `NF2`. The table in this document was cross-checked
against **Ebyte's own `E73-2G4M08S1C User Manual`, section 3 "Size and pin
definition"** — all 43 pins match the LCSC symbol exactly. Ebyte calls pin 19
`VDD` and pin 25 `DCCH`; the LCSC symbol calls them `VCC` and `DCH`. Same pins.

EasyEDA abbreviates names: `AI0`=P0.02/AIN0,
`AI2`=P0.04, `AI3`=P0.05, `AI4`=P0.28, `AI5`=P0.29, `AI6`=P0.30, `AI7`=P0.31,
`P12`=P0.12, `P15`=P0.15, `P17`=P0.17, `VDH`=VDDH, `DCH`=DCC, `VBS`=VBUS,
`SWD`=SWDIO, `SWC`=SWDCLK, `NF1`=P0.09/NFC1, `NF2`=P0.10/NFC2.

## Step 2 — remove the old U5

In Eeschema, select the MDBT50Q symbol **and all its pin stub wires and labels**,
delete the lot. Leave every other block alone. Don't reuse the old label text
objects — you'll place fresh ones, and stale labels floating in space are exactly
what caused the `BOOST_SW` and orphan-NC-flag ERC noise already in `Todo.md`.

## Step 3 — place the E73 and set its fields

Place `parawinch:E73-2G4M08S1C`. Then set:

| Field | Value |
|---|---|
| Reference | `U5` |
| Value | `E73-2G4M08S1C` |
| Footprint | `parawinch:WIRELM-SMD_E73-2G4M08S1C` |
| `LCSC Part` | `C356849` |

Field name must be exactly `LCSC Part` — that's what the other 15 parts already use.

## Step 4 — wire it, one edge at a time

Short wire stub off each pin, then a net label on the stub. Work the edges in
order so you can tick straight down the list.

### Right edge — pins 1–10. The entire LoRa bus, all castellated.

| Pin | GPIO | Label |
|---|---|---|
| 1 | P1.11 | `LORA_NSS` |
| 2 | P1.10 | `LORA_SCK` |
| 3 | P0.03 | `LORA_MOSI` |
| 4 | P0.28 | `LORA_MISO` |
| 5 | GND | `GND` |
| 6 | P1.13 | `LORA_BUSY` |
| 7 | P0.02 | `LORA_DIO1` |
| 8 | P0.29 | `LORA_RST` |
| 9 | P0.31 | `LORA_TXEN` |
| 10 | P0.30 | `LORA_RXEN` |

This is the whole point of the layout: all nine radio lines leave one face of the
module, straight at the E22.

### Bottom edge — pins 11–25.

| Pin | GPIO | Label |
|---|---|---|
| 11 | XL1 / P0.00 | 32.768 kHz crystal — see below |
| 12 | P0.26 | `LED_LINK` |
| 13 | XL2 / P0.01 | 32.768 kHz crystal — see below |
| 14 | P0.06 | `BUZZER_PWM` |
| 15 | P0.05 / AIN3 | `ADC_VBAT` |
| 16 | P0.08 | `BTN_SELECT` |
| 17 | P1.09 | **no-connect flag** (spare) |
| 18 | P0.04 | `BTN_UP` |
| 19 | VCC | `+3V3` |
| 20 | P0.12 | `BTN_DOWN` |
| 21 | GND | `GND` |
| 22 | P0.07 | **no-connect flag** (spare) |
| 23 | VDDH | `+3V3` |
| 24 | GND | `GND` |
| 25 | DCC | **no-connect flag** (normal-voltage mode) |

### Left edge — pins 26–43.

| Pin | GPIO | Label |
|---|---|---|
| 26 | RST / P0.18 | `SWD_RESET` |
| 27 | VBUS | `VBUS` |
| 28 | P0.15 | `BTN_ESTOP` |
| 29 | D− | `USB_DN` |
| 30 | P0.17 | **no-connect flag** (spare) |
| 31 | D+ | `USB_DP` |
| 32 | P0.20 | **no-connect flag** (spare) |
| 33 | P0.13 | `5V_EN` |
| 34 | P0.22 | **no-connect flag** (spare) |
| 35 | P0.24 | `LED_PWR` |
| 36 | P1.00 | **no-connect flag** (spare) |
| 37 | SWDIO | `SWDIO` |
| 38 | P1.02 | **no-connect flag** (spare) |
| 39 | SWDCLK | `SWDCLK` |
| 40 | P1.04 | **no-connect flag** (spare) |
| 41 | NFC1 / P0.09 | **no-connect flag** (spare) |
| 42 | P1.06 | **no-connect flag** (spare) |
| 43 | NFC2 / P0.10 | **no-connect flag** (spare) |

**12 no-connect flags:** 17, 22, 25, 30, 32, 34, 36, 38, 40, 41, 42, 43.
(Plus 11 and 13 if you skip the crystal — 14 total.)

### The crystal decision is now forced — the manual settles it

Ebyte's manual, pins 11 and 13: **"Connect to 32.768 kHz crystal."** The module has
**no onboard LFXO**. That closes the open question in `Todo.md` — it was never
"internal to the module", that note came from the MDBT50Q.

So either:

- **Fit it (recommended).** 32.768 kHz 3215 crystal + 2 × 12 pF 0603 on pins 11/13.
  ~2 SEK. Gives you accurate BLE timing, much lower sleep current, and removes the
  risk that a stock nRF52 bootloader/softdevice build expects an LFXO and hangs.
- **Skip it.** NC flags on 11 and 13, and **confirm your bootloader and SoftDevice
  are configured for the internal RC low-frequency source before you order.**

## Step 5 — check your work

Export the netlist and diff it against the table above, don't eyeball the symbol:

```
"C:\Program Files\KiCad\10.0\bin\kicad-cli.exe" sch export netlist ParaWinchRemote.kicad_sch
```

18 signal nets should touch `U5`, plus `+3V3` ×2, `GND` ×3. Nothing else in the
netlist should have changed — `/VBAT`, `/VBUS`, `/BOOST_FB`, `/BUZZ_GATE`,
`/BUZZ_DRV`, `Net-(U3-SW)` etc. must be byte-identical to before.

Then run ERC. Expect the same `pin_to_pin` noise as before plus nothing new.

---

## Firmware pin map — old vs new

Five survive unchanged, thirteen move.

| Net | Old (MDBT50Q) | New (E73) | |
|---|---|---|---|
| `BUZZER_PWM` | P0.06 | P0.06 | same |
| `BTN_DOWN` | P0.12 | P0.12 | same |
| `BTN_ESTOP` | P0.15 | P0.15 | same |
| `LED_LINK` | P0.26 | P0.26 | same |
| `5V_EN` | P0.13 | P0.13 | same |
| `BTN_UP` | P0.11 | **P0.04** | moved |
| `BTN_SELECT` | P0.14 | **P0.08** | moved |
| `LED_PWR` | P0.27 | **P0.24** | moved |
| `ADC_VBAT` | P0.02 / AIN0 | **P0.05 / AIN3** | moved |
| `LORA_SCK` | P0.19 | **P1.10** | moved |
| `LORA_MOSI` | P0.20 | **P0.03** | moved |
| `LORA_MISO` | P0.21 | **P0.28** | moved |
| `LORA_NSS` | P0.22 | **P1.11** | moved |
| `LORA_RST` | P0.23 | **P0.29** | moved |
| `LORA_BUSY` | P0.24 | **P1.13** | moved |
| `LORA_DIO1` | P0.25 | **P0.02** | moved |
| `LORA_TXEN` | P1.00 | **P0.31** | moved |
| `LORA_RXEN` | P1.01 | **P0.30** | moved |
| `SWDIO` / `SWDCLK` / `SWD_RESET` | — | pins 37 / 39 / 26 | same |
| `USB_DP` / `USB_DN` / `VBUS` | — | pins 31 / 29 / 27 | same |

**Watch out:** `ADC_VBAT` moves from AIN0 to **AIN3** — the SAADC channel number
changes in firmware, not just the pin. And `LORA_DIO1` now sits on P0.02, which is
AIN0; that's fine as a digital interrupt, just don't let an ADC config grab it.

GPIO budget after this: 18 used, **11 clean spare** (+2 more if no crystal).

## Then, still open from the pre-order review

Independent of this swap, unchanged:

1. Add `D5` flyback — `1N4148W` `C81598`, **anode → `BUZZ_DRV`, cathode → `+5V`**,
   populated. Only remaining schematic edit that isn't the MCU.
2. Buzzer must be **passive** (`Passive (Externally Driven)` in the JLC
   description). `TMB12A05` and `TMB12A03` are both active — wrong for `tone()`.
   Candidate `C252922` GMC1209YB-42R2400. Footprint unchanged.
3. `QQ1` → `Q1`, `BZ3` → `BZ1`.
4. Confirm you own a TC2030-IDC-NL cable + retaining clip, or swap `J3` for a
   2.54 mm header while the board is still editable.
5. ERC cleanup: `BOOST_SW` label onto its wire, delete stub above `U2.6`, delete
   2 orphan NC flags, `PWR_FLAG` ×5.
6. Order a **stencil** with the PCBs if you're reflowing yourself — the E73 has
   15 inset pads, it is not an iron job.

Settled, do not re-open: TPS61023 has true load disconnect (rail-kill works);
L1 SWPA4020S1R0NT Isat 4.78 A (fine); all non-MCU wiring verified from netlist.
