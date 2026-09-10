## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - String-based IP validation bypass
**Vulnerability:** Weak Origin header validation for CORS/CSRF using `startsWith` and `indexOf` (e.g. `10.attacker.com` bypassing `10.`).
**Learning:** Checking for substrings or using basic index offsets to validate IPs is prone to bypasses since domains can simply be prefixed with the IP string.
**Prevention:** Always use robust IP parsing libraries (like `IPAddress.fromString()` in ESP/Arduino) to validate the Origin header strictly matches a safe private IP space.
