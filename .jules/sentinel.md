## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - CSRF and CORS misconfiguration on ESP8266/ESP32 WebServers
**Vulnerability:** CSRF vulnerability and bypassable CORS on ESP WebServer. The previous implementation returned `Access-Control-Allow-Origin: *` unconditionally, and state-modifying POST requests did not validate the Origin or require custom headers.
**Learning:** Returning `Access-Control-Allow-Origin: *` on validation failure allows CORS preflight requests to succeed, entirely defeating custom header checks (like `X-Requested-With`) used for CSRF protection. In addition, Origin checking must be robust against IP-based domains and never blindly trust `null` (due to sandboxed iframe attacks).
**Prevention:** Always omit the `Access-Control-Allow-Origin` header when validation fails. Ensure Origin checking correctly whitelists private/local IPs only. Require a custom header and validate origins strictly for state-modifying actions.
