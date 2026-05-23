# Smart IoT-Based Machine Health Monitoring System

> Real-time predictive maintenance using ESP32, three sensors, and ThingSpeak cloud — with hybrid threshold + adaptive fault detection.

![ESP32](https://img.shields.io/badge/MCU-ESP32-blue)
![Arduino](https://img.shields.io/badge/IDE-Arduino-teal)
![ThingSpeak](https://img.shields.io/badge/Cloud-ThingSpeak-orange)
![License](https://img.shields.io/badge/License-MIT-green)

---

## Overview

This project implements a **Smart IoT-Based Machine Health Monitoring System** that continuously monitors three critical machine parameters in real time:

| Parameter | Sensor | Protocol |
|---|---|---|
| Temperature | DS18B20 (Waterproof) | 1-Wire |
| Vibration | MPU6050 | I2C |
| Current | ACS712 | Analog (ADC) |

The ESP32 processes sensor data using a **hybrid detection algorithm** combining fixed threshold detection with adaptive mean-based normalisation. It classifies machine health into three states and triggers local alerts (LEDs + buzzer) and remote alerts (ThingSpeak cloud).

---

## Features

- **3-level fault classification**: Normal → Warning → Fault
- **Hybrid detection algorithm**: threshold-based + adaptive mean deviation
- **Local alerts**: Green / Yellow / Red LED + buzzer patterns
- **Cloud monitoring**: ThingSpeak dashboard with 4 real-time charts
- **Fault-tolerant uploads**: ThingSpeak upload skipped if sensors not ready
- **Auto-reconnect**: WiFi and sensor retry logic built-in
- **Cost-effective**: Total component cost ≈ ₹1155

---

## System Architecture

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   DS18B20   │     │   MPU6050   │     │   ACS712    │
│ Temperature │     │  Vibration  │     │   Current   │
│   1-Wire    │     │    I2C      │     │    ADC      │
└──────┬──────┘     └──────┬──────┘     └──────┬──────┘
       │                   │                   │
       └───────────────────┼───────────────────┘
                           ▼
                    ┌─────────────┐
                    │    ESP32    │
                    │ Edge Logic  │
                    │ Hybrid Algo │
                    └──────┬──────┘
              ┌────────────┼────────────┐
              ▼            ▼            ▼
        LED (×3)        Buzzer     ThingSpeak
       GPIO 25/26/27   GPIO 32     via WiFi
```

---

## Hardware Requirements

| Component | Specification | Qty | Cost (₹) |
|---|---|---|---|
| ESP32 Dev Module | Dual-core, WiFi, 12-bit ADC | 1 | 500 |
| DS18B20 Waterproof | 1-Wire, ±0.5°C | 1 | 80 |
| MPU6050 | 6-axis I2C IMU | 1 | 150 |
| ACS712 (20A) | Hall-effect current sensor | 1 | 120 |
| LED (Green, Yellow, Red) | 5mm | 3 | 15 |
| Active Buzzer | 5V | 1 | 30 |
| Resistors (220Ω × 3, 5.1kΩ × 1) | 1/4W | 4 | 8 |
| Breadboard + Jumper Wires | — | 1 set | 90 |
| USB Cable + 5V Supply | — | 1 set | 150 |
| **TOTAL** | | | **₹1,155** |

---

## Wiring

### DS18B20 (Waterproof)
```
Red   → 3.3V
Black → GND
Yellow→ GPIO 4
5.1kΩ pull-up resistor between GPIO 4 and 3.3V  ← MANDATORY
```

### MPU6050
```
VCC → 3.3V   GND → GND
SDA → GPIO 21   SCL → GPIO 22   AD0 → GND
```

### ACS712
```
VCC → 5V   GND → GND   OUT → GPIO 34
Load current must pass through IP+ and IP− pins
```

### LEDs (each with 220Ω resistor)
```
GPIO 25 → 220Ω → Green  LED → GND
GPIO 26 → 220Ω → Yellow LED → GND
GPIO 27 → 220Ω → Red    LED → GND
```

### Buzzer
```
GPIO 32 → Buzzer (+)   GND → Buzzer (−)
```

---

## Software Setup

### 1. Install Arduino IDE
Download from [arduino.cc](https://www.arduino.cc/en/software) and add ESP32 board support:
- Go to **File → Preferences → Additional Board Manager URLs**
- Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- Go to **Tools → Board → Boards Manager** → search `esp32` → Install

### 2. Install Libraries
In Arduino IDE go to **Tools → Manage Libraries** and install:
- `OneWire` by Jim Studt
- `DallasTemperature` by Miles Burton
- `Adafruit MPU6050` by Adafruit (install all dependencies)
- `ThingSpeak` by MathWorks

### 3. ThingSpeak Setup
1. Create account at [thingspeak.com](https://thingspeak.com)
2. Create new channel with 4 fields:
   - Field 1: Temperature (°C)
   - Field 2: Vibration (m/s²)
   - Field 3: Current (A)
   - Field 4: Machine Status
3. Copy your **Write API Key**

### 4. ACS712 Calibration
Before uploading the main sketch, run `calibration/acs712_calibration.ino` with **no load connected** to measure your board's actual zero offset voltage. Update `ACS712_ZERO_OFFSET` in the main sketch with the measured value.

> The theoretical zero offset is VCC/2 = 1.65V but varies per board. The calibrated value for this board was **1.884V**.

### 5. Configure Firmware
Open `firmware/machine_health_monitor/machine_health_monitor.ino` and update:
```cpp
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASS     = "YOUR_WIFI_PASSWORD";
unsigned long CHANNEL_ID  = YOUR_CHANNEL_ID;
const char* WRITE_API_KEY = "YOUR_WRITE_API_KEY";
#define ACS712_ZERO_OFFSET  1.884   // ← your calibrated value
```

### 6. Upload and Verify
- Select **Tools → Board → ESP32 Dev Module**
- Select correct COM port
- Click Upload
- Open Serial Monitor at **115200 baud**

---

## Threshold Values

| Parameter | Safe | Warn | Fault |
|---|---|---|---|
| Temperature | < 30°C | 30–40°C | ≥ 40°C |
| Vibration | < 10 m/s² | 10–15 m/s² | ≥ 15 m/s² |
| Current | < 0.5A | 0.5–1A | ≥ 1A |

To change thresholds, edit these lines in the firmware:
```cpp
#define TEMP_SAFE   30.0
#define TEMP_WARN   40.0
#define CURR_SAFE   0.5
#define CURR_WARN   1.0
#define VIB_SAFE    10.0
#define VIB_WARN    15.0
```

---

## Alert System

| State | LED | Buzzer |
|---|---|---|
| NORMAL | 🟢 Green ON | Silent |
| WARNING | 🟡 Yellow ON | 1 slow beep (200ms) |
| FAULT | 🔴 Red ON | 3 fast beeps (100ms each) |

---

## Serial Monitor Output

```
────────────────────────────────────
Vibration (m/s²) : 9.17   [SAFE ]
Temperature (°C) : 28.94  [SAFE ]
Current (A)      : 0.0000 [SAFE ]
>> OVERALL        : SAFE
ThingSpeak ✓ uploaded
────────────────────────────────────
```

---

## ThingSpeak Dashboard

Four real-time charts are generated automatically:
- **Field 1**: Temperature — smooth curve, sharp rise on overheating
- **Field 2**: Vibration — low at rest (~9.8 m/s²), high on disturbance
- **Field 3**: Current — near-zero idle, rises with load
- **Field 4**: Status step chart — 0 (Normal), 1 (Warning), 2 (Fault)

> Free ThingSpeak tier allows 1 update per **15 seconds** minimum.

---

## Troubleshooting

| Problem | Cause | Fix |
|---|---|---|
| Temperature = −127°C | Missing pull-up resistor | Add 4.7kΩ–5.1kΩ between DATA and 3.3V |
| Vibration = 0.00 | MPU6050 not initialising | Check SDA/SCL wiring; power cycle |
| Current ≠ 0 at no load | Wrong zero offset | Run calibration sketch, update ZERO_OFFSET |
| Sensors drop on startup | WiFi power spike | Already handled: WiFi connects first, then 2s delay |
| ThingSpeak error −401 | Uploading too fast | Already handled: 15s interval enforced |

---

## Repository Structure

```
machine-health-monitor/
│
├── README.md
├── LICENSE
│
├── firmware/
│   └── machine_health_monitor/
│       └── machine_health_monitor.ino
│
├── calibration/
│   └── acs712_calibration.ino
│
├── docs/
│   ├── report.docx
│   └── thingspeak_setup.md
│
└── results/
    ├── serial_monitor_output.txt
    └── thingspeak_screenshot.png
```

---

## Industry 4.0 Alignment

| Principle | Implementation |
|---|---|
| Smart Sensing | 3-sensor fusion (thermal + mechanical + electrical) |
| Edge Processing | Hybrid detection algorithm runs on ESP32 |
| IoT Connectivity | ThingSpeak cloud via WiFi |
| Predictive Maintenance | Mean-based adaptive fault detection |
| Cyber-Physical Integration | Physical sensors → digital cloud dashboard |

---

## License

MIT License — free to use, modify, and distribute with attribution.

---

## Acknowledgements

Submitted to **Prof. Piyush Ranjan**  
Electronics & Communication Engineering  
Pandit Deendayal Energy University (PDEU)  
Industry 4.0 Laboratory — 2024–25
