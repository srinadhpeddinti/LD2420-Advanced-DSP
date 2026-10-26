## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-10-26 - Insecure CORS Origin wildcard allowed for state-modifying endpoints
**Vulnerability:** CSRF and unauthorized cross-origin requests allowed due to `Access-Control-Allow-Origin: *` unconditionally returned on state-modifying POST requests.
**Learning:** Default wildcard CORS headers on ESP8266/ESP32 webservers bypass browser protections and allow malicious domains to interact with sensitive APIs. Proper header validation requires checking both Host and Origin headers carefully.
**Prevention:** Implement strict dynamic CORS validation. Verify `X-Requested-With`, match `Origin` to the local device IP or `.local` hostname, and only return a wildcard for non-sensitive GET requests. Always return a 403 Forbidden on failed validation instead of silently processing the request.
