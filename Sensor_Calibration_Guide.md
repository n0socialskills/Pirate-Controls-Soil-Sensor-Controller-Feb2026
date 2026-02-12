# Two‑Point Calibration — Using Solution Bottle Charts (Updated with TDC and UI mapping)

This guide explains how to compute a two‑point calibration (slope & intercept) using two calibration solutions (buffers) and your raw EC readings — tailored to the current UI in `html_content.cpp`. It covers both the Quick (direct) method and the Recommended (physical‑units first) method, explains the Temperature‑Dependent Chart (TDC), provides an interpolation example you can copy, lists exact UI field names/IDs and functions, and includes a glossary of abbreviations.

Important: keep units consistent. All values used in the formulas below must be in the same units (e.g., µS, µS/cm).

---

## 1 — Quick summary (one line)
Use two buffer solutions and the raw readings in them to compute slope & intercept. For accurate results, preferred flow is: convert raw→µS, temperature‑correct µS to the bottle reference temperature (T_ref) using the bottle TDC or a suitable model, then compute slope = (EC_high − EC_low)/(raw_high − raw_low) and intercept = EC_low − slope × raw_low.

---

## 2 — Exact UI mapping (labels and input IDs)
These labels and IDs match the HTML/JS in `html_content.cpp` and are used in the calculations:

- Buffer reference inputs (25 °C)
  - "Low Buffer Solution — µS (@25 °C)" — id: `buf-low`
  - "High Buffer Solution — µS (@25 °C)" — id: `buf-high`

- Raw readings
  - "Low RawEC Reading" — id: `raw-low`
  - "High RawEC Reading" — id: `raw-high`

- Optional TDC (Temperature‑Dependent Chart) inputs — used in Manual TC mode, no measured‑temperature fields in UI
  - "TDC Low µS Buffer Solution µS" — id: `buf-low-tdc`
  - "TDC High µS Buffer Solution" — id: `buf-high-tdc`

- Buttons / display
  - Calculate → `calculateCalibration()`
  - Copy to Inputs → `applyCalculatedCal()`
  - Save Calibration → `saveCal()`
  - Mode radio group: `name="cal-method"` values: `quick` (No TC) and `chart` (Manual TC)
  - Calculated display elements: `calc-slope`, `calc-intercept`
  - Calculation status message: `calc-msg`

Behavior rule (how UI chooses values)
- If `cal-method` = `chart` (Manual TC), the calculation will prefer `buf-*-tdc` values if they are numeric (non-empty). If a TDC field is empty, the fallback is the referenced `@25 °C` field (`buf-low`/`buf-high`).
- If `cal-method` = `quick` (No TC), the calculation uses `buf-low`/`buf-high` directly.

---

## 3 — Workflows

### 3.1 Quick (direct) method — simplest
When to use:
- The bottle chart values are the values you want firmware to output (or you already temperature-adjusted those chart values externally).

Steps:
1. Submerge probe in low buffer; record `raw_low` → enter in `Low RawEC Reading` (`raw-low`).
2. Submerge probe in high buffer; record `raw_high` → enter in `High RawEC Reading` (`raw-high`).
3. Enter chart values (the numbers you want the firmware to produce) into:
   - `Low Buffer Solution — µS (@25 °C)` (`buf-low`)
   - `High Buffer Solution — µS (@25 °C)` (`buf-high`)
4. Click `Calculate` (runs `calculateCalibration()`).
   - slope = (high_buf − low_buf) / (raw_high − raw_low)
   - intercept = low_buf − slope × raw_low
5. Click `Copy to Inputs` (`applyCalculatedCal()`), then `Save Calibration` (`saveCal()`).

Notes:
- No automatic temperature compensation occurs unless you entered values already adjusted for temperature.

### 3.2 Recommended (physical‑units first) method — preferred
When to use:
- The bottle chart maps a physical unit (µS/µS·cm) and you want accurate temperature handling.

High-level steps:
1. Measure `raw_low`, `raw_high` at measurement temperature `T_meas`.
2. Convert raw → µS (physical units).
3. Use the bottle TDC (preferred) or a temperature model to determine µS at `T_meas` (µS_meas → µS_ref if needed).
4. Map µS_ref to the chart EC if the chart uses a different EC mapping.
5. Compute slope/intercept using EC_chart values and raw readings:
   - slope = (EC_high_chart − EC_low_chart) / (raw_high − raw_low)
   - intercept = EC_low_chart − slope × raw_low
6. Copy to inputs and save.

Why:
- Applying temperature corrections in the µS domain is physically correct and keeps conversions maintainable.

---

## 4 — Temperature‑Dependent Chart (TDC) — what it is and how to use it

Short definition
- TDC = Temperature‑Dependent Chart. It is the table or curve on the bottle that shows conductivity (µS or µS/cm) vs temperature. Use TDC to find the correct µS at your measurement temperature.

Why it matters
- If `T_meas` ≠ chart reference temperature `T_ref` (commonly 25 °C), the µS value to use for calibration should be the TDC value at `T_meas`, not the 25 °C reference.

How to obtain µS@T_meas from the TDC
- If the chart lists your exact temperature, copy that value.
- If not, linearly interpolate between the two nearest table rows — this is usually accurate for small temperature differences.
- Some bottles supply multiplicative correction factors. If present, use the bottle instructions.

Linear interpolation formula
- Given (T0, V0) and (T1, V1) and target T (T_meas between T0 and T1):
  - V_meas = V0 + (V1 − V0) × (T_meas − T0) / (T1 − T0)

Numeric example (ready to copy)
- Bottle rows:
  - 20.0 °C → 1340 µS
  - 25.0 °C → 1413 µS
  - 30.0 °C → 1488 µS
- Measured at T_meas = 22.0 °C:
  - V_meas = 1340 + (1413 − 1340) × (22 − 20) / (25 − 20)
  - V_meas = 1340 + 73 × 2/5 = 1340 + 29.2 = 1369.2 µS
- Paste 1369.2 into the matching TDC input (`buf-low-tdc` or `buf-high-tdc`).

Tip
- If the bottle provides a correction factor f(T): µS@T = µS@25°C × f(T). Use the bottle-provided method where available.

---

## 5 — Calculation rules & validation
- Avoid division by zero: `raw_high` must differ from `raw_low`.
- Units: ensure `buf-*` and `buf-*-tdc` values are in the same units (µS vs µS/cm).
- After `saveCal()`: verify by placing the probe back in each buffer — computed EC should match bottle numbers within tolerance.
- Suggested verification tolerance: a few percent depending on probe stability; repeat measurements and use averages where possible.

---

## 6 — Abbreviations and glossary (concise)
- µS — microsiemens (conductivity).
- µS/cm — microsiemens per centimeter.
- EC — electrical conductivity (generic).
- rawEC / raw — raw sensor reading (pre-calibration).
- TDC — Temperature‑Dependent Chart (bottle table/curve).
- TC — Temperature Compensation.
- No TC — No Temp Compensation mode (use referenced values).
- Manual TC — Manual Temp Compensation (TDC fields override referenced values if present).
- α — fractional change per °C used in multiplicative temperature correction (e.g., α ≈ 0.02).
- slope — linear multiplier from raw → EC.
- intercept — linear additive offset from raw → EC.

---

## 7 — UI copy & tooltip suggestions (exact strings you can paste)

- Steps area (exact two lines for the UI Steps block):
  > No TC — enter µS values that match the temperature where you measured the probe (or the 25 °C reference if you measured at 25 °C).  
  > Manual TC — enter the bottle TDC µS values (µS at measured temperature) into the "TDC µS @meas" fields below. If left blank, referenced 25 °C values will be used.

- Tooltip for TDC inputs (one-liner):
  - "Optional: paste µS from the bottle Temperature‑Dependent Chart for your measurement temperature."

- Calculate result messages (what the UI should show in `calc-msg` after Calculate):
  - "Calculated using Manual TC (TDC values preferred)." — when Manual TC and at least one TDC value used.
  - "Calculated using No TC (referenced µS values)." — when No TC or no TDC value provided.

---

## 8 — Mapping to code (for maintainers)
- `toggleCalMethod()` — show/hide `buf-low-tdc` / `buf-high-tdc`, set short `calc-msg` guidance.
- `calculateCalibration()` — reads `buf-low`, `buf-high`, `raw-low`, `raw-high`, and (if `cal-method` == `chart`) attempts to read `buf-low-tdc`/`buf-high-tdc` and prefer them if numeric. Produces `calc-slope` / `calc-intercept` and status `calc-msg`.
- `applyCalculatedCal()` — copies calculated slope/intercept into `cal-slope`/`cal-intercept`.
- `saveCal()` — sends `{type:'update_calibration', slope, intercept, tempcoeff}` over WS.

---

## 9 — Reproducible example (end‑to‑end)
1. Measure:
   - raw_low = 19 at T_meas = 22 °C
   - raw_high = 885 at T_meas = 22 °C
2. Bottle chart:
   - low 25 °C reference = 84 µS, TDC at 22 °C interpolates to 80 µS (example)
   - high 25 °C reference = 1413 µS, TDC at 22 °C interpolates to 1370 µS (example)
3. UI:
   - Select Manual TC (`cal-method` = `chart`)
   - Enter `raw-low`=19, `raw-high`=885
   - Enter `buf-low`=84, `buf-high`=1413 (these are the 25 °C references)
   - Enter `buf-low-tdc`=80, `buf-high-tdc`=1370 (TDC values at 22 °C)
   - Click Calculate → the tool uses 80 & 1370 to compute slope/intercept
   - Copy to inputs, Save Calibration
4. Verify by measuring probe in low & high buffers again and checking reported EC.

---

## 10 — Final quick checklist for users
- Record raw readings carefully and average repeats.
- Confirm units on bottle and use same units in UI.
- If you used the TDC, paste the interpolated µS values into `buf-*-tdc`.
- Calculate, copy, save, then verify in both buffers.
- Keep a note of T_meas and TDC values used for future audits.

---

If you'd like, I can:
- Insert the interpolation numeric example and tooltip copy directly into `html_content.cpp`'s Steps/help block and add the tooltip attribute for the TDC fields; or
- Produce a short printable one‑page cheat sheet (PNG or printable markdown) with the interpolation example and checklist.

Tell me which to do next and I’ll update the file accordingly.
