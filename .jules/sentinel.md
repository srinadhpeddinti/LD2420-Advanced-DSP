## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-24 - Missing Rate Limiting on State-Modifying Endpoints
**Vulnerability:** The unauthenticated `/api/cmd` and `/api/thresholds` endpoints allowed unlimited concurrent requests. An attacker could rapidly spam these endpoints, specifically the `reboot` command, causing a Denial of Service (DoS) and potentially crashing the device due to resource exhaustion.
**Learning:** Even internal or setup APIs require fundamental defense-in-depth measures. Unrestricted unauthenticated APIs are trivial to abuse for DoS, especially on resource-constrained microcontrollers like ESP8266/ESP32.
**Prevention:** Always implement basic rate limiting (e.g., checking `millis()`) on sensitive, state-modifying endpoints to protect device stability.
