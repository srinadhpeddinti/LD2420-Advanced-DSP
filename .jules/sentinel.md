## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-03-24 - [Overly Permissive CORS Headers]
**Vulnerability:** The firmware exposed the `/api/cmd` and `/api/thresholds` POST endpoints using an overly permissive wildcard CORS policy (`Access-Control-Allow-Origin: *`).
**Learning:** This wildcard policy allows any web page the user accesses on their local network to make unauthorized cross-origin CSRF requests against the device (e.g. rebooting or resetting device settings). In a local IoT context, this is a serious risk.
**Prevention:** Avoid wildcard CORS headers for state-changing endpoints in embedded web servers.
