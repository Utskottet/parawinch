# Winch System Specifications
# Status: verified measured values only -- no estimates, no future plans

---

## Battery

| Parameter         | Value      | Source          |
|-------------------|------------|-----------------|
| Cell configuration | 20S        | VESC config + log confirmed |
| Max voltage        | 84.0 V     | 20S × 4.2 V/cell |
| Nominal voltage    | 74.0 V     | 20S × 3.7 V/cell |
| Log resting voltage | 76.7 V   | 3.835 V/cell (partial charge) |

Note: plan document says 22S -- this is wrong. Use 20S.

---

## Level-Wind Stepper

### Driver

| Parameter               | Value              | Source        |
|--------------------------|--------------------|---------------|
| Model                    | Integrated Stepper Motor Controller | order listing |
| Current range            | 0–2 A              | order listing |
| Input voltage range      | 10–28 VDC          | order listing |
| Compatible motor sizes   | NEMA 8, 11, 14, 17 | order listing |
| Operating voltage (this system) | 24 V        | confirmed     |
| Speed input              | 5 V PWM            | order listing |
| Direction control        | Run / Dir          | order listing |

### Motor

| Parameter         | Value             | Source        |
|--------------------|-------------------|---------------|
| Model              | NEMA 17 Bipolar   | order listing |
| Step angle         | 1.8°              | order listing |
| Holding torque     | 65 Ncm (92 oz·in) | order listing |
| Rated current      | 2.1 A             | order listing |
| Rated voltage      | 3.36 V            | order listing |
| Dimensions         | 42×42×60 mm       | order listing |
| Wiring             | 4-wire bipolar    | order listing |

### Mechanism

Drives a ballscrew that flip-flops travel direction between two end (limit) switches --
the carriage runs to one end, reverses, runs to the other end, reverses again.

Both end switches are wired in parallel onto a single input, `limitSwitchPin` = GPIO 35.
Pin mapping and control logic: see `winch/src/StepperControl.h/.cpp` -- PWM speed pin, run pin,
dir pin, limit switch pin. Reversal fires on a debounced released->pressed edge (25 ms
stability window) with a 400 ms lockout, plus a 3 s stuck-switch watchdog that forces up to
three recovery reversals before latching a jam fault and cutting the run pin.

**Wiring requirement:** GPIO 35 is input-only and has no internal pull-up (true of GPIO 34-39
on all ESP32s), so `INPUT_PULLUP` is a no-op on this pin. The switch line needs an external
4k7 pull-up to 3V3 and a 100nF cap to GND at the ESP32 end -- without it the line floats
whenever both switches are open and picks up noise from the stepper driver.

---

## Motor

| Parameter         | Value          | Source          |
|-------------------|----------------|-----------------|
| Model             | QS138 90H      | plate           |
| Serial            | SIA72V4000WM11021 | plate        |
| Pole pairs        | 5              | user count + VESC detection + RPM measurement test confirmed 2026-04-27 |
| Flux linkage λ    | 19.844 mWb     | VESC motor detection |
| Motor R           | 4 mΩ           | VESC motor detection |
| Motor L           | 35.3 µH        | VESC motor detection |
| Kt                | 0.1719 Nm/A    | derived from λ, pole pairs |
| Kv (line-line peak) | 55.6 rpm/V   | derived from λ, pole pairs |
| Kv (ERPM/V)       | 277 ERPM/V     | derived from λ via Kt (reliable); earlier ~257 value was from low-speed log with RPM controller error -- discard |
| Sensor mode       | Hall           | VESC config |

---

## Drivetrain

| Parameter         | Value      | Source          |
|-------------------|------------|-----------------|
| Drive type        | Timing belt (synchronous) | physical |
| Belt              | Optibelt Omega 1000-8M-20 | physical |
| Belt pitch        | 8M (8 mm)  | physical        |
| Belt length       | 1000 mm (125 teeth) | physical |
| Belt width        | 20 mm      | physical        |
| Belt material     | Rubber, fiberglass cord | datasheet |
| Belt temp range   | −30°C to +100°C | datasheet (ISO 9563) |
| Motor pulley      | 22T        | physical count  |
| Drum pulley       | 90T        | physical count  |
| Gear ratio        | 4.091      | physical count 22T/90T confirmed (22/90 = 4.0909) |

Note: plan document says 4.09 -- that was correct. VESC was configured to 4.180 -- this is wrong, corrected to 4.091.

---

## Drum

| Parameter              | Value   | Source         |
|------------------------|---------|----------------|
| Aluminium flange OD    | 315 mm  | measured (structural reference, not used in force model) |
| Full drum diameter (all 1500m wound) | 265 mm | measured |
| Empty drum diameter (all line out)   | 120 mm | measured |
| Estimated drum width   | ~214 mm | derived from geometry |
| Estimated layers       | 29      | derived         |
| Turns per layer        | ~85     | derived         |

---

## Line

| Parameter        | Value              | Source   |
|------------------|--------------------|----------|
| Type             | Dyneema SK75       | physical |
| Diameter         | 2.5 mm             | measured |
| Total length     | 1500 m             | measured |
| Breaking strength | 740 kg            | spec SK75 2.5mm |

---

## VESC

| Parameter              | Value     | Source / Note                             |
|------------------------|-----------|-------------------------------------------|
| VESC ID                | 56        | from log                                  |
| Wheel diameter (set)   | 265 mm    | corrected to full drum diameter           |
| Gear ratio (set)       | 4.091     | corrected from 4.180                      |
| Battery cells (set)    | 20S       | correct                                   |
| Motor current max      | 80 A      | currently set                             |
| Absolute max current   | 388.7 A   | hardware protection, do not change        |
| Battery current max    | 250 A     | ~19 kW peak battery protection            |
| ERPM max               | 11000     | currently set                             |
| ERPM min               | -11000    | symmetric                                 |
| Current Kp             | 0.0353    | VESC detection                            |
| Current Ki             | 3.97      | VESC detection                            |
| Observer gain          | 2.54      | VESC detection                            |

---

## Force Model

### Theoretical (from Kt, no losses)

```
line_force_kg = (Kt × current_A × gear_ratio) / (drum_radius_m × 9.81)
             = 0.5410 × current_A  (at full drum 265mm, gear ratio 4.091)
```

### Measured calibration (from static crane scale tests, full drum 265mm)

Drivetrain efficiency: 93% with ~4.6 kg constant friction offset.

```
force_kg = 0.503 × current_A − 4.6   (at full drum 265mm)
```

Source: log 2 (20–80A) and log 3 (100–140A), 2026-04-27.

| Current | Theoretical | Measured | Fit    | Data quality       |
|---------|-------------|----------|--------|--------------------|
| 20 A    | 10.8 kg     | 7.0 kg   | 5.5 kg | measured           |
| 40 A    | 21.6 kg     | 15.0 kg  | 15.5 kg | measured          |
| 50 A    | 27.1 kg     | 20.0 kg  | 20.6 kg | measured          |
| 60 A    | 32.5 kg     | 25.0 kg  | 25.6 kg | measured          |
| 70 A    | 37.9 kg     | 30.0 kg  | 30.6 kg | measured          |
| 80 A    | 43.3 kg     | 33.0 kg  | 35.7 kg | measured          |
| 100 A   | 54.1 kg     | 50.0 kg  | 45.7 kg | measured          |
| 120 A   | 64.9 kg     | ---      | 55.8 kg | UNCERTAIN (43-56 kg range, repeat needed) |
| 140 A   | 75.7 kg     | 65.0 kg  | 65.8 kg | measured          |

### Drum compensation (from measured calibration)

```
force_at_D = force_at_265mm × (265 / D_mm)
```

| Current | Full 265mm | 220mm  | 180mm  | 140mm   | Empty 120mm |
|---------|-----------|--------|--------|---------|-------------|
| 40 A    | 15.5 kg   | 18.7 kg | 22.9 kg | 29.4 kg | 34.3 kg    |
| 60 A    | 25.6 kg   | 30.8 kg | 37.7 kg | 48.4 kg | 56.5 kg    |
| 80 A    | 35.7 kg   | 42.9 kg | 52.5 kg | 67.5 kg | 78.7 kg    |
| 100 A   | 45.7 kg   | 55.1 kg | 67.3 kg | 86.5 kg | 101 kg     |
| 120 A   | 55.8 kg   | 67.2 kg | 82.1 kg | 106 kg  | 123 kg     |
| 140 A   | 65.8 kg   | 79.3 kg | 96.9 kg | 125 kg  | **145 kg** |

140A at empty drum = 145 kg. Safety factor vs 740 kg line = 5.1:1.

WARNING: In real tow operation the drum STARTS near-empty and fills during the tow.
The empty drum column is therefore the START of the tow, not the end.
Drum compensation is mandatory -- without it, current must be set based on the starting
(smallest) drum diameter to avoid overloading the pilot at the beginning of the tow.

---

## Drum Diameter vs Line-Out Model

Linear approximation (area model):

```
line_on_drum_m = 1500 - line_out_m
drum_radius_m  = sqrt(r_empty^2 + (r_full^2 - r_empty^2) × (line_on_drum / 1500))
drum_diam_mm   = drum_radius_m × 2 × 1000

where:
  r_empty = 0.060 m
  r_full  = 0.1325 m
```

Key points:

| Line out (m) | Line on drum (m) | Est. drum diam (mm) |
|-------------|-----------------|---------------------|
| 0           | 1500            | 265                 |
| 300         | 1200            | 246                 |
| 600         | 900             | 226                 |
| 900         | 600             | 201                 |
| 1200        | 300             | 169                 |
| 1380        | 120             | 139                 |
| 1500        | 0               | 120                 |

---

## Line Speed (current ERPM max = 11000)

Motor mech RPM max = 11000 / 5 = 2200 RPM
Drum RPM max = 2200 / 4.091 = 538 RPM

| Drum state       | Diam   | Max line speed |
|------------------|--------|----------------|
| Full (all wound) | 265 mm | 7.47 m/s = 27 km/h  |
| Empty (all out)  | 120 mm | 3.39 m/s = 12 km/h  |

27 km/h is below B class trim speed (~40 km/h). See upgrade_plan.md for ERPM and gear change plans.

Physics ceiling (back-EMF = battery voltage, regardless of ERPM setting):
- At resting 76.7V:  277 × 76.7 / 5 / 4.091 × π × 0.265 / 60 × 3.6 = 52 km/h at full drum
- At full charge 84V: 277 × 84   / 5 / 4.091 × π × 0.265 / 60 × 3.6 = 57 km/h at full drum

---

## Tow Operation Profile

### Direction of operation -- READ THIS FIRST

This is a REELING IN system, like a sailplane winch.

```
BEFORE TOW: line is laid out on the field. Drum is nearly EMPTY.
DURING TOW: winch reels IN the line. Drum FILLS.
AT RELEASE: pilot is airborne. Drum is approximately half full.
```

Typical tow range:
- Start: 1300–1500 m line out (drum 120–148 mm, nearly empty)
- Release: 600–750 m line out (drum 207–219 mm, about half full)

The MOST DANGEROUS moment is the START of the tow:
- Drum is at its smallest → highest force per amp
- Line speed is at its lowest (slow ground run)
- Pilot is on the ground and cannot escape

Safe current limits MUST be calculated from the starting drum diameter.
Setting current based on full drum (265 mm) will severely overload the pilot.

### Tow profile table (current ERPM 11000, 80 kg pilot)

Tow progresses top to bottom (line reeling in, drum filling):

| Phase   | Line out | Drum   | Max speed | 60 A  | 80 A  | 100 A     | Max A (80 kg) |
|---------|----------|--------|-----------|-------|-------|-----------|---------------|
| START   | 1500 m   | 120 mm | 12 km/h   | 47 kg | 79 kg | **101 kg**| **81 A**      |
|         | 1400 m   | 135 mm | 14 km/h   | 42 kg | 70 kg | **90 kg** | **90 A**      |
|         | 1300 m   | 148 mm | 15 km/h   | 38 kg | 64 kg | **82 kg** | **98 A**      |
|         | 1200 m   | 160 mm | 16 km/h   | 35 kg | 59 kg | 76 kg     | 105 A         |
|         | 1100 m   | 171 mm | 17 km/h   | 33 kg | 55 kg | 71 kg     | 112 A         |
|         | 1000 m   | 182 mm | 18 km/h   | 31 kg | 52 kg | 67 kg     | 118 A         |
|         | 900 m    | 192 mm | 19 km/h   | 30 kg | 49 kg | 63 kg     | 124 A         |
|         | 800 m    | 201 mm | 20 km/h   | 28 kg | 47 kg | 60 kg     | 130 A         |
|         | 700 m    | 210 mm | 21 km/h   | 27 kg | 45 kg | 58 kg     | 136 A         |
| RELEASE | 600 m    | 219 mm | 22 km/h   | 26 kg | 43 kg | 55 kg     | 141 A         |

Bold = exceeds 80 kg pilot weight limit at that current.

Force formula: force_at_D = (0.503 × A − 4.6) × (265 / D_mm)
Max safe A:    I_max = (target_kg × D_mm / 265 + 4.6) / 0.503

### Safe current limits by start position (80 kg pilot, force ≤ pilot weight)

| Start position | Drum diam | Max safe current |
|----------------|-----------|-----------------|
| 1500 m out     | 120 mm    | 81 A            |
| 1400 m out     | 135 mm    | 90 A            |
| 1300 m out     | 148 mm    | 98 A            |
| 1200 m out     | 160 mm    | 105 A           |
| 1000 m out     | 182 mm    | 118 A           |

As the tow progresses and the drum fills, the same current delivers progressively LESS force.
A tow started at 95 A from 1300 m delivers ~79 kg at the start and only ~55 kg at 600 m release.
This is the correct safe direction -- tension reduces as speed and altitude build.

### Why drum compensation matters

Without compensation, the operator must choose one fixed current for the whole tow.
That current is limited by the start condition (smallest drum = highest force).
With drum compensation in Lisp, current can be increased as the drum fills to maintain
a roughly constant force throughout the tow, allowing stronger pulls at mid-tow
without overloading the pilot at the start.

---

## Safety Analysis

### Line breaking strength vs motor current

Dyneema SK75 2.5mm breaks at 740 kg.

| Safety factor | Max working load | Current needed at full drum |
|---------------|------------------|-----------------------------|
| 3:1           | 247 kg           | 446 A                       |
| 4:1           | 185 kg           | 335 A                       |
| 5:1           | 148 kg           | 268 A                       |

The line will not break from motor force at any realistic operating current.
The risk is pilot overload, not line break.

### Current safety at VESC motor max (80 A)

| Drum state | Line pull | Safety factor vs line break |
|------------|-----------|-----------------------------|
| Full 265mm | 44 kg     | 16.7:1                      |
| Empty 120mm | 98 kg    | 7.6:1                       |

### Pilot load limits

Safe tow force for paraglider launch is typically 50-100% of pilot all-up weight.
This system is set up for 80 kg all-up pilot weight (body + harness + gear).
Max tow force target: 80 kg (= pilot weight).

Safe current at tow start depends on how much line is out -- see Tow Operation Profile above.
Do NOT use the full-drum force numbers to set operating current for real tows.
