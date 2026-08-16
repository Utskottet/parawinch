# Smart Paragliding Winch — Research Foundation v4

**Status:** research foundation, not design freeze  
**Date:** 2026-08-16  
**Purpose:** preserve observations, challenge the aerodynamic hunches with physics and data, and decide whether adaptive towing is worth pursuing. **TowMonitor is now the preferred measurement platform for answering the open questions before committing to complex adaptive control.**

---

## 1. Project question

Can an electric pay-in paragliding winch use measured aircraft/winch state to produce **more release height from a fixed available tow-line length** than a well-operated conventional tow, while remaining inside established tow safety limits?

Working hunch:

> The aerodynamically/geometrically best tow state changes with line angle, wind, tow loading and reel motion. A programmable electric winch may therefore gain more height by varying tension/reel behaviour — and possibly using controlled payout — than by following one conventional tension schedule.

**Status: OPEN.**

The previously discussed **20–30% extra height** is not an assumption. It is a hypothesis to test.

---

## 2. Research discipline

Every important claim should carry one of these tags.

| Tag | Meaning |
|---|---|
| **INPUT** | Project-specific value supplied from our system; verify when possible |
| **OBS** | Direct towing observation; real for those tows, not automatically universal |
| **PUB** | Published/source-supported |
| **DER** | Derived from stated equations/assumptions |
| **SIM** | Model output; only as good as the model |
| **HYP** | Proposed explanation |
| **EST** | Rough numerical estimate |
| **OPEN** | Important unresolved question |

**Rule:** no `EST`, `HYP` or toy-model output may be used to justify flight-control complexity.

For every new discovery:

1. state the observation/claim;
2. write the minimum governing physics;
3. identify assumptions that could reverse the conclusion;
4. identify the cheapest source/simulation/measurement that could falsify it;
5. update the hunch register.

---

## 3. Current engineering take

### Strong enough to use as foundations

1. **Tow geometry causes diminishing pay-in climb authority.** As line angle rises, the same tension gives less horizontal pull and more downward load. `DER`
2. **Headwind can greatly improve height gained per metre of line consumed.** Much of this happens automatically under ordinary force-controlled towing because ground closure falls. `DER/SIM`
3. **Pay-in and payout have different geometry histories.** Payout may therefore access useful trajectories unavailable to pure pay-in. `DER`, magnitude `OPEN`
4. **Electric control gives high bandwidth, repeatability, logging and programmable dynamic response.** `PUB/engineering fact`
5. **Pilot-end tow force + airborne barometric height are the two most important missing direct measurements.** `engineering decision`
6. **TowMonitor is the preferred research platform:** retained pilot-end load cell + barometer/vario + airspeed + IMU + LoRa, with phone GPS and existing winch telemetry completing the dataset. `engineering decision`
7. **Long-line elasticity/dynamics matter and must be identified before deliberately exciting the tow system.** `PUB + EST`

### Still unresolved

- Does a good conventional tension schedule leave meaningful height on the table?
- Does the visually extreme pilot/canopy geometry correspond to aerodynamic loss?
- Does effective paraglider `L/D` improve, stay similar, or degrade as tow load rises?
- Is controlled payout valuable in realistic wind and within acceptable tow time?
- Can ground line direction predict useful aircraft geometry despite sag/drag?
- How much extra sensing is justified?
- Is the gain large enough to justify adaptive control at all?

### Working verdict

**Build TowMonitor, instrument and model first. Do not build the complex optimiser yet.**

A null result is useful: if a well-tuned electric constant-tension tow already captures nearly all available height, the project should stop at simpler electric-control benefits.

---

## 4. Observations/hunches that must remain

### O1 — Canopy appears far behind pilot on pay-in — OBS

During pay-in the pilot can be visibly far forward of the canopy, particularly at shallow line angle.

Do **not** call this “high AoA” from appearance alone:

- canopy attitude;
- wing/pilot relative position;
- air-relative flight-path angle;
- aerodynamic angle of attack

are different quantities.

**Question:** is the geometry simply the equilibrium produced by the tow-force vector, or does tow loading/trim/flexible deformation create significant extra drag?

---

### O2 — Payout feels fundamentally different — OBS

Observed qualitative sequence:

`shorter line → tension/climb established → pilot reaches tow geometry → line pays OUT while tow continues`

Pay-in:

`long shallow line → reel IN continuously → line length and force-vector geometry continually change`

**Question:** does payout preserve a more height-efficient aerodynamic state, or is the apparent difference mainly kinematic?

---

### O3 — Height gain appears front-loaded — OBS

A large fraction of useful height appears to arrive while comparatively little of the original line has been reeled in, especially with headwind.

Important correction:

> This observation does **not** by itself prove wasted aerodynamic energy or poor conventional control.

Section 6 shows that headwind + tow geometry alone can create strong front-loading.

---

### O4 — Very low groundspeed can coexist with climb — OBS

In roughly 4–5 m/s headwind, the pilot has been observed gaining altitude while making very little apparent forward progress over the ground.

This is physically plausible: tow-induced air-relative climb can remain positive while headwind cancels much of the horizontal ground-speed component.

---

### O5 — Fast electric response may exploit gusts — HYP

VESC torque/reel control can react rapidly and reproducibly.

Possible gains:

- smoother tension transients;
- active damping of the tether/winch mode;
- bounded tension modulation in changing wind;
- perhaps improved `height / line consumed`.

The **dynamic-control benefit exists as an engineering avenue even if the height-optimisation hypothesis fails**, but its actual advantage over a good hydraulic system must still be measured.

---

## 5. Minimum governing physics

Use the simplest model that can falsify the hunch.

Definitions:

- `W` = total airborne weight in the point-mass model
- `T` = aircraft-end tow tension
- `θ` = geometric tow-line angle above horizontal
- `E = L/D` = effective glide ratio at the current state
- `r` = straight winch-to-aircraft distance in the massless-line model
- `V` = airspeed
- `w` = horizontal headwind component
- `α` = air-relative climb-path angle

### 5.1 Tow-force vector — DER

`T_horizontal = T cosθ`

`T_down = T sinθ`

Resultant non-aerodynamic force:

`R = sqrt[(T cosθ)^2 + (W + T sinθ)^2]`

Effective-gravity tilt:

`β = atan2(T cosθ, W + T sinθ)`

This “tilted gravity” picture is the basic steady tow model.

---

### 5.2 Simplified aerodynamic equilibrium — DER, conditional

If the glider can temporarily be represented by a fixed `L/D = E`:

`γ = atan(1/E)`

`α = β − γ`

If we additionally assume classical polar scaling:

`n = R/W`

`V ≈ V0 sqrt(n)`

This speed scaling is useful as a first model, but **constant `L/D` is not yet justified for a flexible paraglider under tow**.

---

### 5.3 Wind + geometry kinematics — DER

`h = r sinθ`

For horizontal headwind:

`r_dot = −V cos(α + θ) + w cosθ`

`r θ_dot = V sin(α + θ) − w sinθ`

This explains a central phenomenon:

- headwind reduces ground closure;
- `|r_dot|` can become small;
- the aircraft can continue climbing/rotating to higher `θ`;
- large height can therefore be gained while consuming little line.

Much of this happens **without smart control**.

The smart controller must beat that natural constant-force response.

---

### 5.4 Diminishing pay-in authority with line angle — DER

As `θ` rises:

- `T cosθ` falls;
- `T sinθ` rises;
- `β` falls toward the glide angle;
- the simplified air-relative climb angle `α` falls.

This creates diminishing late-tow climb even before modelling:

- tether drag/sag;
- canopy deformation;
- pilot/wing dynamics;
- controller limitations.

---

### 5.5 Maximum-height event is not asymptotic — DER

In this toy ODE, maximum geometric height occurs when:

`h_dot = V sinα = 0`

so the event is `α = 0`.

This is a **finite-time zero crossing**, not inherently an asymptote.

At `α = 0`:

`θ_dot = (V − w) sinθ / r`

for a parallel horizontal headwind.

If `V > w` and `θ > 0`, `θ_dot` remains positive, so the model continues through `α = 0`; after that `α < 0` and height decreases.

Therefore any reported “time to maximum height” is the numerical event time at `α = 0`, not time to an asymptotic equilibrium.

---

## 6. Sanity model — can the “first 20%” effect appear without adaptive control?

### 6.1 Purpose

Only answer:

> Can ordinary tow geometry + headwind create strong front-loading?

It is **not** a release-height predictor.

### 6.2 Baseline schedule

The previous version used `T/W = 0.8` from ground level. That was a poor baseline because established tow practice uses reduced tension during the early launch phase.

This revision uses an **illustrative BHPA-style staged schedule**:

- `T/W = 0.4` until approximately `100 ft / 30.5 m AGL`;
- `T/W = 0.8` afterwards.

BHPA Amendment 24 states 50% of target initially until roughly 100 ft, then 100% target for optimum ascent. This is a research baseline, **not a replacement for local approved procedure**.

Other assumptions:

- initial `r = 1000 m`
- `L/D = 8`
- `V0 = 8.89 m/s = 32 km/h`
- `V = V0 sqrt(n)`
- straight massless dragless tether
- uniform horizontal wind
- quasi-steady point-mass wing
- event = `α = 0`

`V0 = 32 km/h` is a **project INPUT**, not a statement about typical paragliders. Verify it against the exact wing model/polar before calibrated simulation.

### 6.3 Output

| Headwind | Line reeled at max-h event | Toy max height | Height after first 200 m reeled | Fraction of toy max already gained | Time to `α=0` event |
|---:|---:|---:|---:|---:|---:|
| 0 m/s | 673 m | 314 m | 81 m | 26% | 91 s |
| 3 m/s | 574 m | 409 m | 143 m | 35% | 123 s |
| 5 m/s | 470 m | 510 m | 244 m | 48% | 160 s |
| 7 m/s | 297 m | **675 m** | 576 m | **85%** | 228 s |

### 6.4 Critical warning — the absolute heights are not credible predictions

The `7 m/s → 675 m` result is deliberately left visible because it exposes how optimistic the toy model becomes.

`675 m / 1000 m = 67.5%` height-to-initial-line.

The previous vague **~42% real-world height/line** note can now be replaced by actual project logs. Two relatively clean 2026-08-14 tows reached **210 m from 537 m initial line (39.1%)** and **239 m from 649 m (36.8%)**. A third recorded **201 m from 676 m (29.7%)**, but that file contains obvious derived-data anomalies and should be treated more cautiously. See §6.7. `OBS/INPUT`

Either way, **do not quote 675 m as expected performance.**

The toy model omits large effects that can change the result:

- tether mass;
- tether aerodynamic drag;
- sag/bow;
- aircraft-end tension differing from winch tension;
- real paraglider polar under tow;
- flexible-canopy deformation;
- actual wind profile/gradient/turbulence;
- pilot control/trim;
- launch transient;
- reel/controller dynamics.

Some omitted effects can help and others hurt; the model must be calibrated rather than “corrected” by guessing losses.

### 6.5 What survives the model failure

The absolute output is optimistic, but one qualitative result remains worth testing:

> Sufficient headwind can create a strongly front-loaded `height vs line-consumed` curve even with a conventional staged tension schedule.

At the more directly relevant `5 m/s` toy case, the first 200 m of reel-in accounts for about **48%**, not “almost all”, of toy maximum height.

Therefore the very strong first-20%-of-line observation remains interesting and should be tested against real logs.

### 6.6 Published sailplane benchmark — highly relevant `PUB`

The RWTH Aachen sailplane work is the strongest quantitative benchmark found so far because it models the full winch-launch system rather than only point-mass geometry. It should be treated as an **analogy and calibration reference**, not as a direct paraglider prediction.

#### Santel 2009 — numerical ASK 21 winch launch

Christoph Santel's 2009 six-degree-of-freedom simulation couples:

- ASK 21 aircraft aerodynamics and dynamics;
- a pilot controller;
- a winch/operator model;
- a dynamic FEM tow cable including elasticity, gravity and aerodynamic drag;
- atmosphere and a 3-D wind field.

Reference configuration:

- available tow distance: `1000 m`;
- zero wind;
- synthetic cable;
- simulated release height: **439 m**;
- release time: **33 s**.

Wind sensitivity in that model:

- `+2.5 m/s` headwind → **+28 m** release height;
- `−2.5 m/s` tailwind → **−26 m** release height.

The local headwind result corresponds to roughly **+11 m release height per +1 m/s headwind** for that specific configuration. This is **not a universal coefficient**.

The same study estimated:

- cable gravity effect on release height: about **−2 m**;
- cable aerodynamic drag effect: about **−32 m**;
- an aggressive pilot technique: about **+22 m**, but with the simulated aircraft entering stall during the dangerous early phase.

This is important because it gives an empirical-order **loss/opportunity budget**: cable drag was a several-percent effect, while an unsafe aggressive flight technique bought only about 5% extra height in that case.

#### Gäb & Santel 2011 — OSTIV / Technical Soaring

The later published model is more directly useful as a conventional-tow benchmark. Its reference case uses an ASK-21-like two-seater with `1000 m` synthetic cable and zero wind.

The commanded winch force:

- starts at `2500 N`, roughly **0.5 aircraft weight**;
- rises within about 5 s after liftoff to `7500 N`, approximately **1.5 aircraft weight**;
- is reduced again as the cable angle approaches `65°`.

Result:

- **431 m release altitude after 35 s** from `1000 m` initial cable.

The paper's wind sweep reports a release-height gradient of roughly:

> **5 m altitude per 1 km/h of headwind**

which is approximately:

> **18 m release height per +1 m/s headwind**

for that model and operating point.

This is one of the most useful numerical results in the literature for this project. The 2009 `+28 m at 2.5 m/s` result and the 2011 `~18 m/(m/s)` gradient differ because the model/configuration evolved; carry both forward as **model-dependent sensitivity bounds**, not as a physical constant.

#### Important actuator qualification

The RWTH winch is **not dynamically equivalent to the present VESC winch**. It represents a diesel winch; rotating drivetrain inertia is lumped mathematically into an equivalent flywheel, and a human-like controller regulates cable force through engine throttle. The word *flywheel* therefore does not mean a flywheel-launch concept; it is an equivalent inertia model.

The authors explicitly note that contemporary electric winches already offered built-in force control, which is why controlled cable force was chosen as the reference operating philosophy.

**Implication:** a current-controlled BLDC/VESC gives substantially more actuator bandwidth and reversible torque authority than the simulated diesel/throttle chain, but the long elastic tether and aircraft dynamics still limit how much of that bandwidth reaches the aircraft-end force. The RWTH work therefore benchmarks **good conventional force-controlled pay-in**, not the full dynamic control space available to this project.

#### Richard Eppler — _Windenschlepp: Sicherheit und optimale Ausklinkhöhe_

Eppler asks a narrower but extremely relevant question: **for a conventional steady sailplane winch launch, how does available cable force affect the attainable climb/release height?**

In §6, _Die optimale Ausklinkhöhe_, his quasi-steady force-equilibrium argument says:

- more available cable force permits a steeper climb;
- theoretical maximum release height is obtained when the winch supplies the **largest permissible cable force through the climb**, subject to the weak-link/safety constraint;
- the benefit has **strong diminishing returns**: increasing `S/G` from `0.4 → 0.8` changes the climb substantially more than `1.2 → 1.6`.

This is strong evidence against a large hidden gain from merely finding a clever **ordinary pay-in tension schedule**.

However, Eppler assumes essentially steady climb at constant airspeed and examines constant `S/G` curves. He does **not** optimise:

- high-bandwidth `T(t)`;
- reel-speed trajectories;
- deliberate line hold;
- payout/re-reel cycles;
- gust harvesting;
- tether-energy manipulation;
- MPC/trajectory optimisation.

Therefore Eppler constrains the **smart-tension** hypothesis but does not answer the more radical **line-length-trajectory / payout** hypothesis.

### 6.7 Project logs — first quantitative comparison `OBS/INPUT`

The three latest public v2.11 logs from 2026-08-14 contain airborne height and line-length data:

| Tow | Max line out | Max logged height | Height / initial line | Data note |
|---|---:|---:|---:|---|
| 15:58 | 537 m | 210 m | **39.1%** | relatively clean |
| 17:07 | 649 m | 239 m | **36.8%** | relatively clean |
| 17:30 | 676 m | 201 m | **29.7%** | derived-data anomalies; use cautiously |

Project inputs for these tows:

- pilot mass: **82 kg** `INPUT`;
- estimated headwind: roughly **2–3 m/s** `INPUT/OBS`;
- full pilot-weight command state (`State 6`) was **not reached** `INPUT`;
- logged `force_kg` is derived from the winch model and is **not a calibrated aircraft-end tension measurement**.

The first two tows therefore already achieve roughly **37–39% height / initial line** at less than the project's full pilot-weight command, while the 2011 ASK 21 benchmark produces about **43%** with a main-climb force around `1.5 × aircraft weight`.

This comparison must **not** be read as aircraft equivalence: sailplane and paraglider aerodynamics, line lengths, line drag, pilot technique, wind and release criteria differ. Its value is narrower:

> The present paraglider system is already operating in the same broad geometric `height/line` region as a serious conventional sailplane simulation despite substantially softer nominal tow loading.

That weakens the hypothesis that a very large improvement is waiting simply by pulling harder or by refining an ordinary pay-in tension schedule. More project logs at higher approved tow-force states are needed to map the remaining conventional-force benefit.

The `17:30` file reports an impossible `105.49 m/s` maximum climb rate and begins with an altitude offset near `−10 m`; retain the raw tow section but do not use its generated headline statistics blindly.

### 6.8 Revised decision implication

The literature + project logs now suggest splitting the original optimisation hypothesis into two branches:

**A — smarter conventional pay-in tension control**

Eppler and the RWTH reference cases make a **20–30% gain from tension scheduling alone look increasingly unlikely**, although this is not yet proven for the paraglider. The next project logs should establish how much height is added as tow force approaches the normal approved target.

**B — non-conventional line-length trajectory**

The larger unresolved opportunity is now:

> Can high-bandwidth torque/reel control use near-zero reel speed, controlled payout, or bounded reel-in/reel-out trajectories in headwind to reach states unavailable to ordinary pay-in?

Neither Eppler nor the RWTH conventional-launch studies answer this. This branch is therefore the main remaining candidate for a genuinely large release-height gain.

---

## 7. Important aerodynamic unknown — `L/D` versus tow loading

Classical first-order aircraft theory says that, with geometry and aerodynamic coefficients unchanged, increasing wing loading mainly shifts the polar to higher speed; best `L/D` need not change.

But a paraglider is flexible and Reynolds number also changes.

Since:

`V ∝ sqrt(n)`

then approximately:

`Re ∝ sqrt(n)`

for unchanged chord/density/viscosity.

The sign of:

`d(L/D) / dn`

is therefore **OPEN**.

### Case A — approximately invariant `L/D`

Tow loading mostly increases required airspeed while preserving efficiency.

**Implication:** harder safe pull may be favourable; “soft is efficient” may be wrong.

### Case B — `L/D` degrades with load/speed

Possible causes include flexible-canopy deformation, inlet/leading-edge effects, suspension/pilot drag fraction and trim changes.

**Implication:** an interior optimum may exist; softer pull could improve height per line.

### Case C — `L/D` improves over part of the load range

Higher speed raises Reynolds number. Depending on the real airfoil/flexible-wing regime, profile drag coefficient **may** improve enough to raise effective `L/D`.

**Important correction:** increasing Reynolds number does **not automatically** mean better `L/D`; the sign must come from the actual wing/polar.

**Implication:** optimum could shift toward higher tension even more strongly than Case A.

### Why this still matters — but is no longer the sole build-decision pivot

`L/D(n, tow state)` remains important for predicting the exact optimum tension and for calibrated simulation. However, the sailplane evidence in §6.6 and the project logs in §6.7 reduce its importance to the **300 h / 35,000 kr build decision**.

The project can now ask a more structural question first: does ordinary high-safe-force pay-in already approach the practical conventional ceiling, and if so can changing the **line-length trajectory itself** create a substantially better class of tow?

Resolution path:

1. exact wing data/published polar;
2. Heatwole/glidersim or equivalent model;
3. instrumented tow logs;
4. airspeed later if state identification remains ambiguous.

---

## 8. Payout — promising but OPEN

Potential mechanism:

- late pay-in has poor line-consumption geometry;
- headwind may permit continued climb/geometry change with near-zero reel speed;
- controlled payout might preserve useful line geometry or deliberately trade time for line.

But:

`dh/dr = sinθ`

is only a geometry identity **if θ can actually be maintained**.

Whether it can be maintained depends on:

`wind + V + T + polar + tether + pilot/wing dynamics`

The airborne-wind-energy result often paraphrased as:

`v_out ≈ wind / 3`

is **not** a paraglider height-maximisation control law.

Correct status:

> Controlled payout is physically plausible and potentially high-value, but the feasible/optimal law must be derived for the paraglider objective.

Simulator first; flight experiment later.

---

## 9. Tether direction, sag and lockout

### 9.1 Geometry limitation

A fairlead sensor measures the **local tether tangent at the winch**, not guaranteed pilot bearing.

Long tether introduces:

- gravity;
- aerodynamic drag;
- lateral bow;
- tension variation;
- elasticity.

Therefore:

`fairlead angle ≠ guaranteed aircraft geometric angle`

and it cannot replace barometric height without a validated tether model.

### 9.2 Safety value

The same sensor still belongs **early** in the programme.

BHPA describes paraglider tow rotation/lock-out as a divergent event: the canopy turns away from the tow direction, tow tension increases and accelerates the turn. It states the angle between tow line and canopy heading must not exceed 45° and prescribes reducing/removing tension when divergence develops.

A two-axis fairlead direction sensor can provide:

- lateral tether excursion;
- rate of lateral excursion;
- local line-direction history;
- an additional machine-readable abnormal-state cue.

But it **does not measure canopy heading**, so it is not a complete lockout detector and must not replace operator observation or other approved safety procedures.

**Decision:** install/log 2-axis fairlead direction **before deliberate control perturbation testing**, primarily as safety/research telemetry, not as a height sensor.

---

## 10. Tether elasticity and the possible ~6 s mode

Long tow line is part of the dynamic plant.

BHPA explicitly notes that tow-line elasticity is proportional to length and that the operator must account for elasticity/length when reacting to canopy fluctuations and tension readings.

First-order line stiffness:

`k_line ≈ ΔT / ΔL`

Current project estimate:

- line length ≈ `1000 m`
- working elongation assumption ≈ `0.8%` at relevant load `EST`
- `ΔL ≈ 8 m`
- `ΔT ≈ 800 N`

gives:

`k_line ≈ 100 N/m` `EST`

A crude undamped longitudinal mode:

`T_n ≈ 2π sqrt(m_eff / k_line)`

With `m_eff ≈ 100 kg`:

`T_n ≈ 6.3 s` `EST`

### Do not over-trust this number

`m_eff` is not simply pilot mass. It can contain:

- projected pilot/wing inertia;
- aerodynamic response;
- line distributed mass;
- drum/motor reflected inertia;
- drum-radius change with fill.

The `0.8%` elongation assumption is also not verified for the actual tow line.

Generic heat-set Dyneema SK78 products can have working stretch below ~1%, so the order of magnitude is plausible, but **the actual line must be measured**.

### Why it matters

If a real mode lies in the several-second range:

- gusts can excite it;
- badly chosen reel/tension perturbations can excite it;
- E3 perturbation frequency becomes a safety/stability variable.

Before E3:

1. measure static load-extension of the actual line;
2. identify drum/motor reflected inertia;
3. use small safe system-identification disturbances to measure the real damped mode.

Do not schedule experimental oscillations near an unmeasured resonance.

---

## 11. Sensor strategy — TowMonitor-centred

The sensor question is now narrower: **what minimum new sensing is needed to determine whether the large smart-tow project is worth building?**

The preferred answer is a retained pilot-end package called **TowMonitor** rather than progressively instrumenting the winch with many research-only sensors.

### R0 — existing winch calibration/logging — mandatory

Keep and improve quantities the winch already knows:

- drum radius vs fill;
- line out / line consumed;
- line speed / ERPM;
- exact commanded current/torque;
- controller state and active limits;
- timestamps;
- existing current/drum-derived force estimate for comparison.

A VESC tachometer/revolution count is useful as a software-only cross-check on line-length reconstruction.

Purpose: establish the ground-side control input and line trajectory without turning the winch into the main research sensor platform.

### R1 — TowMonitor pilot-end load cell — highest new-sensor priority

TowMonitor measures actual pilot-end tow force after the long tether has introduced whatever drag, sag, elasticity and dynamics occur between winch and aircraft.

Output:

`T_pilot`

This becomes the primary research tension channel. Winch current-derived force remains a comparison/engineering channel.

### R2 — TowMonitor barometer / vario — highest new-sensor priority

Output:

`relative height + filtered vertical speed + raw pressure + status + sequence/time`

This directly measures the optimisation objective and replaces phone GPS altitude as the primary vertical measurement.

### R3 — TowMonitor airspeed — high priority

A small forward-pointing pitot/differential-pressure lance measures pilot/glider airspeed.

Its primary project use is to estimate headwind when combined with phone GPS groundspeed, allowing tow-to-tow performance to be separated from wind variation.

The probe points **forward with the pilot/glider**, not toward the winch. Tow-line angle can change by many tens of degrees during the tow and must not define the pitot axis.

### R3b — TowMonitor IMU — useful, low marginal cost

Potential uses:

- transient vertical estimator when fused with barometer;
- pitch/oscillation logging;
- response timing after tension/reel changes;
- later state estimation.

The IMU is not required for absolute height, but is sensible inside TowMonitor if it adds little complexity.

### R4 — phone GPS — use existing sensor

The phone remains the primary GPS source:

- horizontal pilot position;
- groundspeed;
- track/course;
- GPS altitude as a secondary cross-check;
- `accuracy`, `altitudeAccuracy` and GPS timestamp.

Do not add a dedicated GPS to TowMonitor V1 unless phone GPS proves inadequate.

### R5 — 2-axis fairlead direction — useful but lower priority for the main question

Still useful for:

- local tether-direction history;
- tether-model comparison;
- supplemental abnormal-state telemetry.

But TowMonitor + phone GPS + barometric height can answer the initial height/force/wind research questions without it. It remains supplemental rather than a primary height sensor.

### Minimum research state

With TowMonitor + phone + existing winch telemetry, the important measured state becomes:

`T_pilot + h + h_dot + V_air + GPS + L + L_dot + torque/current command`

This is enough to attack the principal build/no-build questions before adding exotic sensing.

---

## 12. TowMonitor — retained pilot-end research node

Separate project document: **TowMonitor — Pilot-End Tow Research Instrument**.

Working mechanical concept:

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

The release is downstream of TowMonitor so the sensor package remains with the pilot after release.

The critical mechanical distinction is:

- the **load cell** measures the single resultant tow force in the tow load path;
- the **airspeed lance** is referenced to the pilot/glider forward direction, not the tow-line direction.

TowMonitor provides:

- actual pilot-end tension;
- barometric height and vertical speed;
- airspeed;
- IMU transients/oscillation;
- common airborne timestamps;
- LoRa telemetry.

Phone supplies GPS. The winch supplies line length/speed and control commands.

The resulting core dataset is:

`T, L, L_dot, h, h_dot, V_air, V_ground`

Derived quantities include:

- headwind estimate from airspeed + GPS groundspeed;
- geometric winch-to-pilot angle from GPS horizontal distance + barometric height;
- line-out vs straight-distance consistency/sag indicator;
- `release height / initial line`;
- `dh / d(line consumed)`;
- delay/response from winch command -> pilot-end tension -> aircraft vertical response.

**Decision:** TowMonitor is a research instrument, not the optimiser. Its purpose is to determine whether the optimiser is worth the larger project investment.

---

## 13. Control architecture

### Layer A — independent safety / approved conventional control

Local at winch:

- hard tension ceiling;
- torque/current limits;
- reel-speed limits;
- emergency states;
- operator abort;
- telemetry-loss behaviour.

Airborne telemetry must never be required to keep a conventional tow safe.

### Layer B — fast deterministic winch control

Inputs:

`pilot-end tension when available + reel speed + line length + motor state + fairlead direction if fitted`

Controls torque/reel response inside Layer A limits.

### Layer C — research/optimisation

Slower inputs:

`TowMonitor height + vertical speed + pilot-end tension + airspeed/wind estimate + reel history + GPS geometry`

Produces only **bounded** target changes for Layer B.

Layer C never bypasses Layer A.

---

## 14. Independent fallback branch — active impedance / gust damping

Even if adaptive height optimisation produces little gain, an electric winch can be investigated as a controllable mechanical impedance.

Generic form:

`T_cmd = T_base + K·ΔL + C·L_dot`

within hard safety limits.

Interpretation:

- `T_base` = normal tow-force demand;
- `K` = synthetic stiffness;
- `C` = synthetic damping.

This is fundamentally different from pure current control, which behaves approximately as a torque/force source.

Impedance control is established control theory for shaping force-motion interaction. Its value **for this specific tow system is not yet proven**, and a good hydraulic winch may already provide useful compliance/damping.

Research question:

> Can measured/controlled impedance reduce tension overshoot and tow oscillation without sacrificing useful climb?

This branch requires only ground-side sensing and can remain valuable if the height-optimisation branch is abandoned.

---

## 15. ML is not the first controller

Start with:

1. physics;
2. calibrated simulation;
3. system identification;
4. trajectory optimisation / MPC if needed.

ML may later help with estimators such as:

- wind-state inference;
- tether-sag inference;
- abnormal-state classification.

Do not make a model-free learned controller responsible for the safety-critical tension loop.

---

## 16. Simulator roadmap

### M0 — point-mass/tow geometry model

States:

`x,h` or `r,θ`

Include:

- staged conventional tension schedule;
- parameterised polar;
- uniform and height-dependent wind;
- reel-in/out command;
- finite line;
- time constraint.

Purpose:

- reproduce qualitative physics;
- expose impossible/optimistic assumptions;
- estimate sensitivity.

**Calibration requirement:** M0 is not trusted for optimisation until it can reproduce a real conventional tow within useful error.

### M1 — flexible tether

Add:

- distributed mass;
- aerodynamic drag;
- elasticity;
- sag/bow;
- fairlead geometry;
- winch-end vs aircraft-end tension.

Purpose:

- explain the large optimism in Section 6;
- determine what fairlead direction actually tells us;
- identify line dynamic modes.

### M2 — paraglider dynamics

Use/adapt existing validated paraglider modelling rather than inventing a canopy model.

Add:

- pilot/wing relative dynamics;
- pitch;
- tow attachment;
- real/identified polar;
- aerodynamic deformation effects as available.

Purpose:

- investigate O1;
- identify `L/D(n,tow state)`;
- evaluate transient reel/tension changes.

### M3 — bounded control optimisation

Only after M0–M2 match real logs:

- tension trajectory;
- reel-speed trajectory;
- gust response;
- possible payout;
- complexity-vs-height trade.

---

## 17. Experimental programme

### E0 — calibration

Measure / validate:

- TowMonitor pilot-end load-cell calibration;
- TowMonitor barometer/vario zero and repeatability;
- TowMonitor airspeed zero and known-speed calibration;
- phone GPS accuracy fields and timestamping;
- drum radius/fill relation;
- line length;
- line speed;
- common timestamp alignment across TowMonitor / phone / winch;
- line static extension;
- reflected winch inertia if practical.

### E1 — competent conventional baseline

Log repeatable normal tows with TowMonitor:

`barometric height + vertical speed + pilot-end tension + airspeed + phone GPS + line speed + line length + commanded current/torque + VESC state/limits + ground wind where available`

Use airspeed + GPS to estimate the actual headwind component during the tow, rather than treating a ground wind reading as the complete airborne wind state.

Baseline must use competent staged tow practice, not “80 kg from zero altitude”.

BHPA guidance explicitly uses reduced early tension followed by full target tension after about 100 ft.

### E2 — natural variability

Estimate tow-to-tow variance before claiming small gains.

Needed before interpreting:

`5% vs 10% vs 20%`

effects.

### E3 — bounded system identification / perturbation

Only after:

- TowMonitor force/height channels are validated;
- line-length reconstruction is validated;
- line dynamic mode identified;
- baseline stable;
- safety review completed.

Apply small bounded changes in allowed tension/reel behaviour.

Measure:

- commanded torque/current -> **pilot-end tension** response;
- airspeed response;
- oscillation / IMU response;
- `dh/dline`;
- vertical response;
- settling time;
- dependence on estimated headwind.

Do **not** select perturbation frequency without considering measured line/winch dynamics.

### E4 — simple closed-loop optimisation

Only if E3 + calibrated simulation show a repeatable useful gradient.

Start with deterministic bounded logic, not ML.

### E5 — payout branch

Requires:

- simulator evidence;
- defined abort/reversion behaviour;
- qualified tow/safety review;
- evidence that expected benefit justifies the new mode.

---

## 18. Performance objectives

Primary project metric:

`release height / initial available line`

Other useful metrics:

- `dh / d(line consumed)`
- vertical speed
- height / tow work
- height / time
- line consumed / height
- tension overshoot
- oscillation amplitude
- tow duration

Do not optimise only climb rate.

Do not optimise only line efficiency.

Likely final objective:

> maximise release height subject to safety limits, available line, acceptable tow duration, pilot workload and hardware constraints.

---

## 19. Complexity stage gates — revised

The previous gates were circular because they asked for measured adaptive gain before deciding whether to build adaptive control.

Use **two different kinds of gate**.

### Gate A — predicted residual opportunity

After M0/M1 are calibrated against E1 conventional tows:

| Calibrated model predicts possible gain over good baseline | Response |
|---:|---|
| `< ~5%` | Do not build height optimiser. Continue only if impedance/gust-control benefits justify the work. |
| `~5–10%` | Do E3 bounded perturbations with existing R0/R1 hardware. |
| `~10–20%` | Build deterministic experimental optimiser; R2 becomes defensible if model sensitivity says geometry/wind is limiting. |
| `> ~20%` | Strong research case, but first suspect/calibrate the model aggressively before adding complexity. |

### Gate B — measured evidence

After E3/E4:

| Repeatable measured gain over competent baseline | Response |
|---:|---|
| `< ~5%` | Stop height-optimisation branch. |
| `~5–10%` | Keep controller simple; avoid exotic sensors. |
| `~10–20%` | Advanced geometry/wind sensing may be worth it. |
| `> ~20%` | Strong case for deeper aero instrumentation and payout research. |

Thresholds are project-management heuristics, **not aerodynamic predictions**.

The simulator earns the right to justify experiments only after matching the real baseline.

---

## 20. Safety boundary

Non-negotiable:

- optimiser never owns hard tension/speed limits;
- telemetry loss cannot leave an unsafe held command;
- operator abort remains above optimisation;
- lockout/rotation response is more important than height;
- novel payout modes require separate hazard analysis;
- weak link is not a substitute for active lockout recognition/control;
- current Swedish/club rules and qualified tow procedures must be checked before flight tests.

BHPA describes rotation/lock-out as potentially divergent because increased tow-line tension accelerates the turn. Its procedure requires prompt tension reduction/removal when divergence develops.

The fairlead sensor is supplemental information, not permission to automate away visual/operator safety responsibilities.

---

## 21. Hunch register

| ID | Hunch | Status | Deciding evidence |
|---|---|---|---|
| H1 | Pay-in height is strongly front-loaded | **OBS / PARTIAL** | Real `h(line)` logs; M0 shows headwind can explain part of it |
| H2 | Canopy far behind pilot = wasted efficiency | **OPEN** | M2 + real polar/airspeed/AoA evidence |
| H3 | Good conventional **pay-in tension scheduling** leaves meaningful height unused | **OPEN, weakened by sailplane evidence** | higher-force project baselines + calibrated M0/M1; Eppler suggests high permissible force is already close to steady-state height optimum |
| H4 | Headwind greatly improves release height / line | **PUB + DER/SIM** | RWTH: +28 m at +2.5 m/s in 2009 case; 2011 sweep ~5 m/km/h (~18 m per m/s); quantify for paraglider logs |
| H5 | Electric dynamic response can improve gust handling | **OPEN, plausible** | line-mode ID + tension transient tests |
| H6 | Softer pull is more efficient | **OPEN, less central** | sign of `d(L/D)/dn` + higher-force project logs |
| H7 | Harder pull increases conventional release height | **PUB for Eppler steady sailplane model; magnitude OPEN for paraglider** | higher-force project logs; diminishing returns expected |
| H8 | Controlled payout materially improves final height | **OPEN, promising** | calibrated simulation then E5 |
| H9 | `wind/3` is correct payout speed | **NOT ESTABLISHED** | paraglider-specific optimisation |
| H10 | Fairlead angle is a complete height sensor | **NO** | local tangent ≠ guaranteed pilot bearing |
| H11 | Fairlead direction is worth early installation | **USEFUL, but no longer required for first build/no-build answer** | TowMonitor + GPS handles the main height/force/wind questions; fairlead remains tether/supplemental telemetry |
| H12 | TowMonitor barometric height + pilot-end force belong in V1 | **YES — highest priority** | directly measures objective and actual aircraft-end tow input |
| H13 | IMU is mandatory for V1 height | **NO, but cheap inside TowMonitor** | baro owns absolute height; IMU adds transient/oscillation data |
| H14 | Dedicated GPS is mandatory in TowMonitor | **NO** | use phone GPS; log its quality fields |
| H15 | Airspeed belongs in TowMonitor | **YES for research** | airspeed + phone GPS estimates headwind and separates wind effects from control effects |
| H16 | ~6 s tether/winch mode exists | **EST / OPEN** | static extension + dynamic ID |
| H17 | Active impedance control remains useful if height gain is null | **OPEN but independent** | tension/oscillation comparison |
| H18 | ML is required | **NO for V1** | physics/sysID first |
| H19 | 20–30% extra height is available | **OPEN; increasingly unlikely from tension scheduling alone** | main remaining high-gain mechanism is non-conventional line-length trajectory / payout |

---

## 22. Highest-value unknowns

1. How does the project's real `height / initial line` change as conventional tow force is increased through the normal approved operating range?
2. After a competent high-safe-force pay-in baseline, **how much residual gain remains for tension scheduling alone?**
3. Can M0/M1 reproduce both the project logs and the published sailplane `height/line` + wind sensitivity order of magnitude?
4. Can a **different line-length trajectory** — hold / near-zero reel / controlled payout / re-reel — beat conventional pay-in substantially?
5. What is the real tether/winch dynamic mode and damping, and therefore the usable bandwidth of the VESC advantage?
6. What is the sign/magnitude of `d(L/D)/dn` for the actual wing, if it remains necessary to distinguish candidate control laws?
7. Can bounded tension/reel modulation improve `dh/dline` without exciting pitch/tether oscillation?
8. Can TowMonitor reduce the remaining measurement uncertainty enough that further sensors are unnecessary?

---

## 23. Immediate next work

### Before more control theory

1. Identify exact current wing model/size.
2. Confirm exact published trim/polar data; keep `32 km/h` only as project input until verified.
3. Build a clean conventional baseline series from the existing 2026-08-14 logs plus new tows across the normal approved tow-force range. For every run preserve:
   - deployed line / reel revolutions;
   - tension/current and controller state;
   - time;
   - barometric height;
   - wind estimate and direction.
4. Explicitly plot `release height / initial line` against tow-force state and wind before adding optimisation complexity.
5. Calibrate drum fill → line length.
6. Measure actual tow-line load-extension.

### TowMonitor / logging

7. Build **TowMonitor V1** around the retained pilot-end node:
   - pilot-end load cell;
   - barometer/vario;
   - differential-pressure airspeed sensor on a forward lance;
   - IMU if low-cost in complexity;
   - LoRa + common timestamping.
8. Keep phone GPS as the GPS source and add raw logging of `accuracy`, `altitudeAccuracy` and GPS timestamps.
9. Synchronise TowMonitor / phone / winch logs at millisecond resolution. Preserve raw values before filtering/downsampling.
10. Validate load-cell force, barometric height, airspeed and line-length geometry before using the dataset for aerodynamic conclusions.
11. Keep 2-axis fairlead direction as a later tether/supplemental channel unless a specific unresolved question requires it earlier.

### Simulation

12. Rewrite M0 with the staged conventional baseline.
13. Calibrate it against real tow data **and** sanity-check its wind sensitivity against the RWTH sailplane order of magnitude.
14. Use TowMonitor pilot-end tension and estimated headwind as calibration inputs where available.
15. Add line drag/sag/elasticity in M1 before treating high-wind height predictions seriously.
16. Run sensitivity analysis; let that decide whether any sensor beyond TowMonitor is justified.

---

## 24. Sources to carry forward

### Project measurement architecture

- **TowMonitor — Pilot-End Tow Research Instrument (2026-08-16).**
  - Retained pilot-end node combining actual tow-force measurement, barometric height/vario, airspeed and IMU.
  - Phone remains GPS source; winch provides line/reel/control state.
  - Defined specifically to answer the build/no-build research questions before adding optimiser complexity.

### Tow practice / safety

- **BHPA Technical Manual, Amendment 24 (August 2025), Tow Launched Paragliding.**
  - 50% target tension initially until approximately 100 ft, then 100% target for optimum ascent.
  - Higher tensions increase risk much faster than height.
  - Rotation/lock-out is divergent; line/canopy-heading angle must remain limited and tension must be reduced/removed when divergence develops.
  - Tow-line elasticity and length affect tension response and must be considered.

### Sailplane winch-launch physics / numerical benchmark

- **Christoph Santel (2009), _Simulation of a Glider Winch Launch_, Deutscher Luft- und Raumfahrtkongress 2009.**  
  PDF: https://www.fzt.haw-hamburg.de/pers/Scholz/dglr/dlrk2009_ohneReview/Papers/121355.pdf  
  Six-DOF ASK 21 + winch + human controllers + FEM cable. Reference `439 m / 1000 m` in zero wind; `+28 m` at `+2.5 m/s` headwind; cable drag cost ~32 m; cable gravity ~2 m; aggressive pilot case +22 m but stalled.

- **Andreas Gäb & Christoph Santel (2011), “Numerical Simulation of Glider Winch Launches”, _Technical Soaring_ 35(3), OSTIV.**  
  PDF: https://ts.ostiv.org/index.php/ts/article/download/76/69  
  Reference `431 m / 1000 m` in zero wind with force rising from ~0.5 W to ~1.5 W; reported wind sensitivity roughly **5 m release altitude per km/h headwind ≈ 18 m per m/s**. Also clarifies that the modeled “flywheel” is equivalent drivetrain inertia in a diesel winch, not a flywheel-launch architecture.

- **Richard Eppler, _Windenschlepp — Sicherheit und optimale Ausklinkhöhe_.**  
  PDF: https://www.grambekerheide.de/wp-content/uploads/2011/04/windenstart_prof_eppler.pdf  
  §6 derives the steady conventional result that higher available `S/G` permits steeper climb and maximum theoretical release height comes from the largest permissible cable force, with clear diminishing returns at high `S/G`. This constrains tension-scheduling claims but does **not** study payout or dynamic reel trajectories.

### Project flight-log evidence

- **ParaWinch public flight logs, GitHub.**  
  Folder: https://github.com/Utskottet/parawinch/tree/main/docs/Flight%20logs  
  2026-08-14 v2.11 logs provide the first direct project `height + line` baseline. Treat `force_kg` as derived/unverified until tension calibration is improved.

### Paraglider aerodynamics / modelling

- **Peter F. Heatwole (2022), _Parametric Paraglider Modeling_, Cal Poly.**
  Open parametric paraglider dynamics/aerodynamics model; strong candidate for M2.

- **Robert Kulhánek (2019), “Identification of a degradation of aerodynamic characteristics of a paraglider due to its flexibility from flight test”, Aircraft Engineering and Aerospace Technology 91(6), 873–879.**
  Directly relevant to flexible-wing drag/polar degradation; reported flexibility-related drag varies substantially with flight speed for the tested paraglider.

- **J. Roskam / NASA CR-151970 (1975), _Methods for Estimating Stability and Control Derivatives of Conventional Subsonic Airplanes_.**
  Useful classical reference: to first order, neglecting Reynolds effects, `L/D` is independent of aircraft weight. Reynolds effects must be treated separately.

### Dynamic interaction / impedance

- **Neville Hogan (1985), “Impedance Control: An Approach to Manipulation”, Parts I–III, ASME Journal of Dynamic Systems, Measurement, and Control.**
  General foundation for deliberately shaping force-motion dynamic behaviour rather than controlling force alone.

### Tethered-wing optimisation — analogy only

- **M. L. Loyd (1980), “Crosswind Kite Power.”**
  Historical simplified reeling result; not directly transferable to paraglider height optimisation.

- **Modern pumping AWE optimal-control literature.**
  Useful conceptual precedent for jointly optimising tether force and reel speed, but different aircraft/objective.

### Line elasticity reference

- **LIROS heat-set Dyneema SK78 product data** show working stretch below ~1% for some constructions.
  This supports only the **order of magnitude** of the current stiffness hunch; it is not data for the actual tow line.

---

## 25. Revision audit — critique checked 2026-08-15

1. **§6 implausibly optimistic absolute output:** accepted. Strong warning added; 675 m retained only as an example of model optimism.
2. **§6 straw-man constant tension from launch:** accepted. Replaced by illustrative 50%→100% staged tension baseline.
3. **`α→0` endpoint asymptotic:** rejected. In this ODE it is a finite-time maximum-height zero crossing; derivation added.
4. **Fairlead sensor too late:** accepted with qualification. Moved before perturbation tests for supplemental safety/research telemetry; still not a complete lockout detector.
5. **Stage gates circular:** accepted. Split into calibrated-simulation Gate A and measured-evidence Gate B.
6. **Impedance/gust branch lost:** accepted. Restored as independent fallback research branch.
7. **~6 s line mode lost:** accepted as `EST/OPEN`. Restored with equation and required identification before E3.
8. **Missing positive Reynolds case:** accepted with correction. Added Case C, but Reynolds increase does not guarantee improved `L/D`.
9. **32 km/h “too low”:** not accepted as a reason to change the model input. It is project-specific input, not a generic trim-speed assumption. It remains flagged for verification against the exact wing.
10. **Missing sailplane numerical benchmark:** accepted. Added Santel 2009 and Gäb/Santel 2011 quantitative results, including `439/431 m from 1000 m`, wind sensitivity and cable-drag effect.
11. **Eppler relevance:** added with scope qualification. His steady-state analysis strongly constrains the ordinary tension-scheduling hypothesis but does not answer dynamic reel/payout optimisation.
12. **“Flywheel winch” interpretation:** corrected. RWTH's flywheel is an equivalent rotating-inertia model inside a diesel/throttle winch; it is not dynamically equivalent to a high-bandwidth VESC actuator.
13. **Unverified ~42% project figure:** replaced with actual 2026-08-14 log ratios `39.1%`, `36.8%`, and a lower-quality `29.7%` case.
14. **Project decision focus:** revised. Paraglider `L/D(n)` remains important but is no longer treated as the sole pivotal unknown; the main possible large-gain branch is now non-conventional line-length trajectory / payout after a stronger conventional baseline is established.

---


### v4 update — TowMonitor measurement architecture (2026-08-16)

15. **Pilot-end force promoted:** current/drum force remains useful, but actual pilot-end load-cell force is now the primary research tension measurement.
16. **TowMonitor created:** retained bridle/harness node with load cell + barometer/vario + airspeed + IMU + LoRa.
17. **Phone GPS retained:** no dedicated airborne GPS is required in TowMonitor V1; phone GPS quality fields must be logged.
18. **Airspeed promoted:** its principal role is headwind estimation with GPS so wind effects can be separated from control effects.
19. **Fairlead sensor demoted for the core build/no-build question:** still useful for tether/supplemental telemetry, but no longer required before establishing the main force/height/wind relationship.
20. **Research programme changed:** TowMonitor is now the preferred measurement platform for E1–E3 and for deciding whether the large smart-control project is justified.

---

## 26. Handoff instruction for future sessions

Do **not** start by proposing more sensors, ML, or a clever PID.

Start with:

> What new evidence from TowMonitor, the winch logs, simulation or literature changes one of the OPEN items?

When a new result appears, add only:

`observation → physics → assumptions → falsification test → status`

TowMonitor is now the default measurement platform. Do not propose extra sensors unless TowMonitor data leaves a specific ambiguity.

The key decision is not “can we make a smart winch?”

It is:

> **How much repeatable release-height gain exists over a competent conventional electric tow, and what is the minimum complexity needed to capture it safely?**
