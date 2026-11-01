## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2025-02-18 - Hardcoded WiFi Setup Credentials
**Vulnerability:** AP_SSID and AP_PASS constants for the Captive Portal were hardcoded as strings in the ESP8266 and ESP32 firmwares. This forces users to compile secrets into their firmware and can lead to accidental commits of sensitive Wi-Fi setup passwords to source control.
**Learning:** In open-source embedded firmware projects, default secrets often get left as literal strings. Users modifying the code for their environment might overwrite these and accidentally leak their network configuration.
**Prevention:** Wrap default credential strings in `#ifndef` blocks. This allows developers to securely pass in passwords at compile-time (e.g., using `-DAP_PASS="MySecurePassword"` in CI pipelines or `platformio.ini`) without touching the tracked source code.
