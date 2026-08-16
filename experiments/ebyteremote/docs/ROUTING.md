# ParaWinch Remote v2 — PCB setup & routing rules

Derived from the exported netlist and the real current budget, not rules of thumb.
2-layer, 1 oz copper, JLCPCB economy.

---

## Current budget

| Rail | Worst case | Where it comes from |
|---|---|---|
| **`VBAT`** | **1.71 A** | boost input 1.11 A **+** 600 mA charge, concurrent (USB plugged in while transmitting) |
| `+5V` | 660 mA | E22 at 30 dBm TX (650 mA) + piezo (10 mA) |
| `VBUS` | ~650 mA | TP4056 charge current + USB |
| `+3V3` | ~25 mA | nRF52840 BLE TX + LEDs |
| signals | mA | — |

Boost input: `5 V × 0.66 A / (0.90 × 3.3 V) = 1.11 A`. **`VBAT` is the highest-current
net on the board, not `+5V`** — easy to get backwards.

⚠️ **Charge current is 600 mA, not 500 mA.** `R3` = 2 kΩ, and TP4056 gives
`I = 1200 / R_PROG = 1200 / 2000 = 0.6 A`. Earlier notes said 500 mA. If your cell is
under 600 mAh that's above 1C — raise `R3` to 2.4 kΩ (500 mA) or 3 kΩ (400 mA) if so.

## Trace width reference — IPC-2221, external layer, 1 oz, 10 °C rise

```
0.20 mm  →  0.74 A        0.60 mm  →  1.64 A
0.25 mm  →  0.87 A        0.80 mm  →  2.03 A
0.30 mm  →  0.99 A        1.00 mm  →  2.38 A
0.40 mm  →  1.23 A        1.50 mm  →  3.20 A
0.50 mm  →  1.44 A
```

---

## Net classes — Board Setup → Net Classes

| Class | Nets | Track | Via (pad/drill) | Why |
|---|---|---|---|---|
| **HighCurrent** | `VBAT`, `+5V` | **1.00 mm** | 0.8 / 0.4 | 1.71 A and 0.66 A; width also buys low impedance for the boost |
| **Power** | `VBUS`, `+3V3` | **0.60 mm** | 0.8 / 0.4 | 650 mA charge path; +3V3 is light but wants low Z |
| **Default** | everything else | **0.25 mm** | 0.6 / 0.3 | 0.87 A capacity, far beyond any signal here |

`GND` is a **pour on both layers**, not a class width. Stitch the two pours with vias
generously, especially around the boost and under the E22.

## Design rules — Board Setup → Constraints

```
Min track width            0.20 mm
Min clearance              0.20 mm
Min via                    0.60 mm pad / 0.30 mm drill
Min annular ring           0.13 mm
Min hole-to-hole           0.50 mm
Copper to board edge       0.30 mm
```

JLC's economy tier can do 0.127 mm/0.127 mm, so 0.20 mm everywhere leaves comfortable
margin and keeps you in the cheapest bracket.

---

## Six nets that need manual attention

### 1. `Net-(U3-SW)` — boost switch node. The single most important trace on the board.

`U3` pin 5 → `L1` pin 2. Keep it **short and small in area**. This node slews volts in
nanoseconds; every mm² radiates. Do **not** widen it beyond ~0.5 mm — extra copper here
buys nothing and adds capacitive coupling.

The loop that matters is `U3 SW → L1 → C6 → GND → U3 GND`. Make that physical loop as
tight as you can. Place `L1` and `C6` right against `U3`, ground return directly
underneath. If you get one thing right in this layout, make it this.

### 2. `Net-(U5-XL1)` / `Net-(U5-XL2)` — 32.768 kHz crystal

`X2` hard against `U5` pins 11 and 13, `C19`/`C20` right beside it. Short, direct,
no vias. Ground pour around and under the whole block, stitched. Keep it **far from the
boost switch node and from `BUZZ_DRV`** — this is a 32 kHz, high-impedance node and it
will happily pick up switching noise.

DNP for now, but route it properly anyway — you get one shot at the copper.

### 3. `USB_DP` / `USB_DN` — differential pair

Route as a pair, keep them together, roughly equal length, ground beneath. Don't agonise
over 90 Ω on 2-layer — this is USB **Full Speed at 12 Mbps** and it's very forgiving.
Short and paired beats impedance-perfect and long.

### 4. `ADC_VBAT` — high impedance, easy to ruin

Source impedance is `R6 ∥ R7` ≈ **248 kΩ**. That is a genuine antenna for injected noise.
Keep `C3` (100 nF) hard against `U5` pin 15, route the node short, and keep it away from
the switch node, `BUZZ_DRV`, and the E22.

### 5. `ANT` — `U6` pin 21

**Pad only. No trace, no pour, no via.** RF exits the E22 via its IPEX connector to the
external antenna. A stub here does nothing good.

### 6. `BUZZ_DRV`

Only 10 mA now that the buzzer is a piezo (was going to be 80 mA magnetic). 0.30 mm is
plenty. Keep `D5` physically tight to the buzzer pads — a long clamp loop defeats the
diode.

---

## Placement constraints — get these right before routing

| # | Item |
|---|---|
| P-1 | **E73 antenna keep-out.** Module is 13 × 18 mm with a **built-in ceramic antenna** at one end. That end must overhang the board edge, with **no copper on any layer** — no traces, no pour, no vias — under or beside the antenna section. This is the one placement error you cannot fix after fab. |
| P-2 | **`C5` 470 µF hard against the E22 `VCC` pins (9, 10).** TX pulls 650 mA in bursts; the bulk cap has to be local or the rail sags. `C13`/`C14` decoupling beside it. |
| P-3 | **`U2` TP4056 exposed pad → copper pour + thermal vias.** Worst case dissipation is `(5 V − 3.0 V) × 0.6 A ≈ 1.2 W` at the start of charge on an ESOP-8. It needs real copper or it will thermally fold back. |
| P-4 | **E22 GND stamp holes** — thermal relief on the pour, module runs warm at 1 W. |
| P-5 | **`U5` decoupling** — one 100 nF at pin 19 (`VCC`), one at pin 23 (`VDDH`), as close as the footprint allows. `C8`/`C9` 10 µF nearby. |
| P-6 | **Boost cluster** `U3`, `L1`, `C6`, `C7` grouped tightly, ground return directly beneath. See net 1. |
| P-7 | `J1` USB-C at a board edge; `J3` SWD header reachable with the board in its enclosure. |

---

## Order of work

1. Board outline + stackup (2-layer, 1.6 mm, 1 oz)
2. Constraints + net classes above
3. Update PCB from Schematic
4. **Placement** — P-1 through P-7. Do not route until placement is settled.
5. Route power first (`VBAT`, `+5V`, `VBUS`), then the boost loop, then signals
6. Ground pour both layers, stitch
7. DRC
8. CPL rotations checked against JLC convention
9. Gerbers + BOM + CPL — **add the stencil to the order if self-reflowing**
