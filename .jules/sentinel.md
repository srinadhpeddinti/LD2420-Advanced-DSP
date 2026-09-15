## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2023-10-27 - [Strict CORS and CSRF Protection]
**Vulnerability:** Overly permissive CORS (`Access-Control-Allow-Origin: *`) allowed any origin to access the device's API, and missing CSRF tokens/headers allowed state-changing requests to be forged.
**Learning:** Hardcoded wildcard CORS is a severe vulnerability in IoT devices, especially on local networks. String-based IP checking can be bypassed; using `IPAddress` class ensures reliable validation of private network spaces.
**Prevention:** Always implement dynamic `Origin` validation using strong IP parsing, enforce `X-Requested-With` header on state-changing API endpoints, and ensure proper `OPTIONS` preflight handling.
