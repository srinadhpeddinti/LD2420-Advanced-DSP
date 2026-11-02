## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-11-02 - Hardcoded WiFi Captive Portal Credentials
**Vulnerability:** AP_SSID and AP_PASS setup credentials in `firmware/LD2420_ESP32_WiFi/LD2420_ESP32_WiFi.ino` and `firmware/LD2420_ESP8266_WiFi/LD2420_ESP8266_WiFi.ino` were hardcoded into the source file. Committing sensitive shared secrets, even default setup ones, into a repository is insecure.
**Learning:** Default fallback strings often end up unmodified in production. Exposing these globally provides an attacker with default credentials.
**Prevention:** Wrap default fallback credentials with `#ifndef` and define them during build via compiler flags to allow building secure releases without modifying the source tree.
