## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Overly Permissive CORS and Missing CSRF Protection
**Vulnerability:** The ESP web server used `Access-Control-Allow-Origin: *` on all API endpoints and lacked CSRF protection, allowing malicious websites to make cross-origin requests and mutate radar settings without user intent.
**Learning:** Returning a static wildcard CORS policy without restricting origins makes embedded IoT devices vulnerable to Drive-by-Pharming and CSRF, especially when state-modifying POST requests don't require custom headers that would trigger preflight checks.
**Prevention:** Implement strict dynamic origin validation (verifying local/private IP structures) and require a custom header like `X-Requested-With` on all POST endpoints. Ensure `httpServer.collectHeaders` is configured to intercept these headers during setup.
