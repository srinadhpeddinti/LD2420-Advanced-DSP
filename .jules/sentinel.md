## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - Unauthenticated Cross-Origin Access and CSRF via GETs
**Vulnerability:** ESP web servers allowed any origin to hit state-modifying endpoints, opening up CSRF, and used an overly permissive `Access-Control-Allow-Origin: *` which allowed unintended domain access.
**Learning:** ESP32/ESP8266 `WebServer` implementations do not strictly enforce method verb restrictions unless coded meticulously. Further, string matching for origins can be bypassed (e.g. `10.attacker.com`).
**Prevention:** Implement strict origin validation using `IPAddress::fromString()` instead of basic string checks, mandate `X-Requested-With: XMLHttpRequest` for all POSTs to mitigate CSRF, and remove `Access-Control-Allow-Origin: *` across the board, explicitly mirroring allowed origins.
