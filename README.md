# LD2420-Advanced-DSP
**A hardware-agnostic, probabilistic tracking and digital signal processing engine for the HLK-LD2420 24GHz mmWave radar.**

Typical mmWave radar implementations rely on basic, jitter-prone threshold logic. **LD2420-Advanced-DSP** throws out the simulated mock code and replaces it with mathematically grounded, double-precision algorithms.

Running natively on highly capable microcontrollers, this firmware utilizes a Singer-model Adaptive Kalman Filter (AKF) for sub-centimeter range tracking, a Viterbi-decoded Hidden Markov Model (HMM) for activity inference (Sitting, Standing, Walking), Goertzel-based micro-Doppler breathing extraction, and Dempster-Shafer evidence fusion.

## 🚀 Cross-Platform Architectures
This codebase contains 5 distinct firmware profiles optimized for the industry's most powerful microcontrollers. Open the `.ino` file that matches your hardware:

1. **`LD2420_Pico_Ultimate.ino` (Raspberry Pi Pico / RP2040)**
   - Pure USB CDC Serial streaming. 
   - Zero-overhead Hardware UART on `Serial1`.
2. **`LD2420_ESP32_WiFi.ino` (ESP32 / ESP32-S3 / ESP32-P4)**
   - Dual-Core RTOS hints.
   - High-speed WebSocket streaming via WiFi.
   - True HardwareSerial.
3. **`LD2420_ESP8266_WiFi.ino` (NodeMCU / Wemos D1 Mini)**
   - Optimized for legacy ESP8266 boards using SoftwareSerial.
   - Streams telemetry via WebSockets.
4. **`LD2420_Teensy_USB.ino` (Teensy 4.0 / 4.1)**
   - Unlocked for the 600MHz ARM Cortex-M7. 
   - Blazing fast USB telemetry and float-point math.
5. **`LD2420_STM32_Serial.ino` (STM32 Cortex-M4/M7)**
   - Bare-metal HardwareSerial optimized for STM32duino.

---

## 🔒 Zero-Config Wireless (ESP32 / ESP8266)
Security is a priority. There are **zero hardcoded WiFi credentials** or IP addresses in this repository.

### Captive Portal Configuration
When you flash an ESP32 or ESP8266, it will attempt to connect to its last known network. If it fails, it broadcasts its own secure access point: **`LD2420_Setup`**.
Connect to this network with your phone to securely input your home WiFi credentials.

### mDNS Zero-Config Discovery
You do not need to hunt for IP addresses. The ESP automatically broadcasts its location via mDNS. You can reach the dashboard or REST API simply by navigating to:
`http://ld2420.local`

---

## 🔌 Hardware Pinouts (Exact Mapping)

### Setup A: Serial USB Boards (Pico, Teensy, STM32)
| MCU Pin | LD2420 Pin | Notes |
|---|---|---|
| **5V / VBUS** | **VCC** | Supply stable 5V power from the USB rail. |
| **GND** | **GND** | Common Ground. |
| **TX Pin (UART)** | **RX** | MCU transmits commands to LD2420. |
| **RX Pin (UART)** | **TX** | MCU receives telemetry from LD2420. |
| **Hardware Int** | **OT2** | MCU reads hardware doppler motion state. |

### Setup B: ESP32 / ESP8266 (WiFi)
> **Note:** Check the specific `.ino` file for exact pin mappings, as ESP8266 uses SoftwareSerial (D5/D6) while ESP32 uses HardwareSerial.

| ESP Pin | LD2420 Pin | Notes |
|---|---|---|
| **5V / VIN** | **VCC** | Supply stable 5V power from the USB rail. |
| **GND** | **GND** | Common Ground. |
| **MCU TX** | **RX** | Transmit commands. |
| **MCU RX** | **TX** | Receive telemetry. |
| **GPIO Int**| **OT2** | Hardware doppler state. |

---

## 🖥️ How to Use the Dashboard

The dashboard (`LD2420_Dashboard.html`) is a universal, static HTML file. You do **not** need an internet connection to use it. Simply double-click the file to open it locally in your browser.

1. Open `LD2420_Dashboard.html` in **Google Chrome** or **Microsoft Edge**.
2. Click the floating **Settings / Connection** button (the gear icon) in the bottom right corner.
3. **If using USB (Pico/Teensy):** Click the **"Connect USB Serial"** button. Select your COM port. The dashboard reads the JSON telemetry straight from the USB cable using the Web Serial API.
4. **If using WiFi (ESP32/ESP8266):** Enter `ld2420.local` into the input box and hit enter. The dashboard will open a high-speed WebSocket.

## 🛠️ Modifying the Logic

If you wish to modify the Advanced DSP logic, open `LD2420_AKF_HMM_NoLimits.hpp`. 
- The `A[HMM_STATES][HMM_STATES]` matrix determines the exact mathematical probability chain the Hidden Markov engine uses to guess the user's activity.
- The `sigma_m` and `tau_m` parameters govern the responsiveness of the Singer Kalman Filter.
- Bio-vitals (Breathing rate) use a Goertzel algorithm and only extract data when the subject is completely still at <1.5m.
