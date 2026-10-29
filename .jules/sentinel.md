## 2024-05-18 - Hardcoded Default AP Setup Credentials
**Vulnerability:** Hardcoded AP_SSID and AP_PASS constants in firmware sketches allow potential attackers to hijack or gain unauthorized access during Wi-Fi setup if left unchanged.
**Learning:** Microcontroller firmware sketches often hardcode default Wi-Fi AP setup credentials for initial captive portal provisioning.
**Prevention:** Wrap AP_SSID and AP_PASS definitions in `#ifndef` preprocessor guards in `.ino` sketches to allow overriding setup credentials at build time via compiler flags (`-DAP_PASS="..."`) without exposing sensitive or static shared secrets in source code repositories.

## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
