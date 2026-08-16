# ParaWinch Handheld Remote — PCB Handoff (Schematic Phase)

Status: architecture locked. Next step: KiCad schematic, block by block.
Owner: Edvin (Utskottet). Target fab: JLCPCB, 5 units, economy SMD assembly.
Modules hand-soldered by Edvin. Cost-optimized: minimize extended parts, all passives 0603 basic lib.

## System summary

Handheld remote for paragliding winch (1500 m tow). nRF52840 for BLE (app) +
SX1262 LoRa 868 MHz uplink to winch. Operating at 869.525 MHz, 27 dBm ERP legal
sub-band (EN 300 220, 500 mW ERP, 10% duty). E22 runs up to 30 dBm conducted;
handheld antenna ~0 to −2 dBi keeps ERP compliant. Winch end gets its own
E22 + pole dipole (separate project, not this board).

## Locked decisions

- MCU: **MDBT50Q-1MV2** (nRF52840, integrated BLE antenna). JLC C2857784 if
  stocked → let JLC place it (priority even at extended fee). Else hand-solder.
- LoRa: **E22-900M30S** (SX1262 + PA, 30 dBm). Hand-soldered stamp-hole module,
  buy from Ebyte official AliExpress store.
- **Antenna: FXP830 (Taoglas FPC, 868 MHz) plugs directly into the E22's onboard
  IPEX connector. NO U.FL receptacle on PCB, NO RF trace on PCB.** Leave module
  ANT stamp-hole pad unconnected (pad only, no trace). Verify E22 batch has
  MHF1/IPEX-1 connector (FXP830 pigtail is MHF1) before ordering.
- No RF layout constraints remain → 2-layer PCB acceptable. 4-layer optional
  (JLC price delta small) for ground integrity; designer's choice.
- USB-C: charging + native nRF52840 USB (UF2/DFU + CDC debug). No USB-UART chip.
- Buzzer: loud through-hole active piezo, 5 V, ~95 dB class. Hand-soldered.
- Power switch: none mechanical. TPS61023 EN via nRF GPIO kills 5 V rail
  (E22 + buzzer). nRF sleeps at µA, wake on button.

## Power architecture

```
USB-C 5V ──► TP4056 (500 mA) ──► 1S LiPo (protected cell, 1000–1500 mAh, JST-PH)
                                      │
                                      ├─► TPS61023 boost → 5 V rail → E22 VCC, buzzer
                                      │     EN ← nRF GPIO
                                      │     ≥470 µF low-ESR bulk at E22 (TX bursts 650 mA)
                                      ├─► AP2112K-3.3 → 3.3 V rail → nRF52840
                                      └─► 1M/330k divider + 100 nF → nRF ADC (VBAT sense)
```

- Protected cell chosen → DW01A/FS8205A omitted.
- E22 wants ≥5 V for full 30 dBm (accepts 3.3–5.5 V; PA saturates lower at 3.3 V).
- TX current up to ~650 mA at 30 dBm → boost inductor 1 µH, ≥3 A Isat.
- E22 logic levels: confirm IO tolerance — module logic is 3.3 V typ; nRF GPIO
  3.3 V direct connect OK, VCC at 5 V. Double-check datasheet §logic levels.

## BOM (JLC-assembled, SMD)

| Ref block | Part | LCSC | Lib | Notes |
|---|---|---|---|---|
| Charger | TP4056 | C382139 | Ext | PROG 2k → 500 mA. NTC pin: tie per datasheet if unused |
| Boost | TPS61023DRLR | C919459 | Ext | |
| Inductor | 1 µH ≥3 A Isat | pick in-stock | Ext | verify Isat |
| LDO | AP2112K-3.3 | C51118 | Ext | |
| USB-C | 16P TYPE-C-31-M-12 | C165948 | Ext | 2× 5.1k CC pulldown (C25905) |
| ESD | SRV05-4 | C85364 | Ext | on D+/D−/VBUS |
| FET | AO3400 ×1 | C20917 | Basic | buzzer low-side drive |
| Diode | 1N4148WS | C81598 | Basic | only if magnetic buzzer; omit for piezo |
| LEDs | 0603 R/G/B | C2286/C72043/C72041 | Basic | charge STAT, link, power. 1k series |
| Buttons | 4× tactile SMD | basic-lib match | Basic | Up/Down/Select/E-stop |
| R/C | all 0603 | C1525 (100n), C19702 (10µ), etc. | Basic | 100 nF per supply pin |
| Bulk cap | ≥470 µF low-ESR polymer | verify stock | Ext | at E22 VCC |

Extended count target: ≤7 (+MDBT50Q if JLC places it).

## Hand-soldered by Edvin

- E22-900M30S (stamp-hole, drag solder)
- MDBT50Q-1MV2 (castellated) — only if not in JLC stock
- Piezo buzzer TH, 5 V active
- JST-PH 2-pin battery connector
- FXP830 antenna: adhesive mount in enclosure + IPEX plug, no solder

## nRF52840 pin budget

| Function | Pins |
|---|---|
| E22 SPI (SCK/MOSI/MISO) | 3 |
| E22 NSS, NRESET, BUSY, DIO1 | 4 |
| E22 TXEN, RXEN | 2 (external PA switching — DIO2 auto-switch NOT available on this module) |
| Buttons | 4 (use sense/wake-capable pins; E-stop on dedicated interrupt) |
| LEDs | 2 (charge STAT LED wired to TP4056 directly, not nRF) |
| Buzzer FET gate | 1 (PWM-capable) |
| TPS61023 EN | 1 |
| VBAT ADC | 1 (AIN) |
| USB D+/D− | native pins |
| SWD | SWDIO/SWDCLK (Tag-Connect TC2030 footprint, no component) |

Total GPIO ≈ 18. Fine for MDBT50Q. Avoid P0.09/P0.10 (NFC) unless CONFIG'd,
avoid pins near the module's antenna keep-out per Raytac datasheet.

## Layout notes (for later, but affects schematic ref planning)

- MDBT50Q antenna keep-out: follow Raytac guideline — module antenna end
  overhangs board edge or has copper keep-out zone.
- E22 placement: thermal relief on ground stamp-holes; module gets warm at 1 W.
- Bulk cap physically adjacent to E22 VCC pin.
- FXP830 sits in enclosure lid — pigtail length 100 mm, plan module orientation
  so IPEX faces antenna side.
- Tag-Connect TC2030-NL footprint for SWD.

## Open items / verify before schematic freeze

1. MDBT50Q-1MV2 stock at JLC (C2857784) — decides assembly vs hand-solder.
2. E22 batch IPEX connector generation = MHF1 (ask Ebyte seller).
3. E22 logic-level spec at VCC=5 V with 3.3 V GPIO (datasheet check).
4. Buzzer part selection: TH active piezo 5 V ~95 dB — pick exact part,
   confirm hole pitch for footprint.
5. Inductor + bulk cap: pick from JLC in-stock at order time.
6. TP4056 vs BQ21040: TP4056 chosen for cost/availability. Revisit only if
   thermal charge cutoff wanted (remote lives in pockets, −20…+35 °C ambient).
7. Enclosure: not started. Board outline TBD — suggest defining rough outline
   (buttons on face, USB-C bottom, antenna in lid) before placement.

## Firmware notes (downstream)

- UF2 bootloader (Adafruit nRF52) → drag-drop firmware over USB, plus BLE DFU.
- LoRa params per earlier work: 869.525 MHz, SF9 up (commands), SF11 down
  (telemetry), command repetition 3×, asymmetric link.
- RadioLib supports SX1262 with external TXEN/RXEN control (setRfSwitchPins).

---

# CONNECTION SPEC (authoritative netlist for schematic entry)

Written by planning agent. Next agent: implement in KiCad exactly as specified.
GPIO assignments use nRF52840 port.pin names — portable across MDBT50Q-1MV2 and
E73-2G4M08S1C (both expose these). **VERIFY each GPIO is broken out on the
chosen module's pads against its datasheet before freeze. Do not silently
remap; flag any conflict back.**

E22 signal names are per Ebyte E22-900M30S user manual. **Verify physical pin
numbers against manual v1.2 — do not trust memory or third-party footprints.**

## Nets

### Power

| Net | Connections |
|---|---|
| VBUS | USB-C VBUS pins (A4/B9/A9/B4), TP4056 VCC(4), SRV05-4 VBUS ch, C_vbus 10µF→GND |
| VBAT | TP4056 BAT(5), JST-PH pin 1 (+), TPS61023 VIN, AP2112K VIN(1), R_div_top 1M, C_vbat 22µF→GND |
| +5V | TPS61023 VOUT, E22 VCC, buzzer +, C_bulk 470µF poly→GND, C_5v 10µF→GND |
| +3V3 | AP2112K VOUT(5), nRF VDD (all VDD pads), C_3v3 10µF + 100nF per VDD pad→GND |
| GND | common. USB-C GND+shield, TP4056(3), JST-PH pin 2, all module GND pads, E22 GND stamp-holes (thermal relief) |

### Charger (TP4056, SOP-8)

| TP4056 pin | Net/Component |
|---|---|
| 1 TEMP | → GND (NTC disabled) |
| 2 PROG | → R_prog 2.0k → GND (≈500 mA) |
| 3 GND | GND |
| 4 VCC | VBUS |
| 5 BAT | VBAT |
| 6 STDBY̅ | → LED_charge_green cathode side: +3V3? **NO** — wire LED string VBUS→R 1k→LED→STDBY̅ |
| 7 CHRG̅ | VBUS→R 1k→LED_red→CHRG̅ |
| 8 CE | → VBUS (enable) |

Charge LEDs are powered from VBUS, not nRF — they only matter when cable in.

### Boost (TPS61023, SOT-563)

| Pin | Net |
|---|---|
| VIN | VBAT |
| SW | L1 1µH → VOUT node |
| VOUT | +5V |
| EN | nRF **P0.13** (net 5V_EN), plus 100k pulldown→GND (rail off at boot) |
| GND | GND |
| FB | internal (fixed 5V variant TPS610233? ) — **verify: TPS61023 is adjustable, needs R divider VOUT→FB→GND. Use R_fb1 787k / R_fb2 100k for ≈5.0 V (Vref 0.6 V → check datasheet formula V=0.6·(1+R1/R2); 732k/100k = 4.99 V. Next agent: confirm ratio from TI datasheet, E12/E24 nearest).** |

### LDO (AP2112K-3.3, SOT-23-5)

Pin 1 VIN=VBAT, 2 GND, 3 EN→VBAT, 4 NC, 5 VOUT=+3V3. 1µF in/out minimum (use 10µF).

### VBAT sense

VBAT → R 1M → node ADC_VBAT → R 330k → GND. C 100nF node→GND.
ADC_VBAT → nRF **P0.02 (AIN0)**.

### E22-900M30S

| E22 signal | Net | nRF pin |
|---|---|---|
| VCC | +5V | — |
| GND (all) | GND | — |
| SCK | LORA_SCK | P0.19 |
| MOSI | LORA_MOSI | P0.20 |
| MISO | LORA_MISO | P0.21 |
| NSS | LORA_NSS | P0.22 |
| NRST | LORA_RST | P0.23 |
| BUSY | LORA_BUSY | P0.24 |
| DIO1 | LORA_DIO1 | P0.25 |
| TXEN | LORA_TXEN | P1.00 |
| RXEN | LORA_RXEN | P1.01 |
| ANT (stamp pad) | no connect — pad only, no trace. RF exits via module IPEX |
| DIO2, other pins | leave NC per manual |

**Logic levels: E22 IO is 3.3 V-tolerant per Ebyte manual even at VCC=5 V
(logic 3.3 V typ) — next agent re-verify §"logic level" in manual v1.2. If
manual contradicts, insert TXB-type level shifter — do not proceed silently.**

### Buttons (4×, active-low to GND, internal pull-ups in nRF)

| Button | Net | nRF pin |
|---|---|---|
| BTN_UP | P0.11 |
| BTN_DOWN | P0.12 |
| BTN_SELECT | P0.14 |
| BTN_ESTOP | P0.15 |

No external pull-ups. Optional 100nF pad each button→GND (DNP default).
All four must be on wake-capable GPIO (any nRF52840 GPIO qualifies via sense).

### LEDs (nRF-driven; charge LEDs handled at TP4056)

| LED | Net | nRF pin | Note |
|---|---|---|---|
| LED_LINK (blue) | P0.26 | +3V3→R 1k→LED→pin? **No: drive pin high = on. Wire pin→R 1k→LED→GND** |
| LED_PWR (green) | P0.27 | pin→R 1k→LED→GND |

### Buzzer

+5V → buzzer(+); buzzer(−) → AO3400 drain; source→GND; gate → R 100Ω → nRF
**P0.06** (PWM); gate 10k pulldown→GND. Flyback 1N4148WS across buzzer,
cathode to +5V — **fit for magnetic buzzer; DNP if final part is piezo.**
Footprint: TH, 7.6 mm pitch typical 12 mm piezo — set after part selection
(open item 4).

### USB data

USB-C A6/B6 D+ → nRF D+ pad; A7/B7 D− → nRF D− pad (tie A/B pairs together).
SRV05-4 on D+, D−, VBUS. No series resistors (nRF internal).
CC1→5.1k→GND, CC2→5.1k→GND.

### SWD

Tag-Connect TC2030-NL footprint: SWDIO, SWDCLK, nRF RESET pad (P0.18/reset),
+3V3, GND. No components.

### nRF misc

- P0.09/P0.10 (NFC): unused, leave NC (firmware sets CONFIG as GPIO not needed)
- Module 32.768 kHz: internal to module — no external xtal
- VDDH/USB power config: MDBT50Q normal-voltage mode, VDD=3.3 V; VBUS pad of
  nRF → USB-C VBUS via module's VBUS pad (required for USB detect).
  **Verify module exposes VBUS pad; connect per Raytac reference schematic.**

## GPIO summary (all assignments)

| Pin | Function |
|---|---|
| P0.02 | ADC_VBAT (AIN0) |
| P0.06 | BUZZER_PWM |
| P0.11 | BTN_UP |
| P0.12 | BTN_DOWN |
| P0.13 | 5V_EN |
| P0.14 | BTN_SELECT |
| P0.15 | BTN_ESTOP |
| P0.19–P0.25 | LoRa SPI + control (see table) |
| P0.26 | LED_LINK |
| P0.27 | LED_PWR |
| P1.00, P1.01 | LORA_TXEN, LORA_RXEN |

## Instructions to next agent

1. Resolve open items 1–5 (main file above) before symbol placement.
2. Confirm MCU module (MDBT50Q vs E73) from JLC stock check → pick footprint.
   If E73: re-verify every GPIO above exists on its stamp pads; report conflicts.
3. Pull footprints: easyeda2kicad for LCSC parts; Ebyte/Raytac modules need
   manual footprint from datasheet mechanical drawing — measure twice.
4. Verify TPS61023 FB divider math against TI datasheet (flagged above).
5. Run ERC; every net in this spec must appear; no extra nets without comment.
6. Deliverable: KiCad 8 project, schematic only. Placement/routing is a later
   phase (board outline pending enclosure sketch — open item 7).

---

# CAD SOURCES — symbols, footprints, 3D models

**Correction to earlier BOM: MDBT50Q-1MV2 LCSC/JLC part number is C5118826,
not C2857784.** Package SMD-61P (61 pads incl. underside — confirms
assembly-only, do not hand-solder). EasyEDA has symbol+footprint for it.

## Primary workflow: easyeda2kicad

All LCSC-numbered parts pull symbol + footprint + 3D model directly from the
JLC/EasyEDA library — these footprints are by definition JLC-assembly
compatible (correct rotation/origin for their pick-and-place).

```
pip install easyeda2kicad
easyeda2kicad --full --lcsc_id=C5118826 --output ./lib/parawinch   # MDBT50Q-1MV2
easyeda2kicad --full --lcsc_id=C382139  --output ./lib/parawinch   # TP4056
easyeda2kicad --full --lcsc_id=C919459  --output ./lib/parawinch   # TPS61023DRLR
easyeda2kicad --full --lcsc_id=C51118   --output ./lib/parawinch   # AP2112K-3.3
easyeda2kicad --full --lcsc_id=C165948  --output ./lib/parawinch   # USB-C 16P
easyeda2kicad --full --lcsc_id=C85364   --output ./lib/parawinch   # SRV05-4
easyeda2kicad --full --lcsc_id=C20917   --output ./lib/parawinch   # AO3400
easyeda2kicad --full --lcsc_id=C81598   --output ./lib/parawinch   # 1N4148WS
easyeda2kicad --full --lcsc_id=C2286    --output ./lib/parawinch   # LED red 0603
easyeda2kicad --full --lcsc_id=C72043   --output ./lib/parawinch   # LED green 0603
easyeda2kicad --full --lcsc_id=C72041   --output ./lib/parawinch   # LED blue 0603
```

Passives (0603 R/C): use KiCad built-in symbols + `Resistor_SMD:R_0603_1608Metric`
/ `Capacitor_SMD:C_0603_1608Metric`. Assign LCSC IDs as `LCSC` field for the
JLC BOM export (jlc-kicad-tools or Fabrication Toolkit plugin). Buttons and
inductor: pick in-stock basic-lib parts at order time, pull with easyeda2kicad
the same way.

## E22-900M30S (not JLC-assembled — hand-soldered, footprint still on our PCB)

- Datasheet/manual (pin numbers — authoritative): https://www.ebyte.com/en/product-view-news.html?id=437
  (or manual PDF via ebyte.com E22-900M30S product page; also
  http://www.ebyte.com/en/downpdf.aspx?id=453)
- Symbol+footprint+3D, SnapMagic (KiCad export):
  https://www.snapeda.com/parts/E22-900M30S/EBYTE/view-part/
- Alternative, already-KiCad-8, used in a working JLC-fabbed design:
  https://github.com/ndoo/ikoka-nano-meshtastic-device (vendors SnapMagic
  E22-900M30S footprint; project confirms it passes JLC DRC)
- **Verify footprint pad numbering against Ebyte manual mechanical drawing
  before use — SnapMagic parts are usually right, not always.**
- ANT stamp pad: keep in footprint, no net (RF exits via module IPEX).

## FXP830 antenna

No PCB footprint needed (adhesive mount + IPEX plug to module). Datasheet for
mechanical/keep-out planning:
https://www.taoglas.com/product/fxp830-cirrus-868mhz-fpc-antenna/

## JST-PH battery connector (hand-soldered)

S2B-PH-K-S (side entry TH) — LCSC C173752 (**verify ID**), or KiCad built-in
`Connector_JST:JST_PH_S2B-PH-K_1x02_P2.00mm_Horizontal` + JST datasheet.
Not JLC-assembled, so KiCad built-in footprint is fine.

## Tag-Connect SWD

KiCad built-in: `Connector:Tag-Connect_TC2030-IDC-NL_2x03_P1.27mm_Vertical`
(no component, footprint only). Official drawings: tag-connect.com if needed.

## MDBT50Q references

- JLC part page (stock check + datasheet): https://jlcpcb.com/partdetail/RAYTAC-MDBT50Q1MV2/C5118826
- Raytac product page (reference schematic, antenna keep-out, layout guide):
  https://www.raytac.com/product/ins.php?index_id=24
- The easyeda2kicad pull gives the JLC-compatible footprint; cross-check
  antenna keep-out zone against Raytac spec v1.x mechanical section and add
  a keep-out area object in the PCB accordingly.

## Fallback MCU (only if C5118826 out of stock): E73-2G4M08S1C

- Buy: Ebyte official AliExpress store (same order as E22)
- Symbol/footprint: SnapMagic https://www.snapeda.com/parts/E73-2G4M08S1C/EBYTE/view-part/
  (**verify against Ebyte E73 manual pad drawing**)
- If this path: re-map GPIO table against E73 pad breakout, report conflicts.

## JLC BOM/CPL export

Use "Fabrication Toolkit" KiCad plugin (or jlc-kicad-tools). Every
JLC-assembled symbol must carry an `LCSC` field with the C-number. Parts
without LCSC field (E22, JST, buzzer, Tag-Connect) are excluded from assembly
BOM automatically — mark them DNP in assembly, populate in schematic.
