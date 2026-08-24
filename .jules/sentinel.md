## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2025-02-28 - CSRF Vulnerability on State-Changing API Endpoints in ESP Firmware
**Vulnerability:** Wildcard CORS (`Access-Control-Allow-Origin: *`) combined with no CSRF token/header check on `POST` endpoints (`/api/cmd`, `/api/thresholds`) in the ESP32 and ESP8266 firmwares permitted CSRF via simple requests (like forms).
**Learning:** Removing wildcard CORS isn't enough to stop CSRF from simple requests, and completely removing `OPTIONS` handlers breaks legitimate cross-origin preflight requests (functional regression). A reliable fix requires forcing a preflight for cross-origin requests by expecting a custom header like `X-Requested-With`, combined with parsing `collectHeaders` in ESP HTTP server.
**Prevention:** For endpoints that modify state (`POST`), implement CSRF protection by requiring a custom header (e.g., `X-Requested-With`), registering it with `httpServer.collectHeaders()`, and allowing that header in the `OPTIONS` CORS preflight response.
