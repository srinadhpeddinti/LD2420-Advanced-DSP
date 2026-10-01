## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Socket exhaustion and CSRF bypass in CORS filters
**Vulnerability:** Socket exhaustion (DoS) due to returning without sending an HTTP response in failure conditions, and CSRF bypass allowing `Origin: null` on POST requests.
**Learning:** Returning early without calling `httpServer.send()` on ESP8266/ESP32 webservers leaves the client connection open, quickly exhausting the limited available sockets. Furthermore, allowing `null` origins globally enables CSRF attacks via sandboxed iframes.
**Prevention:** Always respond with an HTTP status (e.g., `httpServer.send(403)`) on CORS/auth failures. Restrict `null` origin allowances to non-mutating `GET` requests only.
