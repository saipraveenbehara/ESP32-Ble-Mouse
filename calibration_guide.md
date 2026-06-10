# 🎛️ Calibration Guide — MouseGlove

**Created by Sai Praveen Behara**

This guide walks you through tuning each calibration parameter in `MouseGlove.ino` for your specific hardware and hand geometry.

---

## Step 1 — Flex Sensor Threshold (`FLEX_THRESHOLD`)

The flex sensor determines when your pinky is curled to toggle the mouse on/off.

1. Open **Serial Monitor** at 115200 baud.
2. Uncomment the debug line in `readFlexToggle()`:
   ```cpp
   Serial.print(F("Flex: ")); Serial.println(raw);
   ```
3. Note the ADC reading with your **pinky straight** → call it `V_straight`
4. Note the ADC reading with your **pinky fully curled** → call it `V_curled`
5. Set `FLEX_THRESHOLD` to the midpoint:
   ```
   FLEX_THRESHOLD = (V_straight + V_curled) / 2
   ```

**Example:** Straight = 400, Curled = 800 → Set threshold to `600`.

---

## Step 2 — Cursor Speed (`GYRO_SENSITIVITY`)

Controls how fast the cursor moves in response to wrist tilt.

- **Too slow?** Increase `GYRO_SENSITIVITY` (e.g., 2.0 → 3.0)
- **Too fast / shaky?** Decrease it (e.g., 1.5 → 1.0)
- Typical range: `0.8` – `3.0`

Start at `1.5` and adjust in steps of `0.25`.

---

## Step 3 — Dead Zone (`DEAD_ZONE`)

Eliminates cursor drift when your wrist is resting still.

- **Cursor drifts at rest?** Increase `DEAD_ZONE` (e.g., 0.08 → 0.12)
- **Cursor feels sluggish to start moving?** Decrease it (e.g., 0.08 → 0.05)
- Unit: rad/s. Typical range: `0.04` – `0.20`

---

## Step 4 — Complementary Filter Alpha (`COMP_FILTER_ALPHA`)

Controls how much the accelerometer is used to correct gyroscope drift over time.

| Value | Effect |
|---|---|
| `0.98` | Mostly gyro — very responsive, more drift |
| `0.95` | Balanced — recommended starting point |
| `0.90` | More accel correction — smoother but slightly laggy |
| `0.80` | Heavy correction — good for slow, deliberate movement |

If you notice the cursor slowly drifting in one direction over 30–60 seconds, **lower** alpha slightly.

---

## Step 5 — Scroll Sensitivity (`SCROLL_SENSITIVITY`)

Controls how fast the page scrolls when in Scroll Mode.

- **Scrolling too fast?** Decrease `SCROLL_SENSITIVITY` (e.g., 0.10 → 0.05)
- **Scrolling too slow?** Increase it (e.g., 0.10 → 0.20)

---

## Step 6 — Hold Threshold (`HOLD_THRESHOLD_MS`)

How long you must hold your index finger down before Scroll Mode activates.

- **Default:** `600` ms (0.6 seconds)
- **Accidental scroll triggers?** Increase to `800` or `1000`
- **Want faster scroll activation?** Decrease to `400`

---

## Step 7 — Loop Rate (`LOOP_INTERVAL_MS`)

Sets the update frequency in milliseconds.

| Value | Frequency | Notes |
|---|---|---|
| `5` ms | 200 Hz | Very responsive; higher BLE traffic |
| `10` ms | 100 Hz | Recommended — good balance |
| `20` ms | 50 Hz | Lower power; slightly less smooth |

---

## Quick Reference — Suggested Starting Values

```cpp
const int   FLEX_THRESHOLD      = 600;    // Tune to your flex sensor
const float GYRO_SENSITIVITY    = 1.5f;
const float DEAD_ZONE           = 0.08f;
const float COMP_FILTER_ALPHA   = 0.95f;
const unsigned long TOUCH_DEBOUNCE_MS  = 50;
const unsigned long HOLD_THRESHOLD_MS  = 600;
const float SCROLL_SENSITIVITY  = 0.10f;
const unsigned long LOOP_INTERVAL_MS   = 10;
```

---

## Axis Inversion

If the cursor moves in the **wrong direction** (e.g., tilting left moves cursor right), flip the sign in `updateCursor()`:

```cpp
// Current:
int8_t dx = gyroToCursorDelta( motionX,  GYRO_SENSITIVITY);
int8_t dy = gyroToCursorDelta(-motionY,  GYRO_SENSITIVITY);

// Invert X axis:
int8_t dx = gyroToCursorDelta(-motionX,  GYRO_SENSITIVITY);

// Invert Y axis:
int8_t dy = gyroToCursorDelta( motionY,  GYRO_SENSITIVITY);
```

---

*For further help, open an issue on the GitHub repository.*
