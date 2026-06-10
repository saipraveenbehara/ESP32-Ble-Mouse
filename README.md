# 🖱️ MouseGlove — BLE HID Mouse Glove

> A wearable gesture-based Bluetooth mouse built with the **Seeed XIAO ESP32-C3**, MPU6050 IMU, a flex sensor, and TTP223 capacitive touch sensors. Control your cursor with hand movement and finger gestures — no desk required.

**Created by Sai Praveen Behara**

---

## 📸 Overview

| Feature | Details |
|---|---|
| Microcontroller | Seeed XIAO ESP32-C3 |
| Wireless | Bluetooth LE (BLE HID) |
| Cursor Control | Gyroscope (wrist tilt) |
| Drift Correction | Complementary filter (gyro + accel) |
| Mouse Toggle | Pinky flex sensor |
| Left Click / Scroll | Index finger TTP223 |
| Right Click | Middle finger TTP223 |
| Target OS | Windows (any BLE HID-compatible OS) |

---

## 🧰 Hardware Required

| Component | Qty | Notes |
|---|---|---|
| Seeed XIAO ESP32-C3 | 1 | Main MCU — BLE + ADC |
| MPU6050 IMU Module | 1 | I2C — mounted on wrist |
| Flex Sensor (2.2") | 1 | Analog — pinky finger |
| 10kΩ Resistor | 1 | Voltage divider for flex sensor |
| TTP223 Touch Module | 2 | Index finger + Middle finger |
| LiPo Battery (optional) | 1 | 3.7V, 150–400mAh |
| Thin glove | 1 | For mounting components |
| Hookup wire / ribbon | — | Flexible wiring |

---

## 🔌 Wiring Diagram

### MPU6050 → XIAO ESP32-C3

```
MPU6050 Pin    →    XIAO ESP32-C3
─────────────────────────────────
VCC            →    3V3
GND            →    GND
SDA            →    D4 / GPIO6  (I2C SDA)
SCL            →    D5 / GPIO7  (I2C SCL)
AD0            →    GND         (I2C address 0x68)
INT            →    (not used)
```

### Flex Sensor (Pinky)

```
                     3V3
                      │
                   [Flex Sensor]
                      │
          ┌───────────┤ A0 (GPIO2)
          │
        [10kΩ]
          │
         GND
```

### TTP223 Touch Modules

```
TTP223 #1 (Index Finger)
  VCC  →  3V3
  GND  →  GND
  OUT  →  D3 / GPIO4

TTP223 #2 (Middle Finger)
  VCC  →  3V3
  GND  →  GND
  OUT  →  D4 / GPIO3  ← Verify this does not conflict with I2C SDA!
```

> ⚠️ **GPIO Conflict Warning:** On the XIAO ESP32-C3, `D4/GPIO6` is the I2C SDA line. **Use `D1/GPIO2` or `D2/GPIO3` for the middle finger TTP223.** Update `PIN_TOUCH_MIDDLE` in the sketch accordingly.

---

## 🗂️ Project Structure

```
MouseGlove/
├── src/
│   └── MouseGlove.ino          # Main Arduino sketch
├── hardware/
│   └── wiring_diagram.md       # Detailed wiring reference (this file)
├── docs/
│   └── calibration_guide.md    # How to tune the glove
├── README.md                   # This file
└── library.properties          # (optional) dependency list
```

---

## 📦 Library Dependencies

Install all via the **Arduino IDE Library Manager** or **PlatformIO**:

| Library | Version | Install |
|---|---|---|
| **NimBLE-Arduino** | ≥ 1.4.0 | `h2zero/NimBLE-Arduino` |
| **HijelHID_BLEMouse** | latest | [GitHub](https://github.com/Hijel/HijelHID_BLEMouse) |
| **Adafruit MPU6050** | ≥ 2.2.4 | Arduino Library Manager |
| **Adafruit Unified Sensor** | ≥ 1.1.9 | Arduino Library Manager |

### Arduino IDE Board Setup

1. Add ESP32 board support URL in **Preferences**:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Install **esp32** by Espressif via **Board Manager**
3. Select **Board**: `XIAO_ESP32C3`
4. **Upload Speed**: 921600
5. **USB CDC On Boot**: Enabled (for Serial debug)

### PlatformIO (platformio.ini)

```ini
[env:seeed_xiao_esp32c3]
platform  = espressif32
board     = seeed_xiao_esp32c3
framework = arduino

lib_deps =
    h2zero/NimBLE-Arduino @ ^1.4.0
    adafruit/Adafruit MPU6050 @ ^2.2.4
    adafruit/Adafruit Unified Sensor @ ^1.1.9
    ; HijelHID_BLEMouse — install manually (see README)
```

---

## 🚀 Getting Started

### 1. Assemble Hardware
Wire all components as shown in the wiring table above. Attach them to a thin glove with double-sided tape or heat-shrink tubing.

### 2. Install Libraries
Install all four libraries listed above.

### 3. Flash the Sketch
Open `src/MouseGlove.ino` in Arduino IDE, select the correct board and port, and click **Upload**.

### 4. Pair via Bluetooth
- Open **Bluetooth Settings** on Windows
- Click **Add device → Bluetooth**
- Select **MouseGlove**
- Confirm pairing

### 5. Activate & Use
| Gesture | Action |
|---|---|
| Curl pinky | Toggle mouse ON/OFF |
| Tilt wrist | Move cursor |
| Tap index finger | Left click |
| Hold index finger | Enter scroll mode — tilt to scroll |
| Release index (from scroll) | Exit scroll mode |
| Tap middle finger | Right click |

---

## ⚙️ Calibration

All tunable constants are in the `CALIBRATION` section at the top of `MouseGlove.ino`. See [`docs/calibration_guide.md`](docs/calibration_guide.md) for a step-by-step guide.

| Parameter | Default | Effect |
|---|---|---|
| `FLEX_THRESHOLD` | `600` | ADC value to detect pinky curl |
| `GYRO_SENSITIVITY` | `1.5` | Cursor speed multiplier |
| `DEAD_ZONE` | `0.08` | Minimum gyro movement to register |
| `COMP_FILTER_ALPHA` | `0.95` | Gyro vs accel blend (drift correction) |
| `TOUCH_DEBOUNCE_MS` | `50` | Touch bounce suppression |
| `HOLD_THRESHOLD_MS` | `600` | Hold duration to enter scroll mode |
| `SCROLL_SENSITIVITY` | `0.10` | Scroll speed multiplier |
| `LOOP_INTERVAL_MS` | `10` | Update rate (~100 Hz) |

---

## 🐛 Debugging

Open **Serial Monitor** at **115200 baud** to see real-time logs:

```
[MouseGlove] Initialising...
[OK] MPU6050 detected.
[OK] BLE advertising started. Pair 'MouseGlove' on your PC.
[MouseGlove] Ready. Curl pinky to activate.
[Glove] Mouse ACTIVE
[BLE] Left Click
[BLE] Scroll Mode ON
[BLE] Scroll Mode OFF
[BLE] Right Click
```

**To tune the flex sensor**, uncomment this line in `readFlexToggle()`:
```cpp
// Serial.print(F("Flex: ")); Serial.println(raw);
```
Watch the raw ADC value; set `FLEX_THRESHOLD` midway between straight and curled readings.

---

## 🔋 Power Consumption (Approximate)

| State | Current Draw |
|---|---|
| BLE connected + mouse active | ~35–50 mA |
| BLE connected + mouse idle | ~20–30 mA |
| Deep sleep (not implemented) | ~5 µA |

A 300 mAh LiPo battery provides approximately **6–15 hours** of use.

---

## 🗺️ Roadmap

- [ ] Deep sleep when idle for > 60 seconds
- [ ] LED indicator for mouse active / BLE connected states
- [ ] Double-tap middle finger for middle click
- [ ] Kalman filter option for improved drift correction
- [ ] Battery level reporting over BLE
- [ ] Gesture macro support (assign shortcuts)

---

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgements

- [h2zero/NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) — Lightweight BLE stack for ESP32
- [Hijel/HijelHID_BLEMouse](https://github.com/Hijel/HijelHID_BLEMouse) — NimBLE HID Mouse library
- [Adafruit MPU6050](https://github.com/adafruit/Adafruit_MPU6050) — IMU driver
- Seeed Studio — XIAO ESP32-C3 hardware platform
