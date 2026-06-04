# Independent Long-Range Multi-Bounce Laser Security System

A production-grade perimeter defense and security grid engineered for the **ESP32 DevKit V1** platform using **PlatformIO** and the **Arduino framework**. This system utilizes an independent, constantly active external laser node and a high-reflectivity multi-mirror array to secure complex, long-range perimeters with zero data-wire overhead across the room.
<br><br>

---

## 🚀 Key Features

* **Independent Laser Topology:** Optimized for practical room deployment. The transmitter node operates entirely on its own power source across the room, completely eliminating the need for long signal wires running back to the controller.
* **Reflected Multi-Bounce Optimization:** Engineered to process focusable green ($532\text{nm}$) or red ($650\text{nm}$) industrial laser modules bounced across first-surface reflection arrays.
* **Dynamic Independent Calibration:** Dynamically measures laser targeting thresholds and profiles ambient light environments on boot, making the system highly adaptive to changing room illumination without needing to control the laser's power state.
* **Zero-Driver Audio Sirens:** Eliminates unstable, crash-prone microcontroller PWM audio subsystems (`ledc`) by driving hardware buzzers via low-level, high-frequency manual digital toggling loops.
* **Hardware Interruption Reset:** Features an internal-pullup hardware polling engine on `GPIO 18` that allows a standard push-button or a simple jumper-wire touch shortcut to instantly silence alarms and re-arm the perimeter.
<br>

## 📂 Directory Structure Layout
``` text
esp32-laser-security/          
├── .pio/                      
├── assets/                    <-- Create this folder for your media 📸
│   ├── system_schematic.png   <-- Your PCB circuit diagram/schematic image
│   └── demo_preview.gif       <-- Your 5-second project demonstration GIF
├── include/                   
├── lib/                       
├── src/                       
│   └── main.cpp               
├── .gitignore                 
├── platformio.ini             
└── README.md
```
<br>

---

## 🛠️ Hardware Requirements

| Component | Quantity | Purpose |
| :--- | :--- | :--- |
| **ESP32 DevKit V1 Board** | 1 | Central processing unit & real-time monitoring engine |
| **Industrial Dot Laser Module (5mW)** | 1 | Focusable transmitter node ($532\text{nm}$ Green recommended for long ranges) |
| **Light Dependent Resistor (LDR)** | 1 | High-sensitivity optical receiver |
| **Metal Film Resistor ($10\text{ k}\Omega$)** | 1 | Pull-down resistor for the analog voltage divider circuit |
| **Active Piezo Buzzer** | 1 | High-decibel audible dual-frequency warning siren |
| **Tactile Push-Button / Jumper Wire** | 1 | Instant manual system reset / alarm silencer |
| **First-Surface Mirrors / HDD Platters** | 2–4 | Zero-ghosting high-reflectivity corner reflection nodes |

<br>

---

## 🔌 Circuit Topology & Wiring

To achieve a clean optical baseline and prevent room lighting from flooding the sensor, the LDR must be housed inside an opaque, dark isolation tube pointed directly down the incoming laser path.

### Central Control Unit Pinout Mapping

```text
       +---------------------------------------------+
       |               ESP32 DEVKIT V1               |
       +---------------------------------------------+
          | GPIO 5  | -------> Active Buzzer (+)
          | GPIO 18 | <------- Reset Jumper Wire / Switch (Short to GND)
          | GPIO 34 | <------- LDR / 10kΩ Divider Junction (ADC1_CH6)
          | 3V3     | -------> LDR Input Power (+)
          | GND     | -------> Common System Ground Rail (Buzzer, Switch, Resistor)
```

### 3.3V Safe Voltage Divider Layout

```text
3V3 Rail -----[ LDR ]-----+-----> GPIO 34 (Analog Read Input)
                          |
                     [ 10kΩ Resistor ]
                          |
                         GND Rail
```
_Note: Powering the LDR network from the 3V3 rail protects the ESP32's 12-bit ADC pins from 5V over-voltage degradation._

<br>

## 💻 Software Configuration & Installation

This project is built using PlatformIO IDE inside VS Code for robust environment management and smaller, compiled binary footprints.
### Project Environment Configuration (platformio.ini)
```Ini, TOML

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    bblanchon/ArduinoJson @ ^7.0.0
```
### Deployment Instructions

1. Clone this repository to your local workspace:
    ```Bash
    git clone https://github.com/JaswantBhartiya/esp32-laser-security.git
    ```
    
2. Open the project folder directly inside Visual Studio Code with the PlatformIO extension active.

3. Align your external laser node across your mirror grid so it hits the center of the LDR tube.

4. Click the PlatformIO: Upload arrow icon on the bottom status bar (or press (`Ctrl + Alt + U`) to compile and flash the firmware.

5. Open the Serial Monitor (`Ctrl + Alt + M`) at `115200` baud to watch the system run its initial calibration profiling.
<br><br>

## ⚙️ How System States Work
```text
       +------------------+
       | STATE_CALIBRATING| <-------- On Boot / User Reset
       +------------------+
                 |
                 v (Captures Max Beam Intensity & Computes Median)
       +------------------+
  +--->|   STATE_ARMED    |
  |    +------------------+
  |              |
  |              v (LDR Reading drops below threshold for >50ms)
  |    +------------------+
  |    |  STATE_BREACHED  |
  |    +------------------+
  |              |
  +--------------+ (GPIO 18 Shorted to GND / Reset Wired Activated)
```

1. Optical Profiling (Boot): The system samples the active beam alignment over a 3-second window to capture maximum intensity. It then models a mathematical median trigger threshold exactly halfway between the direct laser strength and background ambient lighting.

2. Active Guard Mode: The ESP32 continuously polls the internal 12-bit Analog-to-Digital Converter (`ADC1`). A software debounce filter requires the beam to be fully broken for more than `50ms` to prevent false alarms from flying bugs or floating dust.

3. Breached Alert Loop: When triggered, the system shifts into a high-priority alert state. The ESP32 generates a non-blocking dual-tone police siren sweep pattern (`800Hz` to `1300Hz`) using microseconds delay-toggling on `GPIO 5`.

4. Hardware Disarm: While driving the siren frequencies, the controller actively checks `GPIO 18`. The exact microsecond your reset button is pressed (or your jumper wires touch), the system immediately mutes the buzzer, runs a fresh optical room calibration, and shifts smoothly back to active protection mode.
<br><br>

## 📄 License

This project is licensed under the MIT License - see the `LICENSE` file for details.
