## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Hardcoded secrets in setup configuration
**Vulnerability:** Hardcoded setup credentials (AP_PASS) in the firmware source code which may be committed to version control.
**Learning:** In microcontroller firmware (ESP8266/ESP32), it is common to hardcode default Access Point credentials in the global variables. This is a bad practice as it leaks secrets if the source code is made public.
**Prevention:** Wrap default AP setup credentials in `#ifndef` preprocessor guards. This allows overriding them via compiler flags (e.g., `-DAP_PASS="..."`) during the build process without committing sensitive shared secrets into the repository.
