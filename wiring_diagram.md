# 🔌 Wiring Diagram — MouseGlove

**Created by Sai Praveen Behara**

---

## Full Pin Table

| Component | Component Pin | XIAO ESP32-C3 Pin | Notes |
|---|---|---|---|
| MPU6050 | VCC | 3V3 | 3.3V supply |
| MPU6050 | GND | GND | Common ground |
| MPU6050 | SDA | D4 / GPIO6 | I2C data |
| MPU6050 | SCL | D5 / GPIO7 | I2C clock |
| MPU6050 | AD0 | GND | Sets I2C address to 0x68 |
| MPU6050 | INT | — | Not connected |
| Flex Sensor | Pin 1 | 3V3 | Via 10kΩ resistor to A0 |
| Flex Sensor | Pin 2 | GND | — |
| 10kΩ Resistor | — | A0 / GPIO2 → GND | Voltage divider pull-down |
| TTP223 #1 (Index) | VCC | 3V3 | — |
| TTP223 #1 (Index) | GND | GND | — |
| TTP223 #1 (Index) | OUT | D3 / GPIO4 | Digital input |
| TTP223 #2 (Middle) | VCC | 3V3 | — |
| TTP223 #2 (Middle) | GND | GND | — |
| TTP223 #2 (Middle) | OUT | D1 / GPIO3 | Digital input — **avoid GPIO6 (SDA)** |

---

## Flex Sensor Voltage Divider Schematic

```
        3V3 ──────────────────────┐
                                  │
                           [Flex Sensor]
                             (resistance
                           increases on bend)
                                  │
                     ┌────────────┤──→ A0 (GPIO2) — ADC Input
                     │
                  [10kΩ]
                     │
                    GND
```

**How it works:**
- Straight finger: Flex sensor ~10kΩ → A0 reads ~1650 mV (~2050 ADC counts)
- Bent finger: Flex sensor ~40–80kΩ → A0 reads higher voltage → higher ADC count
- `FLEX_THRESHOLD` is set to catch the transition point

---

## I2C Bus

```
XIAO ESP32-C3
┌──────────────────────────────────────┐
│  D4 (SDA) ───────────────────────────┼──→ MPU6050 SDA
│  D5 (SCL) ───────────────────────────┼──→ MPU6050 SCL
└──────────────────────────────────────┘
```

**Pull-up resistors:** The XIAO ESP32-C3 includes internal I2C pull-ups. External 4.7kΩ pull-ups may improve reliability over longer wire runs (>10 cm).

---

## ⚠️ GPIO Notes for XIAO ESP32-C3

| GPIO | Silk Label | Special Function | Safe for… |
|---|---|---|---|
| GPIO2 | A0 | ADC1 CH2 | ✅ Flex sensor analog read |
| GPIO3 | D1 | — | ✅ TTP223 Middle OUT |
| GPIO4 | D3 | — | ✅ TTP223 Index OUT |
| GPIO6 | D4 | **I2C SDA** | ❌ Don't use for touch sensors |
| GPIO7 | D5 | **I2C SCL** | ❌ Don't use for touch sensors |
| GPIO20 | D10 | **USB D+** | Avoid if USB active |
| GPIO21 | D9 | **USB D−** | Avoid if USB active |

---

## Physical Placement on Glove

```
        ┌──────────────────────┐
        │     Back of Hand     │
        │                      │
        │   [MPU6050] ← wrist  │
        │                      │
        │  Index  Middle       │
        │   [T1]   [T2]        │   T1/T2 = TTP223 modules
        │                      │
        │        [FLEX] ←── Pinky finger channel
        └──────────────────────┘
        
        [XIAO ESP32-C3] — mounted on back of hand or wrist band
        [Battery] — mounted on wrist strap or back of hand
```

**Tips:**
- Route flex sensor wire along the pinky finger and secure with elastic bands
- Mount TTP223 modules on the fingertip pads with double-sided tape
- Use flexible silicone wire (28–30 AWG) to preserve glove flexibility
- Hot-glue or heat-shrink wire joints to prevent breaks during movement
