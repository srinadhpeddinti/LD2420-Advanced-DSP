# LD2420 Ultimate DSP & Edge AI Firmware

[![CI/CD Pipeline](https://github.com/srinadhpeddinti/LD2420-Advanced-DSP/actions/workflows/build.yml/badge.svg)](https://github.com/srinadhpeddinti/LD2420-Advanced-DSP/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A hyper-optimized, multi-core C++ Digital Signal Processing (DSP) and Machine Learning engine for the HiLink LD2420 mmWave 24GHz radar. Originally designed for the Raspberry Pi Pico (RP2040), but fully compatible with ESP32, ESP8266, STM32, and Teensy.

This firmware replaces simple manufacturer threshold-based detection with an enterprise-grade probabilistic tracking stack featuring Adaptive Kalman Filters, Hidden Markov Models, and TinyML algorithms running entirely local to the microcontroller.

---

## 📖 Table of Contents
1. [Mathematical Architecture & DSP Core](#-mathematical-architecture--dsp-core)
2. [Machine Learning & TinyML Upgrades](#-machine-learning--tinyml-upgrades)
3. [Embedded Software Architecture](#-embedded-software-architecture)
4. [Custom Binary Telemetry Protocol](#-custom-binary-telemetry-protocol)
5. [Hardware Pinouts & Connections](#-hardware-pinouts--connections)
6. [WiFi Network Provisioning (ESP32/ESP8266)](#-wifi-network-provisioning-esp32esp8266)
7. [Installation & Compilation](#-installation--compilation)
8. [DevOps & Verification](#-devops--verification)

---

## 🔬 Mathematical Architecture & DSP Core

### 1. Singer-Model Adaptive Kalman Filter (AKF)
Standard tracking filters fail to resolve targets that accelerate quickly (e.g. lunging or lung displacements) while maintaining stability when a target sits still. This implementation employs a continuous-time Singer Maneuver Model mapped into a discrete state-space formulation:

$$\mathbf{x}_k = \begin{bmatrix} p_k \\ v_k \\ a_k \end{bmatrix}$$

Where $p$ is range (cm), $v$ is target velocity (cm/s), and $a$ is acceleration ($\text{cm/s}^2$). 
- **Dynamic Transition Matrix ($\mathbf{F}$)**: Calculated dynamically using the Singer correlation time constant $\tau_m$.
- **Maneuver-Aware Covariance ($\mathbf{Q}$)**: Dynamically scales process noise based on velocity innovation to prevent filter lag during rapid target maneuvers.

### 2. Hidden Markov Model (HMM) & Viterbi Decoding
To avoid sudden "drops" (false negatives) in presence detection when a target is stationary (e.g., sitting or breathing), the state sequence is computed using a 3-state HMM:
1. `ABSENT` (Low energy, zero velocity)
2. `SITTING` (Static micro-movements, high static energy)
3. `WALKING` (Dynamic movement, high velocity variance)

The system models observation emissions as dynamic Gaussian probability density functions. It computes the most likely current sequence of states ($q_1, q_2, \dots, q_k$) over a rolling observation window to smooth out radar sensor noise natively.

---

## 🧠 Machine Learning & TinyML Upgrades

The namespace `UltimateML` implements several localized classifiers to analyze targets:
- **Support Vector Machine (SVM)**: Formulates a 3-class boundary (Standing, Sitting, Prone) using a **Radial Basis Function (RBF) Kernel**:
  $$K(\mathbf{x}, \mathbf{x}') = \exp(-\gamma ||\mathbf{x} - \mathbf{x}'||^2)$$
  It classifies posture in real-time based on Range Variance and Target Radar Cross Section (RCS).
- **Decision Tree Classifier**: Runs a CART node sequence to classify human targets vs household pets (dogs/cats) using velocity trends and micro-Doppler spectral energy.
- **Isolation Forest**: Computes continuous anomaly scores:
  $$S(x, n) = 2^{-\frac{E(h(x))}{c(n)}}$$
  Used to identify stagger patterns, irregular gait anomalies, or fall dynamics.
- **Sleep Stage Classification**: Processes breathing rate variability (BRV) to estimate sleep states (`Awake`, `Light`, `Deep`, `REM`).

---

## 🛠️ Embedded Software Architecture

### Multi-Core Isolation (RP2040)
For microcontrollers with dual-core layouts like the RP2040, the system isolates critical tasks to guarantee zero timing jitter:
- **Core 0**: Exclusively runs the hard real-time loop. It processes incoming UART data via DMA, executes the Adaptive Kalman Filter, updates the HMM transitions, and runs the TinyML classifiers.
- **Core 1**: Manages the serialization pipeline. It packs the computed data structures into the custom binary format and streams it over USB CDC or serial connections, ensuring blocking operations never affect the DSP timing loop.

---

## 📊 Custom Binary Telemetry Protocol

To reduce communication overhead, ASCII JSON telemetry is replaced with a packed 47-byte little-endian binary structure:

| Offset (Bytes) | Field Name | Data Type | Description |
|---|---|---|---|
| `0 - 1` | `magic` | `uint16_t` | Frame start identifier (`0xBEEF`) |
| `2 - 5` | `timestamp` | `uint32_t` | Monotonic system runtime (ms) |
| `6 - 9` | `range` | `float` | Estimated target distance (cm) |
| `10 - 13` | `velocity` | `float` | Estimated target velocity (cm/s) |
| `14 - 17` | `acceleration`| `float` | Estimated target acceleration ($\text{cm/s}^2$) |
| `18 - 21` | `rcs` | `float` | Radar Cross Section (RCS) Energy |
| `22` | `state` | `uint8_t` | HMM activity state (`0=ABSENT`, `1=SITTING`, `2=WALKING`) |
| `23` | `posture` | `uint8_t` | SVM Posture prediction (`0=Standing`, `1=Sitting`, `2=Prone`) |
| `24 - 27` | `anomaly_score`| `float` | Isolation Forest anomaly score (`0.0 - 1.0`) |
| `28 - 31` | `breath_rate` | `float` | Isolated respiratory rate (breathing bpm) |
| `32 - 45` | `reserved` | `uint8_t[14]`| Future payload padding |
| `46` | `checksum` | `uint8_t` | Modulo-256 byte sum verification |

---

## 🔌 Hardware Pinouts & Connections

### 1. Serial USB Setup (Pico, STM32, Teensy)
| MCU Pin | LD2420 Pin | Description |
|---|---|---|
| **5V / VBUS** | **VCC** | 5V Power Supply |
| **GND** | **GND** | Ground Reference |
| **TX Pin (UART)** | **RX** | Transmit command data |
| **RX Pin (UART)** | **TX** | Receive telemetry data |
| **GPIO Pin** | **OT2** | Out-of-target Hardware Interrupt |

### 2. Wireless Setup (ESP32, ESP8266)
| ESP Pin | LD2420 Pin | Description |
|---|---|---|
| **5V / VIN** | **VCC** | 5V Power Supply |
| **GND** | **GND** | Ground Reference |
| **MCU TX** | **RX** | Transmit command data |
| **MCU RX** | **TX** | Receive telemetry data |
| **GPIO Pin** | **OT2** | Out-of-target Hardware Interrupt |

---

## 🌐 WiFi Network Provisioning (ESP32/ESP8266)

The wireless firmware configurations feature automated provisioning:
1. **Captive Portal**: If the device cannot establish a connection to saved network credentials, it boots in AP mode, broadcasting a hotspot named **`LD2420_Setup`**. Connect to this network on your smartphone or PC to provision local network details.
2. **mDNS Resolution**: Once connected, the firmware advertises its location via multicast DNS. The dashboard or API can be accessed directly at `http://ld2420.local`.

---

## 🚀 Installation & Compilation

### Arduino IDE
The core logic files are organized as an **Arduino Library** to prevent code duplication across separate board-specific folders.
1. Copy or symlink the folder `libraries/LD2420_Ultimate` into your local Arduino directory (e.g. `Documents/Arduino/libraries/`).
2. Navigate to `firmware/` and open the `.ino` sketch that matches your hardware platform.
3. Verify library dependencies (e.g., `ArduinoJson` for configuration APIs) are installed in the IDE Library Manager, select your board, and flash.

### Direct Flash (RP2040)
Pre-compiled `.uf2` binaries are distributed under the **Releases** tab.
1. Connect your Pico W / RP2040 board while holding down the `BOOTSEL` button.
2. Drag and drop the downloaded `.uf2` file directly onto the mounted volume.

---

## 🛠️ DevOps & Verification

### Unit Testing (Catch2)
Native unit tests verify the mathematical execution of matrix inversions, Kalman state updates, and HMM state transitions.
To execute tests locally:
```bash
cd tests
mkdir build && cd build
cmake ..
make
./kalman_tests
```

### Anomaly Fuzzing & Simulation
Python toolsets are included to test stability and integration without hardware connected:
- **`tools/radar_simulator.py`**: Simulates the standard radar output stream to verify parser states.
- **`tools/serial_fuzzer.py`**: Bombards the UART parsing layer with corrupted sequences to verify validation robustness.
