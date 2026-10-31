## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - Hardcoded Setup Credentials in Firmware
**Vulnerability:** Default WiFi AP credentials (AP_SSID and AP_PASS) are hardcoded in the source code, making them available to anyone with read access to the repository.
**Learning:** Shared secrets should never be checked into version control. While they act as placeholders, developers often fail to change them before deployment.
**Prevention:** Wrap default credential definitions in `#ifndef` preprocessor guards. This allows passing custom credentials at build time via compiler flags (e.g., `-DAP_PASS="mysecret"`), avoiding the need to commit them in the source.
