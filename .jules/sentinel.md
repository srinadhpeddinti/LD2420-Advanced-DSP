## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Overly permissive CORS and Missing CSRF Protection
**Vulnerability:** The API endpoints sent `Access-Control-Allow-Origin: *` blindly and did not require any specific headers like `X-Requested-With` for state-modifying POST requests, allowing trivial CSRF and unrestricted cross-origin access.
**Learning:** Returning `*` for CORS on private devices exposes them to malicious sites accessed from the local network. Using string functions like `startsWith()` for IP checks can be bypassed (e.g. `192.168.attacker.com`).
**Prevention:** Dynamically validate the `Origin` header using `IPAddress` parsing for local/private IPs. Require `X-Requested-With: XMLHttpRequest` on POST requests to mitigate CSRF, and configure CORS preflight appropriately.
