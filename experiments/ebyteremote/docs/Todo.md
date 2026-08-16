# ParaWinch Remote v2 — TODO (authoritative)

**Last verified: 2026-08-12 00:05** against the **exported KiCad netlist**, plus live
JLCPCB/LCSC stock data and the manufacturers' own datasheets. This file supersedes
every earlier todo/review. If another document disagrees with this one, this one is
right — or re-verify from the netlist yourself.

**Board status: no board-killers remain.** The MCU has been swapped to the E73 and
verified pin-by-pin. What is left is one schematic edit, component bookkeeping,
ERC cleanup, and three ordering decisions.

Work order: **A → B → C → ERC → decisions → order.** Do not start layout until ERC is clean.

---

# ⛔ STOP — settled facts. Do not re-open these.

| Claim | Verdict |
|---|---|
| **MCU is the EBYTE `E73-2G4M08S1C`** (`C356849`) | **Settled.** It is *not* the MDBT50Q. The handoff buried this under a heading called "Fallback MCU"; the real gate was "MDBT50Q vs E73 from JLC stock check", and MDBT50Q stock is **0**. This was lost twice. |
| A fake `E73-2G4M08S1C` symbol exists | **Purged.** A global library `ParaWinchRemote` held hand-written stubs with 33 invented pins and empty Footprint fields. The real module has **43** pins. If you ever see a 33-pin E73, it's the fake. |
| TP4056 / TPS61023 / AP2112 / E22 / USB-C blocks "miswired" | **False.** Checked against the netlist. Acting on those claims breaks working circuits. |
| TPS61023 rail-kill may not work (old risk R1) | **Resolved.** TI datasheet: *"True disconnection between input and output during shutdown"*, 0.1 µA. Pulling `5V_EN` low genuinely kills the rail. |
| L1 inductor may saturate (old risk R3) | **Resolved.** `C91254` SWPA4020S1R0NT verified: 1 µH, **Isat 4.78 A**, Irms 2.15 A, 38 mΩ. Requirement was ≥3 A. |
| LED symbol pinouts are opposite each other | **True, and current wiring already respects it.** `parawinch:KT-0603R` (D1,D3) pin1=A. `parawinch:FC-1608GEK-572E` (D2,D4) pin1=K. Do not "correct" this. |
| E22 hand-solderable | **Yes.** 22 pads, all on the edge, zero interior. |
| Shorted nets | **None.** |

---

# ✅ Done this session

- **MCU swapped MDBT50Q → E73-2G4M08S1C.** `U5`, footprint
  `parawinch:WIRELM-SMD_E73-2G4M08S1C`, `LCSC Part` `C356849`, real datasheet URL.
  Symbol imported via `easyeda2kicad --full --lcsc_id=C356849`, all 43 pins
  cross-checked against Ebyte's *User Manual* section 3.
- **All 29 signal pins verified correct**, all 14 NC pins correct, **16 non-MCU nets
  byte-identical** to before the swap — zero collateral damage.
- Two faults found and fixed during the swap: pin 23 (VDDH) was on `SWD_RESET`,
  pin 24 (GND) was on `VBUS` — the second was a dead short, VBUS → module ground → GND.
- Earlier repairs, do not redo: boost L1/divider rewire · USB D− short to `BTN_UP` ·
  AP2112K EN off-grid wire · C4/C6/C7/C8/C9/C13 values · D2 part swap · C2/C6 to 0805 ·
  `J3` Tag-Connect footprint + NC on pin 6.

---

# A — Schematic edits

**One item.**

### A-1 · `D5` flyback diode not placed

The buzzer is a **coil**. The AO3400's drain flies above +5 V on every PWM edge,
thousands of times per beep, with nothing clamping it.

| | |
|---|---|
| Part | `parawinch:1N4148W_C81598` — LCSC `C81598`, SOD-123, 1.6 M in stock |
| Wiring | **anode → `BUZZ_DRV`**, **cathode → `+5V`** |
| Populate | **Yes.** Not DNP. |

If you switch to a passive *piezo* instead of a magnetic buzzer the diode becomes
unnecessary — place the footprint anyway and mark it DNP. Never leave the footprint off.

---

# B — Component / BOM

| # | Item | Fix |
|---|---|---|
| **B-1** | **`BZ3` buzzer must be PASSIVE.** Firmware plays melodies with a tone library, so the nRF generates the frequency. `TMB12A05` **and** `TMB12A03` are both verified `Active Buzzer (Built-in Driving Circuit)` — they self-oscillate at a fixed ~2.4 kHz and will only honk. | Buy a part whose JLC/LCSC description literally says **`Passive (Externally Driven)`**. Candidate: **`C252922`** GMC1209YB-42R2400 — 12 mm × 9 mm, 3–7 V, 50 mA, 85 dB, 1001 in stock. Confirm 7.6 mm lead pitch. |
| B-2 | Footprint for the buzzer | **No change needed.** `Buzzer_Beeper:Buzzer_12x9.5RM7.6` is the standard 12 mm / 7.6 mm-pitch through-hole pattern and fits both active and passive parts. This is a purchasing error, not a layout error. |
| B-3 | reference `QQ1` | rename to `Q1` |
| B-4 | reference `BZ3` | rename to `BZ1` |
| B-5 | 100 nF part | **`C14663`** CC0603KRX7R9BB104, 0603 50 V X7R, basic, 100 M stock. **Do NOT use `C1525`** — verified 0402, wrong package. |
| B-6 | 10 µF part | `C19702` CL10A106KP8NNNC, 0603 — verified |
| B-7 | `732k` (`R8`) | Not in JLC's basic library. Extended: `C23239` (4835) or `C245062` (1714). Fine if you buy reels yourself. |
| B-8 | `LCSC Part` field missing on all 24 R/C | Only matters if JLC assembles them. Field name must be exactly `LCSC Part`. Alternative: leave empty and match in JLC's parts picker at order time — their UI shows stock and package as you pick, which kills the whole `C1525` class of error. |
| B-9 | Hand-fit parts off the assembly BOM | `U6` E22 · `BZ1` buzzer · `J2` JST-PH · `J3` Tag-Connect (pads only, nothing soldered) |
| B-10 | `C5` and `U1` `lib_symbol_mismatch` | Tools → Update Symbols from Library. **Safe** — pin maps diffed identical, graphics only. |

---

# C — ERC gate. Clear before PCB.

| # | Item |
|---|---|
| C-1 | `BOOST_SW` label floating beside the wire, not on it — drag onto the wire between `U3 pin 5 (SW)` and `L1`. The net itself is already correct. |
| C-2 | Leftover wire stub above `U2 pin 6 (STDBY)`, top end dangling — delete. Harmless (crosses the EP/GND wire with no junction) but it hides real errors. |
| C-3 | 2 stray no-connect flags floating in the LED/buzzer area — delete both |
| C-4 | No `PWR_FLAG` anywhere — add on `GND`, `+5V`, `+3V3`, `VBAT`, `VBUS` |

**ERC noise — ignore.** Most `pin_to_pin` warnings come from the `E22` / `TP4056`
symbols declaring pins as *Unspecified*, so KiCad warns on every pairing. Same root
cause for `pin_not_driven` on `U6`/`U1`/`D2`. The `power_pin_not_driven` errors clear
after C-4. Target end state: only `pin_to_pin` left.

---

# D — Decide before you click order

### D-1 · No 32.768 kHz crystal — confirm your firmware agrees

Ebyte's manual, pins 11 and 13: **"Connect to 32.768 kHz crystal."** The E73 has
**no onboard LFXO** — the old "internal to module" note came from the MDBT50Q and
does not apply. Pins 11/13 are currently NC, so you have chosen *no crystal*.

**Action:** confirm your bootloader and SoftDevice are built for the internal RC
low-frequency source. If you'd rather fit it: 32.768 kHz 3215 crystal + 2 × 12 pF
0603 on pins 11/13, ~2 SEK, better BLE timing and lower sleep current. **Adding it
later means a new board.**

### D-2 · Can you actually flash it?

`J3` is `TC2030-IDC-NL` = **no connector is soldered at all**, just pads and
alignment holes. The E73 ships blank and Ebyte's manual is explicit: **J-Link / SWD
is the only programming path** — no serial, no ISP.

**Action:** confirm you own a **TC2030-IDC-NL cable (~€40) + retaining clip (~€25)
+ a J-Link or CMSIS-DAP**. If not, swap `J3` for a 4-pin 2.54 mm header *now*, while
the board is still editable. Otherwise you get five bricks.

### D-3 · Assembly strategy → do you need a stencil?

The E73 has 43 pads: 28 castellated edge, **15 inset 2.1 mm under the body**. It is
**not** an iron job. Neither is `U3` TPS61023 (SOT-563, 0.5 mm pitch) or `U2` TP4056
(thermal pad underneath).

Two viable paths:

- **JLC assembly.** Now possible — E73 is in their library with 1525 stock, ~$8.13/1,
  ~$6.36/30. Cleanest.
- **Reflow yourself.** Order a **stencil with the PCBs** — that is a checkbox at
  order time and cannot be added later. Then hand-solder `U6` E22, `BZ1`, `J2`.

---

# E — PCB / layout (only after A–D + ERC are clean)

| # | Item |
|---|---|
| E-1 | `C5` 470 µF bulk physically adjacent to the E22 VCC pin — TX bursts 650 mA |
| E-2 | Boost switching loop `U3 SW → L1 → C6 → GND` kept tight and small |
| E-3 | Ground plane continuity; `U2` TP4056 EP needs a copper pour for thermal (~0.65 W at 500 mA charge) |
| E-4 | **E73 antenna keep-out** — built-in ceramic antenna, module is 13 × 18 mm with the antenna at one end. Overhang the board edge, no copper under it. |
| E-5 | E22 ANT stamp pad: pad only, **no trace**. RF exits via the module's IPEX to the FXP830. |
| E-6 | E22 GND stamp-holes: thermal relief. Module runs warm at 1 W. |
| E-7 | Trace widths: +5 V rail for 650 mA, VBAT for 500 mA charge |
| E-8 | CPL rotations checked against JLC convention for every placed part |
| E-9 | DRC clean |
| E-10 | 2-layer is fine (no RF on the board); 4-layer is a small price delta for ground integrity |

---

# F — Firmware notes (no board change)

### F-1 · E22 GPIO sequencing — required

`5V_EN` has a 100 k pulldown, so at boot the 5 V rail is down while the nRF is up at
3.3 V. Driving `NSS`/`SCK`/`MOSI`/`RST`/`TXEN`/`RXEN` into an unpowered E22 pushes
current through its input ESD diodes into its VCC rail — parasitic powering and a
latch-up path. The wiring is correct; the fix is sequencing:

- hold all E22 GPIOs low or hi-Z before dropping the rail
- keep them hi-Z until the rail is up and settled
- reverse order on wake

### F-2 · ADC channel changed

`ADC_VBAT` moved from **AIN0 → AIN3** (P0.02 → P0.05). The SAADC *channel* changes,
not just the pin. And `LORA_DIO1` now sits on P0.02 which is AIN0 — fine as a
digital interrupt, just don't let an ADC config claim it.

### F-3 · Startup into 470 µF

With true disconnect the +5 V rail starts from 0 V and must charge ~490 µF against a
3.7 A limit. It will probably just current-limit, but it could hiccup. Value change
only, footprint unaffected — test on unit 1, drop `C5` to 220 µF if it misbehaves.

### F-4 · Regulatory

E22-900M30S is 30 dBm (1 W). Swedish/ETSI 868 MHz allows 500 mW ERP at 10 % duty in
869.4–869.65. **Turn the power down in firmware.**

---

# GPIO map — E73-2G4M08S1C

Verified against Ebyte *User Manual* §3 and the exported netlist.

| Pin | Symbol | Net | | Pin | Symbol | Net |
|---|---|---|---|---|---|---|
| 1 | P1.11 | `LORA_NSS` | | 23 | VDH (VDDH) | `+3V3` |
| 2 | P1.10 | `LORA_SCK` | | 24 | GND | `GND` |
| 3 | P0.03 | `LORA_MOSI` | | 25 | DCH (DCCH) | NC |
| 4 | AI4 (P0.28) | `LORA_MISO` | | 26 | RST (P0.18) | `SWD_RESET` |
| 5 | GND | `GND` | | 27 | VBS (VBUS) | `VBUS` |
| 6 | P1.13 | `LORA_BUSY` | | 28 | P15 (P0.15) | `BTN_ESTOP` |
| 7 | AI0 (P0.02) | `LORA_DIO1` | | 29 | D− | `USB_DN` |
| 8 | AI5 (P0.29) | `LORA_RST` | | 30 | P17 (P0.17) | NC |
| 9 | AI7 (P0.31) | `LORA_TXEN` | | 31 | D+ | `USB_DP` |
| 10 | AI6 (P0.30) | `LORA_RXEN` | | 32 | P0.20 | NC |
| 11 | XL1 (P0.00) | NC | | 33 | P0.13 | `5V_EN` |
| 12 | P0.26 | `LED_LINK` | | 34 | P0.22 | NC |
| 13 | XL2 (P0.01) | NC | | 35 | P0.24 | `LED_PWR` |
| 14 | P0.06 | `BUZZER_PWM` | | 36 | P1.00 | NC |
| 15 | AI3 (P0.05) | `ADC_VBAT` | | 37 | SWD (SWDIO) | `SWDIO` |
| 16 | P0.08 | `BTN_SELECT` | | 38 | P1.02 | NC |
| 17 | P1.09 | NC | | 39 | SWC (SWDCLK) | `SWDCLK` |
| 18 | AI2 (P0.04) | `BTN_UP` | | 40 | P1.04 | NC |
| 19 | VCC (VDD) | `+3V3` | | 41 | NF1 (P0.09) | NC |
| 20 | P12 (P0.12) | `BTN_DOWN` | | 42 | P1.06 | NC |
| 21 | GND | `GND` | | 43 | NF2 (P0.10) | NC |
| 22 | P0.07 | NC | | | | |

**18 GPIO used, 11 clean spare** (13 counting XL1/XL2).

---

# Gotchas that have already bitten this project

1. **A symbol with an empty Footprint field and a `~` datasheet was improvised, not
   imported.** That is how the 33-pin fake E73 got placed. **Check pin count against
   the manufacturer manual before wiring anything** — it catches this in five seconds.
2. **Never trust a C-number without checking the package.** `C1525` was recommended
   for 100 nF and is a 0402 part. Validate with
   `GET https://easyeda.com/api/products/<C-number>/components?uuid=&version=6.4.19.5`
   (desktop User-Agent, `dangerouslyDisableSandbox` on the Bash call). For stock and
   basic/extended: POST `https://jlcpcb.com/api/overseas-pcb-order/v1/shoppingCart/smtGood/selectSmtComponentList`
   with `{"currentPage":1,"pageSize":20,"keyword":"...","searchSource":"search"}`.
3. **"Buzzer" hides the only spec that matters** — active vs passive. Both TMB12A03
   and TMB12A05 are active.
4. **Swapping a symbol clears its Footprint field.**
5. **`get_component_nets` returns `null` for pins on unlabeled local nets.** Tool
   limitation, not a disconnection. Export the netlist for truth. Generic `Device:R` /
   `Device:C` pins have no `pinfunction` field — a parser requiring it silently drops
   every passive.
6. **`kicad-cli` is not on PATH.** `C:\Program Files\KiCad\10.0\bin\kicad-cli.exe`
7. **Datasheet PDFs defeat WebFetch.** Download with curl and parse with `pypdf`
   (installed) — that is how the Ebyte pin table was recovered.

---

# Tick sheet

```
A   [ ] A-1  D5 added: 1N4148W C81598, A->BUZZ_DRV, K->+5V, POPULATED

B   [ ] B-1  passive buzzer part picked (NOT TMB12A03/A05)
    [ ] B-3  QQ1 -> Q1
    [ ] B-4  BZ3 -> BZ1
    [ ] B-8  "LCSC Part" on all R/C   (or decide to match at order time)
    [ ] B-10 C5 + U1 updated from library

C   [ ] C-1  BOOST_SW label onto the wire
    [ ] C-2  delete stub above U2 STDBY
    [ ] C-3  delete 2 stray NC flags
    [ ] C-4  PWR_FLAG x5
    [ ] ERC re-run — only pin_to_pin noise should remain

D   [ ] D-1  no-crystal decision confirmed against bootloader/SoftDevice LF source
    [ ] D-2  Tag-Connect cable + clip in hand, OR J3 swapped to 2.54mm header
    [ ] D-3  assembly path chosen; stencil added to the order if self-reflowing

PCB ---- only after everything above ----
```
