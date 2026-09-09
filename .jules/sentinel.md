## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Fix Overly Permissive CORS and CSRF
**Vulnerability:** ESP32/ESP8266 HTTP server endpoints changing system states (like reboot, thresholds) were accessible to any Origin with `Access-Control-Allow-Origin: *` and missing CSRF tokens for POST requests.
**Learning:** Embedded web servers often set `Access-Control-Allow-Origin: *` blindly for convenience, allowing malicious websites on the same network to interact blindly.
**Prevention:** Implement strict Origin validation mapping to local IPs / expected hostnames and verify CSRF protection like `X-Requested-With: XMLHttpRequest` headers on state-changing API endpoints. Ensure Frontend UI is updated to pass the required headers for API requests.
