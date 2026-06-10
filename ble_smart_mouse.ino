/**
 * ============================================================
 *  BLE HID Mouse Glove — Seeed XIAO ESP32-C3
 * ============================================================
 *  Created by  : Sai Praveen Behara
 *  Version     : 1.0.0
 *  Date        : 2025
 *  License     : MIT
 *
 *  Description :
 *    A wearable gesture-based Bluetooth HID mouse glove using
 *    the Seeed XIAO ESP32-C3 microcontroller. Translates hand
 *    orientation into cursor movement, and finger gestures into
 *    mouse button clicks and scroll events.
 *
 *  Hardware :
 *    - Seeed XIAO ESP32-C3
 *    - MPU6050 IMU (I2C, mounted on wrist)
 *    - 1x Analog Flex Sensor (pinky finger, voltage divider 10kΩ)
 *    - 2x TTP223 Capacitive Touch Modules (index & middle fingers)
 *
 *  Control Scheme :
 *    - Flex Sensor (Pinky)  → Toggle mouse ON / OFF
 *    - Gyroscope X/Y        → Cursor Y / X movement
 *    - Accelerometer        → Drift stabilisation (complementary filter)
 *    - Index Touch Tap      → Left Click
 *    - Index Touch Hold     → Scroll Mode (tilt to scroll)
 *    - Middle Touch Tap     → Right Click
 *
 *  Dependencies :
 *    - ESP32 Arduino Core (NimBLE-based BLE stack)
 *    - NimBLE-Arduino          : https://github.com/h2zero/NimBLE-Arduino
 *    - HijelHID_BLEMouse       : NimBLE HID Mouse library
 *    - Adafruit MPU6050        : https://github.com/adafruit/Adafruit_MPU6050
 *    - Adafruit Unified Sensor : https://github.com/adafruit/Adafruit_Sensor
 *
 *  Wiring Summary :
 *    See /hardware/wiring_diagram.md for full wiring table.
 *
 *  Calibration :
 *    All tunable parameters are grouped in the CALIBRATION section
 *    below. Adjust these values to match your hardware and preference.
 * ============================================================
 */

// ─────────────────────────────────────────────
//  INCLUDES
// ─────────────────────────────────────────────
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// BLE HID Mouse — NimBLE-based library
// Install: https://github.com/Hijel/HijelHID_BLEMouse  (or compatible NimBLE fork)
#include <BleMouseHID.h>   // ← Adjust header name to match your installed library

// ─────────────────────────────────────────────
//  PIN DEFINITIONS
// ─────────────────────────────────────────────
#define PIN_FLEX_SENSOR   A0   // Analog — flex sensor via 10kΩ voltage divider
#define PIN_TOUCH_INDEX   D3   // Digital — TTP223 index finger (Left Click / Scroll)
#define PIN_TOUCH_MIDDLE  D4   // Digital — TTP223 middle finger (Right Click)

// ─────────────────────────────────────────────
//  ██  CALIBRATION  ██  — Tune all values here
// ─────────────────────────────────────────────

// --- Flex Sensor (Pinky Toggle) ---
const int   FLEX_THRESHOLD      = 600;   // ADC value above which mouse is toggled ON
                                          // Range: 0–4095 (12-bit ADC). Increase if
                                          // mouse activates when finger is straight.

// --- Gyroscope Sensitivity ---
const float GYRO_SENSITIVITY    = 1.5f;  // Multiplier for raw gyro → cursor pixels.
                                          // Higher = faster cursor. Typical: 0.5–3.0

// --- Cursor Dead Zone ---
const float DEAD_ZONE           = 0.08f; // Gyro rad/s below this → no cursor movement.
                                          // Eliminates idle drift. Typical: 0.05–0.15

// --- Complementary Filter Weight ---
// Blends gyroscope (fast, drift-prone) with accelerometer (slow, stable)
// alpha = 0.0 → pure accel, alpha = 1.0 → pure gyro. Typical: 0.90–0.98
const float COMP_FILTER_ALPHA   = 0.95f;

// --- Touch Debounce ---
const unsigned long TOUCH_DEBOUNCE_MS   = 50;   // ms — ignore bounces shorter than this
const unsigned long HOLD_THRESHOLD_MS   = 600;  // ms — hold duration to enter Scroll Mode

// --- Scroll Sensitivity ---
const float SCROLL_SENSITIVITY  = 0.10f; // Gyro rad/s multiplier for scroll ticks.
                                          // Higher = faster scroll. Typical: 0.05–0.20

// --- Loop Timing ---
const unsigned long LOOP_INTERVAL_MS    = 10;   // ms between updates (~100 Hz)

// --- Flex Toggle Debounce ---
const unsigned long FLEX_DEBOUNCE_MS    = 300;  // ms — prevent rapid toggle on flex

// ─────────────────────────────────────────────
//  OBJECT INSTANCES
// ─────────────────────────────────────────────
Adafruit_MPU6050 mpu;

// BLE Mouse — adjust constructor args to match your library's API
// Common signatures:
//   BleMouseHID bleMouse("DeviceName", "Manufacturer", batteryLevel);
//   BleMouse    bleMouse("DeviceName");
BleMouseHID bleMouse("MouseGlove", "SaiPraveenBehara", 100);

// ─────────────────────────────────────────────
//  STATE VARIABLES
// ─────────────────────────────────────────────

// --- Mouse active toggle (flex sensor) ---
bool mouseActive       = false;
bool lastFlexState     = false;
unsigned long lastFlexToggleTime = 0;

// --- Complementary filter angles (radians) ---
float angleX = 0.0f;   // Pitch
float angleY = 0.0f;   // Roll

// --- Touch state machine (index finger) ---
enum IndexState { IDX_IDLE, IDX_PRESSED, IDX_SCROLL };
IndexState indexState        = IDX_IDLE;
unsigned long indexPressTime = 0;
bool indexLastRaw            = false;
unsigned long indexDebounceTime = 0;
bool scrollModeActive        = false;

// --- Middle finger touch state ---
bool middleLastRaw           = false;
unsigned long middleDebounceTime = 0;
bool middleDebounced         = false;

// --- Timing ---
unsigned long lastLoopTime   = 0;
unsigned long lastMPUTime    = 0;   // Timestamp of last MPU read (for dt calculation)

// ─────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ─────────────────────────────────────────────
bool  readFlexToggle();
bool  readDebouncedTouch(int pin, bool &lastRaw, unsigned long &debounceTime);
void  updateLeftClick(bool touched, float gyroX, float gyroY, float dt);
void  updateRightClick(bool touched);
void  updateCursor(float gyroX, float gyroY, float accelX, float accelY, float dt);
void  applyComplementaryFilter(float gyroX, float gyroY,
                               float accelX, float accelY, float dt);
int8_t gyroToCursorDelta(float gyroRad, float sensitivity);

// ─────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);   // Wait for serial on USB-only builds (optional)

  Serial.println(F("[MouseGlove] Initialising..."));

  // --- GPIO ---
  pinMode(PIN_TOUCH_INDEX,  INPUT);
  pinMode(PIN_TOUCH_MIDDLE, INPUT);
  // Flex sensor is analog; no pinMode needed for A0

  // --- I2C + MPU6050 ---
  Wire.begin();
  if (!mpu.begin()) {
    Serial.println(F("[ERROR] MPU6050 not found! Check wiring (SDA/SCL)."));
    while (true) { delay(1000); }  // Halt — cannot proceed without IMU
  }
  Serial.println(F("[OK] MPU6050 detected."));

  // Configure MPU6050 ranges
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);   // ±2g — sufficient for wrist tilt
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);        // ±250°/s — good for hand movement
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);     // Low-pass filter — reduces vibration

  // --- BLE Mouse ---
  bleMouse.begin();
  Serial.println(F("[OK] BLE advertising started. Pair 'MouseGlove' on your PC."));

  lastMPUTime  = micros();
  lastLoopTime = millis();
  Serial.println(F("[MouseGlove] Ready. Curl pinky to activate."));
}

// ─────────────────────────────────────────────
//  MAIN LOOP
// ─────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // Enforce fixed loop interval for consistent timing
  if (now - lastLoopTime < LOOP_INTERVAL_MS) return;
  lastLoopTime = now;

  // ── 1. Read MPU6050 ──────────────────────────
  sensors_event_t accelEvent, gyroEvent, tempEvent;
  mpu.getEvent(&accelEvent, &gyroEvent, &tempEvent);

  float gx = gyroEvent.gyro.x;    // rad/s — maps to cursor Y
  float gy = gyroEvent.gyro.y;    // rad/s — maps to cursor X
  // float gz = gyroEvent.gyro.z;  // (unused — yaw, not needed for 2D cursor)

  float ax = accelEvent.acceleration.x;  // m/s² — used for drift correction
  float ay = accelEvent.acceleration.y;

  // Compute dt in seconds since last MPU read
  unsigned long nowMicro = micros();
  float dt = (nowMicro - lastMPUTime) / 1e6f;
  dt = constrain(dt, 0.001f, 0.05f);   // Clamp: ignore stalls / huge first-frame
  lastMPUTime = nowMicro;

  // ── 2. Flex Sensor — Mouse Toggle ───────────
  bool flexCurled = readFlexToggle();
  if (flexCurled != lastFlexState) {
    if (now - lastFlexToggleTime > FLEX_DEBOUNCE_MS) {
      lastFlexToggleTime = now;
      if (flexCurled) {           // Rising edge — finger just curled
        mouseActive = !mouseActive;
        Serial.print(F("[Glove] Mouse "));
        Serial.println(mouseActive ? F("ACTIVE") : F("INACTIVE"));
      }
    }
    lastFlexState = flexCurled;
  }

  // ── 3. Touch Inputs (always read, act only when mouse active) ──
  bool indexTouched  = readDebouncedTouch(PIN_TOUCH_INDEX,  indexLastRaw,  indexDebounceTime);
  bool middleTouched = readDebouncedTouch(PIN_TOUCH_MIDDLE, middleLastRaw, middleDebounceTime);

  // ── 4. BLE Actions — only when connected and mouse active ──────
  if (bleMouse.isConnected() && mouseActive) {
    applyComplementaryFilter(gx, gy, ax, ay, dt);
    updateLeftClick(indexTouched, gx, gy, dt);
    updateRightClick(middleTouched);

    if (!scrollModeActive) {
      updateCursor(gx, gy, ax, ay, dt);
    }
  }
}

// ─────────────────────────────────────────────
//  FUNCTION: readFlexToggle
//  Returns true when pinky is curled beyond threshold
// ─────────────────────────────────────────────
bool readFlexToggle() {
  int raw = analogRead(PIN_FLEX_SENSOR);
  // Debug: uncomment below to tune FLEX_THRESHOLD via Serial Monitor
  // Serial.print(F("Flex: ")); Serial.println(raw);
  return (raw > FLEX_THRESHOLD);
}

// ─────────────────────────────────────────────
//  FUNCTION: readDebouncedTouch
//  Software debounce for TTP223 digital output.
//  Updates lastRaw and debounceTime by reference.
// ─────────────────────────────────────────────
bool readDebouncedTouch(int pin, bool &lastRaw, unsigned long &debounceTime) {
  bool currentRaw = digitalRead(pin);
  unsigned long now = millis();

  if (currentRaw != lastRaw) {
    debounceTime = now;
    lastRaw = currentRaw;
  }

  if ((now - debounceTime) > TOUCH_DEBOUNCE_MS) {
    return currentRaw;   // Stable state
  }
  // Still within debounce window — return last stable state
  // (approximated by the opposite of lastRaw before the change)
  return !currentRaw;
}

// ─────────────────────────────────────────────
//  FUNCTION: updateLeftClick
//  State machine:
//    IDLE    → touched → PRESSED (start timer)
//    PRESSED → hold > threshold → SCROLL mode
//    PRESSED → released < threshold → Left Click
//    SCROLL  → released → exit scroll mode
// ─────────────────────────────────────────────
void updateLeftClick(bool touched, float gyroX, float gyroY, float dt) {
  unsigned long now = millis();

  switch (indexState) {

    case IDX_IDLE:
      if (touched) {
        indexState    = IDX_PRESSED;
        indexPressTime = now;
      }
      break;

    case IDX_PRESSED:
      if (!touched) {
        // Released before hold threshold → Left Click
        bleMouse.click(MOUSE_LEFT);
        Serial.println(F("[BLE] Left Click"));
        indexState = IDX_IDLE;
      } else if ((now - indexPressTime) >= HOLD_THRESHOLD_MS) {
        // Held long enough → enter Scroll Mode
        indexState    = IDX_SCROLL;
        scrollModeActive = true;
        Serial.println(F("[BLE] Scroll Mode ON"));
      }
      break;

    case IDX_SCROLL:
      if (!touched) {
        // Released → exit scroll mode
        scrollModeActive = false;
        indexState = IDX_IDLE;
        Serial.println(F("[BLE] Scroll Mode OFF"));
      } else {
        // Still held → use gyroX for vertical scroll
        // Apply dead zone
        float scrollAxis = gyroX;   // Tilt wrist forward/back to scroll
        if (fabsf(scrollAxis) > DEAD_ZONE) {
          // Accumulate fractional scroll to avoid sending too many events
          static float scrollAccum = 0.0f;
          scrollAccum += scrollAxis * SCROLL_SENSITIVITY * (dt * 1000.0f);

          int scrollTicks = (int)scrollAccum;
          if (scrollTicks != 0) {
            bleMouse.move(0, 0, (int8_t)constrain(-scrollTicks, -127, 127));
            scrollAccum -= scrollTicks;
          }
        }
      }
      break;
  }
}

// ─────────────────────────────────────────────
//  FUNCTION: updateRightClick
//  Tap-to-click for Right Click.
//  Detects rising edge (press) for single right click.
// ─────────────────────────────────────────────
void updateRightClick(bool touched) {
  // Edge detection — fire on rising edge only
  static bool prevMiddle = false;

  if (touched && !prevMiddle) {
    bleMouse.click(MOUSE_RIGHT);
    Serial.println(F("[BLE] Right Click"));
  }
  prevMiddle = touched;
}

// ─────────────────────────────────────────────
//  FUNCTION: applyComplementaryFilter
//  Blends gyroscope integration with accelerometer
//  tilt angle to correct long-term gyro drift.
//  Updates global angleX, angleY.
// ─────────────────────────────────────────────
void applyComplementaryFilter(float gyroX, float gyroY,
                               float accelX, float accelY, float dt) {
  // Accelerometer-derived tilt angles (in radians)
  // accel values in m/s²; divide by g≈9.81 for normalised tilt
  float accelAngleX = atan2f(accelY, 9.81f);
  float accelAngleY = atan2f(-accelX, 9.81f);

  // Complementary filter
  angleX = COMP_FILTER_ALPHA * (angleX + gyroX * dt)
         + (1.0f - COMP_FILTER_ALPHA) * accelAngleX;
  angleY = COMP_FILTER_ALPHA * (angleY + gyroY * dt)
         + (1.0f - COMP_FILTER_ALPHA) * accelAngleY;
}

// ─────────────────────────────────────────────
//  FUNCTION: updateCursor
//  Maps gyro angular velocity to BLE mouse move.
//  Gyro X → Cursor Y  |  Gyro Y → Cursor X
// ─────────────────────────────────────────────
void updateCursor(float gyroX, float gyroY,
                  float accelX, float accelY, float dt) {
  // Apply dead zone to suppress idle jitter
  float motionX = (fabsf(gyroY) > DEAD_ZONE) ? gyroY : 0.0f;
  float motionY = (fabsf(gyroX) > DEAD_ZONE) ? gyroX : 0.0f;

  // Convert to integer cursor delta using sensitivity multiplier
  // Negative signs correct for natural hand-tilt direction mapping.
  // Flip signs here if cursor moves in wrong direction.
  int8_t dx = gyroToCursorDelta( motionX,  GYRO_SENSITIVITY);  // Cursor X ← gyroY
  int8_t dy = gyroToCursorDelta(-motionY,  GYRO_SENSITIVITY);  // Cursor Y ← gyroX (inverted)

  if (dx != 0 || dy != 0) {
    bleMouse.move(dx, dy, 0);   // (deltaX, deltaY, scroll)
  }
}

// ─────────────────────────────────────────────
//  FUNCTION: gyroToCursorDelta
//  Converts gyro rad/s to int8 cursor pixel delta.
//  Clamps to ±127 (HID mouse report limit).
// ─────────────────────────────────────────────
int8_t gyroToCursorDelta(float gyroRad, float sensitivity) {
  float raw = gyroRad * sensitivity * (1000.0f / LOOP_INTERVAL_MS);
  // Scale down — raw gyro values can be large; empirical divisor 10 works well
  raw /= 10.0f;
  return (int8_t)constrain((int)raw, -127, 127);
}
