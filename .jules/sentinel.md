## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-20 - Over-permissive CORS and CSRF on POST endpoints
**Vulnerability:** CSRF vulnerability and overly permissive CORS due to hardcoded `Access-Control-Allow-Origin: *` and missing `X-Requested-With` header validation on state-modifying POST endpoints (`/api/cmd` and `/api/thresholds`).
**Learning:** Even if HTTP methods are restricted to POST, simple POST requests without custom headers can still be triggered cross-origin via HTML forms. Furthermore, wildcard CORS exposes API data unconditionally to any origin.
**Prevention:** Dynamically validate the `Origin` header against a whitelist or local/private IP rules to set `Access-Control-Allow-Origin`. Enforce `X-Requested-With` on all state-modifying POST endpoints, and configure OPTIONS preflight handlers to require `Access-Control-Allow-Headers: X-Requested-With` to properly prevent unauthenticated cross-origin requests.
