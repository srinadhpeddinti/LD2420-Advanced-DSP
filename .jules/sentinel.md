## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - [Secure Custom Header CSRF Protection]
**Vulnerability:** CSRF protection via custom header (`X-Requested-With`) combined with wildcard CORS (`Access-Control-Allow-Origin: *`) allows malicious websites to bypass protection using `fetch()`.
**Learning:** Custom header CSRF protection relies on the browser's CORS preflight blocking the custom header. If the preflight explicitly allows the header for all origins, the protection is nullified. Also, origin validation using `.indexOf()` or `.startsWith()` without strict boundary checks (like digits-only for IPs) allows spoofing domains like `192.168.evil.com`.
**Prevention:** Dynamically validate the `Origin` header against a strict whitelist (e.g., parsing IPs for digits) and echo it back. Never use `Access-Control-Allow-Origin: *` with `Access-Control-Allow-Headers: X-Requested-With`.
