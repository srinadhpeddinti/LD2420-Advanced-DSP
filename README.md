# LD2420 Advanced DSP

A hardware-agnostic Digital Signal Processing (DSP) and probabilistic tracking firmware for the HLK-LD2420 24GHz mmWave radar module.

This implementation replaces basic sensor thresholding with continuous probabilistic models. It utilizes an Adaptive Kalman Filter (AKF) for kinematic state estimation, a Hidden Markov Model (HMM) for activity classification, and Dempster-Shafer theory for sensor fusion.

## Core Capabilities
- **Kinematic Tracking:** Singer-model Adaptive Kalman Filter (AKF) for precise range, velocity, and acceleration estimation.
- **Activity Classification:** 6-state Hidden Markov Model (HMM) decoded via the Viterbi algorithm (Sitting, Standing, Walking, etc.).
- **Bio-Vital Extraction:** Sub-millimeter chest displacement measurement (breathing rate) via the Goertzel algorithm.
- **Gait Analysis:** Stride cadence detection via MUSIC pseudo-spectrum.
- **Evidence Fusion:** Multi-modal sensor data fused using Dempster-Shafer theory.

## Supported Architectures

The repository includes firmware profiles tailored for specific hardware platforms. Select the `.ino` file that matches your microcontroller:

1. **`LD2420_Pico_Ultimate.ino`** (Raspberry Pi Pico / RP2040)
   - Interface: USB CDC Serial
   - UART: HardwareSerial on `Serial1`
2. **`LD2420_ESP32_WiFi.ino`** (ESP32 / ESP32-S3 / ESP32-P4)
   - Interface: WebSocket via WiFi
   - UART: HardwareSerial
3. **`LD2420_ESP8266_WiFi.ino`** (NodeMCU / Wemos D1 Mini)
   - Interface: WebSocket via WiFi
   - UART: SoftwareSerial
4. **`LD2420_Teensy_USB.ino`** (Teensy 4.0 / 4.1)
   - Interface: USB CDC Serial
   - UART: HardwareSerial on `Serial1`
5. **`LD2420_STM32_Serial.ino`** (STM32 Cortex-M4/M7)
   - Interface: USB/Hardware Serial
   - UART: HardwareSerial

## Network Configuration (ESP32 / ESP8266)

The wireless firmware profiles utilize a captive portal for network provisioning and mDNS for local discovery.

1. **WiFi Provisioning:** On initial boot, or if the known network is unavailable, the device will broadcast an Access Point named **`LD2420_Setup`**. Connect to this AP to configure the local network credentials.
2. **mDNS Discovery:** Once connected to the local network, the device will broadcast its address via mDNS. The web dashboard and REST API are accessible at `http://ld2420.local`.

## Hardware Pinouts

### Setup A: Serial USB Boards (RP2040, Teensy, STM32)
| MCU Pin | LD2420 Pin | Function |
|---|---|---|
| **5V / VBUS** | **VCC** | 5V Power Supply |
| **GND** | **GND** | Common Ground |
| **TX Pin (UART)** | **RX** | Transmit command data |
| **RX Pin (UART)** | **TX** | Receive telemetry data |
| **Hardware Int** | **OT2** | Hardware Doppler interrupt |

### Setup B: ESP32 / ESP8266 (WiFi)
| ESP Pin | LD2420 Pin | Function |
|---|---|---|
| **5V / VIN** | **VCC** | 5V Power Supply |
| **GND** | **GND** | Common Ground |
| **MCU TX** | **RX** | Transmit command data |
| **MCU RX** | **TX** | Receive telemetry data |
| **GPIO Int**| **OT2** | Hardware Doppler interrupt |

*Note: Verify the exact UART pin mappings defined at the top of the respective `.ino` file.*

## Dashboard Interface

The included `LD2420_Dashboard.html` is a standalone static web application for telemetry visualization. 

1. Open `LD2420_Dashboard.html` in a supported browser (Chrome or Edge).
2. Click the connection settings icon.
3. **For USB Serial Devices:** Select **"Connect USB Serial"** and choose the corresponding COM port. (Requires Web Serial API support).
4. **For WiFi Devices:** Enter `ld2420.local` or the assigned IP address to establish a WebSocket connection.

## DSP Engine Configuration

The core logic parameters are located within `LD2420_AKF_HMM_NoLimits.hpp`:
- `A[HMM_STATES][HMM_STATES]`: The Markov transition probability matrix.
- `sigma_m` and `tau_m`: Singer model maneuver variance and correlation time constants.
- `EMU` and `ESIG`: Gaussian emission model parameters for velocity bounds per state.
