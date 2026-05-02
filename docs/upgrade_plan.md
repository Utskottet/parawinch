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

Setting ERPM above these values has no effect -- motor back-EMF prevents higher speed...    remote upgrade  how you feel about percent ?
The percentage calculation is wrong for this use case. The floor is 3.40V which is too high — it throws away the bottom chunk of the real battery range. But honestly, LiPo voltage-to-percent is fundamentally noisy regardless:

The relationship is non-linear (flat in the middle, steep at the ends)
It's heavily load-dependent — voltage sags under TX/LoRa bursts
Under charge it's meaningless — voltage is inflated by the charger, not reflecting true SoC
My honest recommendation:

While charging → just show charging + voltage, drop the %
While on battery → smooth the ADC (4-sample rolling average) + use a better curve with 3.0V floor
Below ~3.5V → show a low battery warning regardless of %
The % number as it stands is misleading more than it's helpful. A user seeing "7%" might think they have almost nothing left when they actually have 30 min of runtime left, or vice versa.

Want me to fix the curve and add the averaging, or is the % display mostly for debugging anyway?

even after a full night change before inciden it never showed more than 47 % tracks?
That's a smoking gun. It tells us two bugs are compounding:

The nRF52840's SAADC reference isn't 3.3V — it's actually ~3.6V internally. The code uses 3.3f which makes every voltage read ~8% too low.

Working backwards from your 47% max:

47% on a 3.40–4.20V scale = 3.776V displayed
But actual full-charge = 4.2V
Ratio: 4.2 × (3.3 / 3.67) = 3.776V → actual ADC reference ≈ 3.67V
So you have:

Wrong ADC reference — 3.3f should be ~3.6f → voltage reads low
Wrong floor — 3.40V floor is too high, cuts off the bottom third
Both together = max ~47%, never 100%.

The fix in main.cpp is two lines:


float readRemoteBatteryVoltage() {
  int raw = analogRead(PIN_VBAT);
  return raw * 3.6f / 1023.0f * (1510.0f / 510.0f);  // was 3.3f
}

int voltageToPercent(float v) {
  if (v >= 4.20f) return 100;
  if (v <= 3.00f) return 0;                            // was 3.40f
  return (int)((v - 3.00f) / (4.20f - 3.00f) * 100.0f);
}
Want me to apply it? When you next charge to full you should see close to 100%.

summorize this for a todo
ParaWinch Remote — TODO

Battery fixes (code)

 Fix ADC reference: 3.3f → 3.6f in readRemoteBatteryVoltage() — voltage reads 8% low
 Fix % floor: 3.40V → 3.00V in voltageToPercent() — was capping at ~47%
 Add 4-sample ADC rolling average to reduce % jitter
Deep sleep (code)

 Implement 30-min idle → system-off deep sleep
 Wake on either button press (D0 / D6 GPIO sense)
 Safety rule: only allow sleep when desiredState == 0
 Send state=0 to winch before entering sleep
Hardware

 Monitor current 902830 LiPo charge to full — assess capacity after recovery from deep discharge
 If runtime is <50% of expected after full charge, replace battery
 Consider 803030 or 902530 as drop-in replacement (~600mAh, JST-PH 2.0mm)
[BAT] 3.78V  47%  charging
That's actually full charge — this confirms the diagnosis perfectly.

Working backwards with the ADC correction:
3.78V × (3.67 / 3.3) = **4.20V actual**

The battery is at 4.2V — fully charged. The wrong ADC reference is making it display as 3.78V / 47%.

Battery is salvaged. Ready to apply the two-line fix?
