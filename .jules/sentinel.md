## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Fix CSRF via overly permissive CORS and missing token
**Vulnerability:** The API allowed Cross-Origin Request Forgery (CSRF) by setting Access-Control-Allow-Origin: * and not requiring an anti-CSRF token on state modifying POST endpoints.
**Learning:** ESP web servers can be easily vulnerable to CSRF when CORS is misconfigured to allow all origins on state modifying endpoints, and without checking an explicit token like X-Requested-With.
**Prevention:** For local network devices, use dynamic Origin validation (against Host header, localnames, and private IPs). Ensure state modifying endpoints require a custom header like X-Requested-With: XMLHttpRequest, and properly configure OPTIONS handling.
