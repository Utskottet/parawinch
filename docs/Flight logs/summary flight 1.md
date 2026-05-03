# Field Session Summary — Flight 1
**Date:** 2026-05-03
**Location:** Farmland site
**Logs:** 4 sessions (3 calibration reel-outs + 1 actual tow flight)

---

## Sessions overview

| Log | Start | Duration | Max line out | Purpose |
|-----|-------|----------|-------------|---------|
| 1 | 10:16 | 109 s | 146 m | Short reel-out, GPS calibration check |
| 2 | 10:27 | 384 s | 571 m | Long reel-out, GPS calibration check |
| 3 | 11:40 | 277 s | 353 m | Reel-out (no GPS anchor set) |
| 4 | 11:47 | 471 s | 356 m | **Actual tow flight** |

---

## Tachometer vs GPS calibration (logs 1 & 2)

GPS anchor set at winch position. Line reeled out by foot.
GPS column = haversine straight-line distance from anchor.

**Log 2 (571 m, best data):**

| Tach (m) | GPS (m) | Delta | Ratio |
|---------|---------|-------|-------|
| 100 | 114 | +14 | 1.14 |
| 200 | 223 | +23 | 1.12 |
| 300 | 339 | +27 | 1.09 |
| 400 | 429 | +29 | 1.07 |
| 500 | 528 | +28 | 1.06 |
| 571 | 595 | +24 | 1.04 |

**Conclusion:** Tach reads approximately **5% low** across the range.
The GPS haversine can only be ≤ actual path walked, so if GPS > tach, the tach is under-counting.
Delta stabilises at ~+27 m from 300 m onwards — systematic, not noise.

**Action:** Within spec for now. Revisit with a longer measured straight-line run to refine the
VESC drum diameter calibration constant. Data from this session will be used as baseline
for a future tach offset correction as a function of line-out.

---

## Flight 4 — First tow flight

**GPS anchor:** Not set (XCT not connected at log start — no GPS distance column).

**State configuration (confirmed):** State 1=20A, 2=30A, 3=40A, 4=60A, 5=80A, 6=100A

### Two attempts recorded:

**Attempt 1 (t ≈ 73–107 s):** Aborted launch.
- 20–30 A pull, line barely moved (stayed at ~348 m)
- Glider likely did not inflate
- Regen braking visible: −19 A to −31 A as winch backed off

**Attempt 2 (t ≈ 360–430 s):** Successful tow.
- Ramp-up: 21–35 A at t=349–368 s (gentle tension phase)
- Full tow: 57–84 A, t ≈ 380–415 s (~35 s of full power)
- Line reeled in: 356 m → 185 m **(171 m reeled)**
- **Max altitude gained: 46 m**
- **Max amps: 84 A**
- **Max climb rate: 4 m/s**
- **Max speed: 9.8 km/h** (XCT ground speed)
- Peak regen braking: −40 A (VESC overspeed limit as line shortened)
- Descent: line released back out 185 m → 356 m as pilot glided down

---

## Signal quality

Measured on LoRa link (remote ↔ M5Stack winch) during reel-out tests.

| Distance | RSSI (dBm) | SNR (dB) | Lost (%) |
|----------|-----------|---------|---------|
| ~100 m | −80 to −95 | 8–10 | 0–5 |
| ~300 m | −103 to −111 | 3–8 | 15–21 |
| ~500 m | −119 to −123 | −6 to −9 | 42–70 |
| ~570 m | −120 to −126 | −10 to −13 | 57–70 |

SNR of −13 dB approaches the SF9 decodability floor (~−12 dB).
**42–70% packet loss at 500+ m is not acceptable for routine operation** at this site.

Notes:
- Farmland environment — possible frequency congestion vs coastal site
- Remote antenna was not in optimal position during reel-out walk
- During the actual flight (operator at winch): 15–30% packet loss at ~350 m — better but still notable
- Winch antenna (on car roof): confirmed good position

**Action:** Monitor at next session. If loss remains high, consider:
- Switching to SF12 (+9.4 dB sensitivity, cost: slower packet rate)
- Dedicated ground-plane antenna for remote
- Check if local LoRa interference on 868.1 MHz

---

## Bugs identified this session

### 1. Amps display wraps on VESC regen current — FIXED in v2.4 / firmware
VESC reports negative amps during regenerative braking.
The BLE packet uses `uint16` for `amps_x10`. Negative values wrap to ~65 000+, displaying
as "6534 A" on screen and corrupting log data.
**Fix:** Firmware clamps to 0 before encoding. JS also clamps on decode.

### 2. `# Max amps` header wrong in logs — FIXED in v2.4
`cv.amps` is a JS string (`.toFixed(1)`). The max-tracking comparison was string-based,
so `"9.0"` beat `"84.0"` alphabetically. Flight 1 header showed `9.0 A` instead of `84 A`.
**Fix:** `parseFloat()` wrapper added to comparison.

### 3. Voltage hardcoded 3.7 V
Power (kW) and energy (Wh) columns are placeholder only — VESC voltage telemetry
pipeline not yet implemented. See `docs/upgrade_plan.md`.

---

## Next session goals

- Set GPS anchor before starting log — needed for GPS distance column in flight
- Test tach calibration on a longer straight run (>600 m) with GPS anchor at winch wheel
- Monitor LoRa signal quality at same site to confirm whether session 1 was anomalous
- First full-height tow to characterise line-out correction curve for future tach offset
