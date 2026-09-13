## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2025-02-28 - Secure CORS and CSRF Protection
**Vulnerability:** The firmware's web server allowed Cross-Origin Resource Sharing (CORS) from any origin using `Access-Control-Allow-Origin: *` on all endpoints and lacked Cross-Site Request Forgery (CSRF) protection for state-changing POST requests.
**Learning:** Returning wildcard CORS headers alongside non-idempotent endpoints on local IoT devices can enable malicious websites to hijack user browsers on the local network to reconfigure or control devices silently.
**Prevention:** Always restrict `Access-Control-Allow-Origin` dynamically by validating the `Origin` header against internal IP subnets and localhost. For POST endpoints, require custom headers like `X-Requested-With: XMLHttpRequest` to act as a CSRF token. Remember to register these headers in the firmware using `collectHeaders`.
