# Winch Upgrade Plan
# Status: planning only -- nothing here is implemented on the real system

All numbers in this document are derived from real calibration data in specifications.md
but applied to configurations that do not yet exist on the machine.

---

## Open Items

### Completed 2026-04-27
- [x] Crane scale static pull test -- log 2 (20-80A) and log 3 (20-140A) done
- [x] Correct VESC wheel diameter from 280 mm to 265 mm
- [x] Correct VESC gear ratio from 4.180 to 4.091
- [x] VESC motor current max / abs max / battery max confirmed
- [x] VESC ERPM max / min confirmed
- [x] Confirm gear ratio -- 22T/90T = 4.091 physically counted and verified by RPM test
- [x] Confirm pole pairs = 5 by RPM measurement test
- [x] Confirm Kv = 277 ERPM/V from flux linkage (reliable); discarded noisy 257 value

### Completed 2026-05-02
- [x] Fix remote battery ADC reference (3.3f → 3.6f) and percent floor (3.40V → 3.00V)
- [x] Add deep sleep (System OFF ~0.4µA) with 30 min idle timeout, wake on button press
- [x] Sleep/wake sounds, BLE sleep countdown to app (v2.0)
- [x] GPS data quality hardened — stale position cleared on fix loss or XCT disconnect (v2.1)
- [x] Wake Lock added to app to prevent BLE drop on phone screen-off (v2.2)

### Winch telemetry -- incomplete data pipeline (do not remove cards from app yet)
- `vesc_mV` in MetricsPacket is hardcoded `3700` — placeholder, never real data
- `can_temperature` reaches M5Stack correctly via CAN ID 9 but is never put into MetricsPacket
- Lisp reads `get-batt` (pack voltage) but never sends it over CAN — no buffer, no can-send-sid
- Plan: add CAN ID 10 for voltage in Lisp, receive in CAN.cpp, feed MetricsPacket vesc_mV
- Plan: add temperature to MetricsPacket (already in LoraStruct, just not populated)
- Plan: use vesc_mV to drive winch battery % bar in app (linear map, floor/ceiling TBD)
- Need to confirm: does get-batt return pack voltage or cell voltage, and what is real empty voltage

### Still open -- current hardware
- [ ] Repeat 120A crane scale test (43-56 kg range too wide, not reliable)
- [ ] Verify actual drum width (estimated 214 mm)
- [ ] Confirm line-out tracking method (VESC get-dist or encoder)
- [ ] Add MOSFET temp telemetry logging during real tows
- [ ] Implement drum diameter compensation in Lisp using line-out estimate -- HIGH PRIORITY:
      without compensation, fixed current significantly overloads pilot at tow start vs mid-tow

### Drum spacer (current gear, immediate improvement) -- IN PROGRESS
- [ ] 3D print cylindrical spacer: 120 mm bore → 185 mm OD
- [ ] Verify full drum still fits within flange (new full = 300 mm, flange OD = 315 mm -- 7.5 mm margin)
- [ ] Re-wind full 1500 m line onto drum with spacer installed
- [ ] Re-confirm full drum diameter measurement after winding (expect ~300 mm)
- [ ] Raise ERPM to 18000 after spacer is installed (do not raise on bare 120 mm drum)

### Live tow logging -- after spacer install
- [ ] Log real tows: ERPM, motor current, battery voltage, MOSFET temp, duty cycle
- [ ] Log speed and height (GPS or barometric)
- [ ] Verify estimated drum diameter matches line-out model
- [ ] Check MOSFET temps under sustained load -- add temp logging if not already active
- [ ] Use logs to validate or correct force model before implementing Lisp compensation

### Gear change for B class towing
- [ ] Laser-cut 50T drum pulley (8M pitch, ≥20mm wide, PC layers)
- [ ] Source replacement belt Optibelt Omega 840-8M-20 (105T)
- [ ] Fit 50T drum pulley, shift motor mount ~12mm, update VESC gear ratio to 2.273
- [ ] Re-calibrate force model with crane scale at 2.27:1 -- mandatory before any tows
- [ ] Raise VESC motor current max to ≥160A; verify MOSFET thermals in static tests first
- [ ] Set ERPM max ~20,000 for 2.27:1 ratio
- [ ] No-load ERPM test after gear change to verify Kv = 277 ERPM/V at new ratio

---

## ERPM Upgrade: 11000 → 18000 (current gear, B class prep)

At current gear ratio 4.091:1, raising ERPM to 18000 gives:

Motor mech RPM max = 18000 / 5 = 3600 RPM
Drum RPM max = 3600 / 4.091 = 880 RPM

| Drum state       | Diam   | Max line speed |
|------------------|--------|----------------|
| Full (all wound) | 265 mm | 12.22 m/s = 44 km/h |
| Empty (all out)  | 120 mm |  5.53 m/s = 20 km/h |

Back-EMF at 18000 ERPM = 18000 / 277 = 65.0 V
Voltage headroom at resting 76.7 V = 11.7 V
Voltage headroom at full charge 84 V = 19 V

Still insufficient for B class at tow start (120mm drum, 20 km/h max) -- see gear change below.
Useful as interim improvement for shorter line starts (≥600m out) or A class gliders.

Tow profile at ERPM 18000, current gear 4.091:1, 80 kg pilot:

| Phase   | Line out | Drum   | Max speed | 60 A  | 80 A  | 100 A     | Max A (80 kg) |
|---------|----------|--------|-----------|-------|-------|-----------|---------------|
| START   | 1500 m   | 120 mm | 20 km/h   | 47 kg | 79 kg | **101 kg**| **81 A**      |
|         | 1400 m   | 135 mm | 22 km/h   | 42 kg | 70 kg | **90 kg** | **90 A**      |
|         | 1300 m   | 148 mm | 25 km/h   | 38 kg | 64 kg | **82 kg** | **98 A**      |
|         | 1200 m   | 160 mm | 27 km/h   | 35 kg | 59 kg | 76 kg     | 105 A         |
|         | 1100 m   | 171 mm | 28 km/h   | 33 kg | 55 kg | 71 kg     | 112 A         |
|         | 1000 m   | 182 mm | 30 km/h   | 31 kg | 52 kg | 67 kg     | 118 A         |
|         | 900 m    | 192 mm | 32 km/h   | 30 kg | 49 kg | 63 kg     | 124 A         |
|         | 800 m    | 201 mm | 33 km/h   | 28 kg | 47 kg | 60 kg     | 130 A         |
|         | 700 m    | 210 mm | 35 km/h   | 27 kg | 45 kg | 58 kg     | 136 A         |
| RELEASE | 600 m    | 219 mm | 36 km/h   | 26 kg | 43 kg | 55 kg     | 141 A         |

Force formula (unchanged from calibration): force_at_D = (0.503 × A − 4.6) × (265 / D_mm)

---

## Gear Change: 90T → 50T Drum Pulley (B Class Towing)

### Why the current gear is insufficient

At 4.091:1 and full charge 84V, physics ceiling at 1300m out (148mm drum):
```
Max ERPM (physics) = 277 × 84 = 23,268
Drum RPM = 23,268 / 5 / 4.091 = 1,138 RPM
Max speed = π × 0.148 × 1,138 / 60 = 8.84 m/s = 31.8 km/h
```
B class trim speed ≈ 40 km/h. 31.8 km/h is the absolute ceiling -- no ERPM setting can overcome this.

### Why flux weakening is not viable

VESC flux weakening currently disabled (field weakening current = 0A).
For 35% speed gain to reach ~43 km/h at 1300m out:
```
id = (19.844e-3 − 19.844e-3/1.35) / 35.3e-6 = 146 A d-axis
```
146A d-axis (heating only, no torque) is not practical. Motor and MOSFET would overheat.

### Tooth count analysis -- why 50T drum pulley

Back-EMF to reach 40 km/h at 120mm drum = 2872 / T_motor × (T_drum/90) volts.
With fixed 22T motor pulley, varying drum pulley:

```
| Drum teeth | Ratio  | Max speed 84V | Max speed 77V | I for 80kg start | 40 km/h? |
|------------|--------|---------------|---------------|------------------|----------|
| 90T (now)  | 4.09:1 | 26 km/h       | 24 km/h       |  81 A            | NO       |
| 60T        | 2.73:1 | 35 km/h       | 32 km/h       | 107 A            | NO       |
| 56T        | 2.55:1 | 37 km/h       | 34 km/h       | 114 A            | NO       |
| 50T        | 2.27:1 | 46 km/h       | 42 km/h       | 139 A            | YES      |
| 44T        | 2.00:1 | 52 km/h       | 48 km/h       | 158 A            | YES      |
```

50T is the recommended choice:
- Achieves 40+ km/h at 120mm drum on both full and resting charge
- 139A for 80 kg at tow start -- within VESC 100/250 capability
- 44T gives unnecessary speed at the cost of ~20A more current

### Practical implementation

**Drum pulley:** 50T, 8M pitch, ≥20mm wide, laser-cut PC layers (same method as current 90T).
Pitch diameter = 50 × 8 / π = 127.3mm (vs current 229.2mm).

**Belt:** smaller drum pulley requires shorter belt.
At current center distance ~262mm, belt length needed ≈ 816mm.
Use **Optibelt Omega 840-8M-20** (105T, 840mm).
Motor mount shifts ~12mm further from drum to tension correctly.

**VESC settings after change:**
- Gear ratio: 2.273 (50/22)
- ERPM max: ~20,000 (gives 40 km/h at 120mm drum, ~74 km/h at full 265mm drum -- but drum
  compensation in Lisp should limit actual speed at larger diameters)
- Motor current max: raise to ≥160A after static crane scale verification

### Estimated force model at 2.27:1

NOT measured. Extrapolated from 4.091:1 calibration assuming same efficiency and proportional friction.

```
Theoretical:       force_kg = 0.301 × A   (at full drum 265mm)
Estimated measured force_kg ≈ 0.280 × A − 2.56
```

**Re-calibrate with crane scale before any tows. Do not use these numbers operationally.**

Safe motor current for 80 kg pilot at tow start (120mm drum, estimated):
```
I ≈ 139 A
```
Raise VESC motor current max to ≥160A for margin. Verify MOSFET thermals in static tests.

---

## Drum Spacer: 120 mm → 185 mm Empty Drum (current gear 4.091:1)

### Why a spacer helps

Without spacer: empty drum = 120 mm, full drum = 265 mm.
Force and speed both vary 2.21× across the tow range.
Max safe current at tow start (120 mm drum, 80 kg pilot) = 81 A -- very low.

Adding a cylindrical spacer to the drum core raises the empty diameter.
If full drum is allowed to grow to 300 mm (flange OD = 315 mm, 7.5 mm margin), the same 1500 m
of line fits in the new annulus by conservation of cross-section area:

```
New annulus area = old annulus area
r_full_new² − r_empty_new² = r_full_old² − r_empty_old²
150² − r_empty_new² = 132.5² − 60²
r_empty_new = sqrt(22500 − 13956) = sqrt(8544) = 92.4 mm
→ D_empty_new = 185 mm
```

### Tow profile with spacer, ERPM 18000, 80 kg pilot

Drum diameter model (area model):
```
drum_radius_mm = sqrt(8543 + 13957 × line_on_drum_m / 1500)
```

| Phase   | Line out | Drum   | Max speed | 60 A  | 80 A  | 100 A | 120 A     | Max A (80 kg) |
|---------|----------|--------|-----------|-------|-------|-------|-----------|---------------|
| START   | 1500 m   | 185 mm | 30.7 km/h | 37 kg | 51 kg | 65 kg | **80 kg** | **120 A**     |
|         | 1400 m   | 195 mm | 32.3 km/h | 35 kg | 48 kg | 62 kg | 76 kg     | 126 A         |
|         | 1300 m   | 204 mm | 33.8 km/h | 33 kg | 46 kg | 59 kg | 72 kg     | 132 A         |
|         | 1200 m   | 213 mm | 35.3 km/h | 32 kg | 44 kg | 57 kg | 69 kg     | 137 A         |
|         | 1100 m   | 222 mm | 36.7 km/h | 31 kg | 43 kg | 55 kg | 67 kg     | 142 A         |
|         | 1000 m   | 230 mm | 38.1 km/h | 29 kg | 41 kg | 53 kg | 64 kg     | 147 A         |
|         | 900 m    | 238 mm | 39.4 km/h | 28 kg | 40 kg | 51 kg | 62 kg     | 152 A         |
|         | 800 m    | 245 mm | 40.7 km/h | 28 kg | 39 kg | 50 kg | 60 kg     | 156 A         |
|         | 700 m    | 253 mm | 42.0 km/h | 27 kg | 37 kg | 48 kg | 58 kg     | 161 A         |
| RELEASE | 600 m    | 260 mm | 43.1 km/h | 26 kg | 36 kg | 47 kg | 57 kg     | 165 A         |

Force formula (unchanged): `force_at_D = (0.503 × A − 4.6) × (265 / D_mm)`

Physics ceiling at tow start (185 mm drum):
- At resting 76.7V: 277 × 76.7 / 5 / 4.091 × π × 0.185 / 60 × 3.6 = **36.2 km/h**
- At full charge 84V: 277 × 84   / 5 / 4.091 × π × 0.185 / 60 × 3.6 = **39.7 km/h**

ERPM 18000 gives 30.7 km/h at tow start -- below the physics ceiling, so ERPM is the binding limit
throughout the entire tow. Raising ERPM further (up to ~23,000 at 84V) would give more start speed.

### Comparison: without vs with spacer

| Metric                           | No spacer (120 mm) | Spacer (185 mm) |
|----------------------------------|--------------------|-----------------|
| Start speed at ERPM 18000        | 20 km/h            | 30.7 km/h       |
| Physics ceiling at start (84V)   | 26 km/h            | 39.7 km/h       |
| Max safe current at start (80 kg)| 81 A               | 120 A           |
| Force variation start→600m rel.  | 1.83×              | 1.40×           |

---

## Speed Ceiling Reference (current gear 4.091:1, all conditions)

```
| Battery   | Physics ceiling ERPM | Max speed full drum | Max speed 120mm drum |
|-----------|----------------------|---------------------|----------------------|
| 84V full  | 23,268               | 57 km/h             | 26 km/h              |
| 76.7V rest| 21,236               | 52 km/h             | 24 km/h              |
```

Setting ERPM above these values has no effect -- motor back-EMF prevents higher speed.

---

## VESC current sign validation + full telemetry pipeline

### Background -- VESC current sign bug (found 2026-05-03)

Flight session logs (`docs/Flight logs/winch_2026-05-03-11-47-17.csv`) showed anomalous
amps readings of ~6500+ A at multiple points during the tow. Math analysis confirmed these
are negative VESC current values wrapped in a uint16 BLE field:

```
raw uint16 65226 → sign-extend → −30 A
raw uint16 65136 → sign-extend → −40 A
```

All anomalous values decode cleanly to small negative integers -- not random packet corruption
(LoRa CRC would have discarded corrupted packets). Timing correlates with deceleration events:
- Aborted first attempt (t≈83–105 s): winch backing off, drum decelerating
- Peak altitude moment (t≈416–417 s): regen −40 A as tow ends and drum stops

A JS clamp fix was applied in app v2.4 and firmware was clamped in remote. However, the root
cause through the full pipeline has not been verified. See `docs/Flight logs/summary flight 1.md`
for the full data and reverse-engineering.

### What still needs verification

- Confirm CAN ID 8 from VESC carries motor current (signed, bidirectional) not battery input current
- Confirm M5Stack `CAN.cpp` passes the float sign through to `MetricsPacket.current_mA`
- Confirm the XIAO remote receives and forwards it without sign loss
- A negative value at the right moment on M5Stack Serial would close the loop

### State configuration (confirmed 2026-05-03)

State 1=20A, 2=30A, 3=40A, 4=60A, 5=80A, 6=100A.
State 6 (100A) was not used in flight 1. State 5 (80A) peaked at 84A in logs (slight VESC
overshoot past configured limit). States 2-4 appear noisy in logs because operator steps
through them quickly during ramp-up — amps in those rows reflect the VESC ramping, not
steady-state at the configured limit. Always document state config in session summary.

### Test plan (bench test, do alongside telemetry pipeline work)

1. Hang a known weight on the line (static load -- no pilot)
2. Run a controlled pull + deliberate brake event (short burst then release/brake)
3. Capture M5Stack Serial output -- log raw CAN ID 8 float value during brake
4. Simultaneously record the phone app log (CSV) -- compare `amps_A` column
5. If M5Stack Serial shows negative current during brake: pipeline confirmed, fix is correct
6. If M5Stack Serial shows positive-only: investigate VESC CAN packet type (motor vs battery current)

---

## Winch telemetry pipeline -- temperature and voltage (not yet implemented)

### Current state (confirmed 2026-05-03)

| Field | Status | Evidence |
|-------|--------|---------|
| `vesc_mV` | Hardcoded 3700 in both MetricsPacket build sites in `winch/src/main.cpp` | All logs show voltage_V = 3.7 |
| `can_temperature` | Received correctly on M5Stack via CAN ID 9 | Seen in `winch/src/CAN.cpp` |
| `can_temperature` → MetricsPacket | Never populated | Not in MetricsPacket build code |
| VESC pack voltage | Lisp reads `(get-batt)` but never sends over CAN | Not in Lisp send loop |

### Implementation plan

**Step 1 -- Temperature (M5Stack only, no Lisp change needed):**
- `winch/src/main.cpp`: populate `m.can_temperature` in MetricsPacket from `can_temperature` variable
- Already received correctly from CAN ID 9, just not packed
- No Lisp change required

**Step 2 -- Battery voltage (Lisp + M5Stack):**
- Lisp: add `(can-send-sid 10 (list (round (* (get-batt) 10))))` to send pack voltage × 10 as CAN ID 10
- First confirm: does `(get-batt)` return pack voltage (e.g. 74.5 V) or cell voltage?
- `winch/src/CAN.cpp`: add CAN ID 10 receive handler, store as `can_voltage_dV` (uint16, dV)
- `winch/src/main.cpp`: populate `m.vesc_mV = can_voltage_dV * 100` (or adjust scale to match field)
- Once real voltage flows through, log columns `voltage_V`, `power_kW`, `energy_Wh` become meaningful

**Step 3 -- App battery bar:**
- Determine real floor/ceiling from a charge/discharge cycle once voltage is live
- Update winch battery % bar linear map in `index.html` (currently placeholder)

### Do all of this in the same bench session as the current sign validation test

Having real voltage and temperature in the logs from the same session will allow:
- Cross-referencing power (kW) against MOSFET temperature rise
- Validating energy (Wh) against battery voltage drop
- Confirming the current sign fix against a known load
