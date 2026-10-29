## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-30 - Fix hardcoded AP credentials in firmware
**Vulnerability:** AP_SSID and AP_PASS were hardcoded in the ESP firmware code, exposing sensitive default credentials to the source code repository.
**Learning:** Hardcoded credentials should be wrapped in preprocessor guards to allow for build-time overrides securely, thus preventing them from being stored directly in the repository source files.
**Prevention:** Use `#ifndef` preprocessor guards around default AP setup credentials in C/C++ firmware code to allow build-time overrides via compiler flags.
