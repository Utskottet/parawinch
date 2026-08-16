# TowMonitor — Pilot-End Tow Research Instrument

**Status:** research architecture / V1 definition  
**Date:** 2026-08-16  
**Purpose:** build the minimum retained airborne sensor package needed to answer the Smart Paragliding Winch research questions with real tow data.

---

## 1. Why TowMonitor exists

The central research question is not whether a more complicated winch can be built. It is:

> **Can different tow-force and reel-length trajectories produce materially more release height from the same available line than a competent conventional electric tow?**

The current logs are already useful, but the two most important physical quantities are still weakly measured:

1. **actual pilot-end tow force** is inferred from motor current/drum geometry rather than measured at the aircraft;
2. **airborne height/wind state** is currently dependent on phone GPS altitude and incomplete wind information.

TowMonitor moves the important research sensing to the pilot end, where the quantity acting on the aircraft can be measured directly.

---

## 2. Core architecture

Conceptual mechanical chain:

```text
HARNESS / TWO-BRIDLE CONVERGENCE
            |
      [ TowMonitor ]
      pilot-end load cell
      barometer + IMU
      MCU + LoRa + battery
      forward airspeed lance
            |
        [ RELEASE ]
            |
          TOW LINE
```

The release is downstream of TowMonitor so the sensor package remains with the pilot after tow release.

The **tow-force load path and airspeed-probe orientation are separate problems**:

- the load cell measures the single resultant tow force after the bridle has converged;
- the pitot/airspeed lance points **forward with the pilot/glider**, not along the tow line;
- tow-line angle may change by many tens of degrees during the tow and must not steer the pitot axis.

This document defines the research instrument only. It does not freeze the mechanical release design.

---

## 3. Sensor priority for the research question

### 1 — Pilot-end load cell — highest priority

Measures the actual force delivered through the tow line to the pilot/wing system.

This is scientifically better than relying only on winch-side motor torque because it naturally includes whatever the long tether has done between winch and aircraft:

- line drag;
- sag/bow;
- elasticity/dynamics;
- difference between winch-end and aircraft-end tension.

Primary output:

`T_pilot [N]`

The existing current/drum-derived force estimate remains useful as a comparison channel, but is no longer the primary research force measurement.

### 2 — Barometer / vario — highest priority

Measures the actual optimisation objective:

`relative height h`

and its fast derivative:

`vertical speed h_dot`

Use a proven high-quality pressure sensor and vario algorithm. Preserve raw pressure as well as filtered altitude/vario output.

### 3 — Airspeed — high priority

A small forward lance measures pilot/glider airspeed through the local airmass.

Its main purpose in this project is **not simply airspeed display**. Combined with phone GPS it provides an estimate of the headwind component during tow.

For an approximately straight tow into wind:

`headwind_component ≈ airspeed − forward_groundspeed`

This lets the analysis distinguish:

- more height caused by stronger headwind;
- more height caused by a different tow-force/reel trajectory.

The pitot must point forward into the relative airflow, not toward the winch.

### 4 — IMU — useful, low marginal cost

The IMU is not needed for absolute altitude. Its useful research roles are:

- fast vertical/transient estimation when fused with barometer;
- pitch/oscillation logging;
- identifying phase/response after tow-force or reel-speed changes;
- later state estimation if dynamic control becomes worthwhile.

Because TowMonitor already contains an MCU and barometer, fitting an IMU is sensible if it does not materially complicate the package.

### 5 — Phone GPS — use existing sensor, do not duplicate in V1

The phone already provides:

- horizontal position;
- groundspeed;
- course/track;
- GPS altitude as a secondary check;
- fix quality/accuracy fields.

Do **not** add a dedicated GPS to TowMonitor V1 unless later evidence says the phone is inadequate.

Log raw phone GPS fields including `accuracy`, `altitudeAccuracy` and source timestamp. Do not round raw altitude before logging.

### 6 — 2-axis fairlead direction — useful but not required to answer the first question

Fairlead direction remains useful for tether modelling and abnormal-state telemetry, but it is not needed to establish whether a different tow strategy gains height.

TowMonitor + phone GPS + barometric height can already estimate aircraft geometric position relative to the winch.

---

## 4. What the winch still needs to log

TowMonitor is intended to avoid turning the winch into a large research-instrument project. The winch should primarily log quantities it already knows in software:

- common timestamp;
- controller state;
- commanded motor current / torque target;
- actual ERPM / reel speed;
- line out;
- line consumed;
- current line-length/radius model state;
- whether an ERPM/current/controller limit is active.

Useful but secondary:

- existing current-derived force estimate, for comparison against `T_pilot`;
- VESC tachometer/revolution count as a cross-check on line length.

Battery power, motor temperature and detailed electrical telemetry are engineering channels, not core aerodynamic research channels unless a run is controller-limited.

---

## 5. Minimum useful synchronized dataset

The research dataset should reduce to:

```text
time
T_pilot              actual pilot-end tow force
h                     barometric relative height
h_dot                 vario / vertical speed
V_air                 airspeed
GPS_lat, GPS_lon      phone GPS position
V_ground, GPS_track   phone groundspeed vector
L                     line out
L_dot                 reel speed
I_cmd / torque_cmd    commanded winch effort
VESC_state            controller state / active limit
```

This gives the core state:

`T, L, L_dot, h, h_dot, V_air, V_ground`

which is enough to answer most of the immediate research questions without adding exotic sensors.

---

## 6. Derived research quantities

### 6.1 Headwind estimate

For an approximately straight tow:

`w_head ≈ V_air − V_ground_forward`

For more general geometry, use the GPS velocity vector and the known/estimated forward-airflow axis. A full 2-D wind solution can be added later if heading/yaw becomes a limiting uncertainty.

### 6.2 Geometric winch-to-pilot angle

Using known winch position, phone GPS horizontal distance `d` and TowMonitor barometric height `h`:

`theta_geo = atan2(h, d)`

This is preferable to assuming line-out directly equals straight-line distance.

### 6.3 Tether excess length / sag indicator

Straight geometric distance:

`r_geo = sqrt(d^2 + h^2)`

Compare with measured line out:

`Delta_L_tether = L - r_geo`

This does not uniquely solve tether shape, but it is a useful consistency/sag/bow indicator and catches impossible combinations in the logging chain.

### 6.4 Height efficiency

Primary project metrics:

`release_height / initial_line`

and during tow:

`dh / d(line_consumed)`

### 6.5 Force-response relationship

TowMonitor lets the analysis directly compare:

`commanded torque/current -> pilot-end tension -> vertical response`

This is important because fast VESC torque response does not automatically imply equally fast pilot-end tow-force response through a long elastic tether.

### 6.6 Wind-normalised tow comparison

With airspeed + GPS, runs can be grouped or corrected by estimated headwind rather than relying only on a ground anemometer reading.

This is central because published sailplane simulations show release height is strongly sensitive to headwind.

---

## 7. Logging architecture

### Common timebase

Every measurement must carry a monotonic timestamp with millisecond resolution.

Do not reduce the source data to one-second rows before storage.

### Suggested source rates

These are starting points, not frozen requirements:

- load cell: `50–100 Hz`;
- IMU: `100+ Hz` raw if available;
- barometer: sensor-native high-rate sampling, filtered vario output retained;
- differential-pressure airspeed: `20–50 Hz`;
- winch/VESC: `20–50 Hz` for ERPM/current/control state;
- phone GPS: native phone update rate.

The analysis can always downsample later.

### Transport vs storage

The preferred user-facing log remains the remote/phone log.

If LoRa bandwidth cannot carry all raw high-rate channels continuously:

1. transmit the research channels needed live at a lower rate;
2. preserve higher-rate raw data locally in TowMonitor only where it adds clear value;
3. keep clocks/sequence numbers aligned so streams can be merged after the flight.

Do not make local high-rate logging a requirement unless bandwidth testing shows it is necessary.

---

## 8. Airspeed lance concept

The sensing point should be placed ahead of the pilot/body disturbance using a small thin forward lance.

Working concept:

- lance approximately `15–30 cm` beyond the TowMonitor package;
- total sensing point ideally well forward of torso/legs;
- small pitot/static or equivalent differential-pressure head at the tip;
- probe axis referenced to pilot/glider forward direction, **not** tow-line direction;
- thin enough to minimise its own disturbance.

A fixed forward probe is the V1 choice. A weathervaning or multi-hole probe is only justified if yaw error becomes a demonstrated limitation.

Calibration should include a simple vehicle/bicycle/known-speed comparison before trusting absolute airspeed.

---

## 9. TowMonitor V1

The smallest version worth building is:

```text
pilot-end load cell
barometer
IMU
differential-pressure airspeed sensor + forward lance
MCU
LoRa
battery
common timestamp / sequence
```

Phone supplies GPS.

Winch supplies line/reel/control state.

This is deliberately enough to answer the research questions **without heavily modifying the winch**.

---

## 10. Calibration / commissioning before research use

### Load cell

- multi-point static calibration through the real load path;
- zero before tow;
- verify hysteresis/repeatability;
- preserve raw ADC counts and calibrated newtons in logs.

### Barometer

- zero relative altitude immediately before tow;
- preserve raw pressure;
- compare against XC Tracer or another known vario during early tests if available.

### Airspeed

- zero differential pressure at rest;
- known-speed calibration;
- check sensitivity to probe yaw/pitch;
- log raw differential pressure as well as converted airspeed.

### GPS / geometry

- store winch GPS location once per session;
- log phone horizontal and altitude accuracy;
- reject/flag impossible geometry when `straight_distance > line_out` by more than known measurement tolerance.

### Time alignment

- verify that a deliberate tow-force change appears in command, load-cell force and airborne response with sensible ordering and timestamps.

---

## 11. What TowMonitor should let us answer

### Conventional pay-in

- How does release height change as pilot-end tow force increases?
- Are current soft/moderate-force tows already close to the useful conventional ceiling?
- Does force scheduling itself leave meaningful height unused?

### Wind

- How many metres of release height are gained per `m/s` of actual headwind in this paraglider/tether system?
- Can apparent tow improvements be explained simply by different wind?

### VESC bandwidth

- How quickly does a commanded motor-torque change become a pilot-end tension change?
- How quickly does the aircraft respond vertically?
- Is there useful control bandwidth beyond what conventional winches exploit?

### Non-conventional line trajectory

If later testing is justified:

- what happens to `h`, `h_dot` and `dh/dline` during near-zero reel speed?
- can controlled payout preserve positive climb or improve final release height?
- does the effect persist after normalising for wind and actual pilot-end tension?

---

## 12. Decision role

TowMonitor is **not the smart controller**.

It is the measurement platform that earns or kills the case for building the smart controller.

The project should not escalate to a complex optimiser merely because TowMonitor exists. Its job is to produce enough trustworthy data to decide whether the remaining release-height opportunity is large enough to justify the larger `~300 h / ~35,000 kr` development programme.
