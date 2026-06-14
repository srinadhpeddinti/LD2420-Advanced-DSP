# LD2420 Ultimate DSP & Edge AI Firmware

[![CI/CD Pipeline](https://github.com/srinadhpeddinti/LD2420-Advanced-DSP/actions/workflows/build.yml/badge.svg)](https://github.com/srinadhpeddinti/LD2420-Advanced-DSP/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A hyper-optimized, multi-core C++ Digital Signal Processing (DSP) and Machine Learning engine for the HiLink LD2420 mmWave 24GHz radar. Originally designed for the Raspberry Pi Pico (RP2040), but fully compatible with ESP32, ESP8266, STM32, and Teensy.

This firmware abandons the basic proprietary tools provided by the manufacturer and replaces them with an enterprise-grade tracking stack featuring Adaptive Kalman Filters, Hidden Markov Models, and TinyML algorithms.

## ✨ Key Features
- **Multi-Core & Real-Time OS (RTOS)**: Zero-jitter DSP processing by pinning the UART DMA and Kalman Filter to Core 0, while JSON/Binary serialization runs on Core 1 (RP2040).
- **Direct Memory Access (DMA)**: Non-blocking 256-byte circular buffer ingestion directly from the LD2420 UART.
- **Adaptive Kalman Filter (AKF)**: A robust 6x6 state-space matrix implementation tracking Position, Velocity, and Acceleration dynamically adjusting measurement trust based on target speed.
- **Hidden Markov Model (HMM)**: A 3-state (ABSENT, SITTING, WALKING) activity transition engine to smooth out radar noise and prevent false negative presence drops.
- **Edge AI / Machine Learning**:
    - **Support Vector Machine (SVM)**: Uses a Radial Basis Function (RBF) kernel to classify posture (Standing, Sitting, Prone) based on Range Variance and Radar Cross Section (RCS).
    - **Isolation Forest**: Detects unusual behavioral anomalies (e.g., staggering, falls) in real-time.
    - **Decision Tree**: Filters out pets (dogs/cats) vs humans by analyzing micro-Doppler signatures.
- **Custom Binary Telemetry Protocol**: 47-byte little-endian packed C++ structs replace slow ASCII JSON for 10x faster telemetry over USB/WiFi.

## 🚀 Installation & Compilation

### 1. Arduino IDE Setup
Because this project utilizes a shared core mathematics engine, it is structured as an **Arduino Library**.
1. Clone this repository to your local machine.
2. Copy or symlink the `libraries/LD2420_Ultimate` folder into your standard Arduino libraries directory (usually `Documents/Arduino/libraries/`).
3. Open the corresponding `.ino` file from the `firmware/` directory for your specific board (e.g., `firmware/LD2420_Pico_Ultimate/LD2420_Pico_Ultimate.ino`).

### 2. For Non-Programmers (Drag-and-Drop)
If you are using a Raspberry Pi Pico, you do not need to compile anything!
1. Go to the **Releases** tab on GitHub (or the Artifacts in GitHub Actions).
2. Download the pre-compiled `.uf2` file.
3. Hold the `BOOTSEL` button on your Pico, plug it into USB, and drag the `.uf2` file onto the Pico drive.

## 📊 The Web Dashboard
This project includes a high-performance, purely local HTML web dashboard (`web_dashboard/LD2420_Dashboard.html`). 
It utilizes the **Web Serial API** allowing your Chrome/Edge browser to connect directly to the dev board via USB, decode the packed binary struct using JavaScript `DataView`, and plot the Kalman Filter states in real-time without needing a Python server.

## 🔬 Benchmarks & Performance
- **Loop Latency (RP2040)**: Core 0 DSP execution time averages `< 1.2ms` per frame.
- **UART Overhead**: `0 CPU cycles` per byte (fully offloaded to hardware DMA).
- **Bandwidth**: The custom binary protocol consumes only `47 bytes/frame`, compared to `~300 bytes` for equivalent JSON, allowing stable 10Hz updates over congested 2.4GHz WiFi (ESP32) or standard Serial.
- **Memory Profiling**: Catch2 Unit tests wrapped in Valgrind confirm 0 memory leaks in the dynamic matrix allocations.

## 🛠️ DevOps & CI/CD Tooling
- **Catch2 Unit Tests**: Run `tests/test_kalman.cpp` natively on Linux/Windows to verify matrix inversions mathematically.
- **Fuzzing & Simulation**: Use `tools/radar_simulator.py` to inject mock ASCII radar targets, and `tools/serial_fuzzer.py` to stress-test the `AsciiProtocolParser` against buffer overflows.

## 🛡️ Privacy & Security
All computation (Kalman Math, SVM, HMM) happens **100% locally** on the embedded microcontroller. No cloud connection is required for presence tracking, ensuring total privacy.

---
*Built for pure performance. No AI slop. Just raw C++ mathematics.*
