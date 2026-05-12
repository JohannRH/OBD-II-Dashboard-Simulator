# OBD-II Dashboard Simulator
### ESP32-C3 SuperMini · ESP-IDF v6 · C++ · FreeRTOS

A real-time automotive dashboard simulator running on an ESP32-C3 microcontroller. Displays live simulated engine data — RPM, vehicle speed, coolant temperature, and SAE J2012 Diagnostic Trouble Codes — on a 0.96" OLED screen over I2C.

Built with the ESP-IDF framework directly (no Arduino abstraction layer), targeting the RISC-V architecture via cross-compilation.

https://github.com/user-attachments/assets/a45480d7-68a3-4b01-a238-e8d4bb770df1

---

## Hardware

| Component | Part |
|---|---|
| Microcontroller | ESP32-C3 SuperMini |
| Display | OLED 0.96" SSD1306 (I2C, 128×64px) |

### Wiring

| OLED Pin | ESP32-C3 Pin |
|---|---|
| VCC | 3.3V |
| GND | G |
| SCL | GPIO7 |
| SDA | GPIO6 |

---

## What it displays

The device cycles through four screens automatically every 6 seconds.

**Screen 1 — Engine RPM**
Shows crankshaft speed in RPM with a throttle position bar. In a real car this comes from the crankshaft position sensor. At idle: ~800 RPM. Normal driving: 1500–3500 RPM. Redline: 6000–8000 RPM depending on engine.

**Screen 2 — Vehicle Speed**
Shows speed in km/h derived proportionally from RPM. In a real car this comes from wheel speed sensors (Hall effect sensors on each wheel hub) — the same sensors used by the ABS system.

**Screen 3 — Coolant Temperature**
Shows engine coolant temperature in °C. Normal operating range: 85–97°C. Displays an OVERHEAT warning above 105°C. In a real car this is an NTC thermistor read by the ECU via ADC.

**Screen 4 — Fault Codes (DTCs)**
Shows active Diagnostic Trouble Codes following the SAE J2012 standard. After 30 seconds the simulator automatically injects one of four real-world fault codes to demonstrate the DTC screen:

| Code | Description |
|---|---|
| P0300 | Random/multiple cylinder misfire detected |
| P0113 | Intake air temperature sensor high input |
| P0171 | System too lean, bank 1 |
| P0420 | Catalyst system efficiency below threshold |


---

## Architecture

The project is structured following separation of concerns, one module per subsystem, consistent with AUTOSAR software component principles.

```
obd2/
├── CMakeLists.txt              ← root build file
├── sdkconfig.defaults          ← ESP-IDF project config
├── components/
│   └── ssd1306/                ← SSD1306 I2C display driver
└── main/
    ├── CMakeLists.txt
    ├── config.hpp              ← all pin and timing constants
    ├── sensors.hpp / .cpp      ← VehicleState struct + simulation
    ├── display.hpp / .cpp      ← OLED rendering engine
    └── main.cpp                ← FreeRTOS tasks + app_main
```

### Two-task FreeRTOS design

```
task_sensors  — priority 5, runs every 20ms
  └── updates simulated RPM, speed, coolant temp, throttle
  └── cycles screen every 6 seconds
  └── injects a fault code at 30 seconds

task_display  — priority 4, runs every 1 second
  └── reads VehicleState and redraws the OLED
```

Separating tasks by frequency mirrors how production ECUs are structured: fast loops for sensor/control logic, slower loops for display and logging. The display task running at 1Hz never blocks the 20ms sensor task.

---

## Setup

### Requirements

- ESP-IDF v6.0 or later
- VS Code with the following extensions:
  - **ESP-IDF** by Espressif Systems
  - **C/C++** by Microsoft
  - **CMake Tools** by Microsoft

### Build and flash

1. Press `Ctrl+Shift+P` → **ESP-IDF: Set Espressif device target** → select `esp32c3`
2. Click the **🔨 Build** button in the bottom status bar
3. Plug in the ESP32-C3 via USB-C and select the COM port in the bottom status bar
4. Click the **⚡ Flash** button
5. Click the **🔌 Monitor** button to view serial output

> If flashing fails with "Failed to connect", hold the **BOOT** button on the ESP32 while clicking Flash, then release once "Connecting..." appears.
