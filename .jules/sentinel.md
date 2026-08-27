## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-27 - [High] Local Web Server CSRF Vulnerability
**Vulnerability:** ESP32 and ESP8266 REST API endpoints (`/api/cmd` and `/api/thresholds`) lacked CSRF protection while handling state-modifying POST requests with `Access-Control-Allow-Origin: *`.
**Learning:** Even local IoT devices with private IPs are vulnerable to CSRF if a victim visits an attacker's public website. The attacker's site can issue hidden simple POST requests (which bypass CORS preflight checks) to common local IPs or mDNS addresses to modify device state.
**Prevention:** Require a custom HTTP header (e.g., `X-Requested-With: XMLHttpRequest`) for all state-modifying operations. Simple cross-origin POST requests cannot set custom headers, effectively preventing the CSRF attack. Ensure `httpServer.collectHeaders()` is used in `setup()` to read the header in ESP WebServer.
