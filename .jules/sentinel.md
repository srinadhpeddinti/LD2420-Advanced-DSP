## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-24 - Hardcoded AP Password Vulnerability
**Vulnerability:** ESP32 and ESP8266 firmwares used a globally known hardcoded string ("YOUR_SETUP_PASSWORD") for the WiFi Manager AP setup password.
**Learning:** Hardcoding credentials in IoT/Edge device firmware allows trivial unauthorized network access if the device is booted in an unprovisioned or reset state. The attacker could intercept or inject configurations.
**Prevention:** Use a hardware-derived dynamic secret (e.g., last 6 chars of MAC address) to generate unique AP passwords per device. This makes mass exploitation impossible and requires physical proximity to learn the specific device's MAC.
