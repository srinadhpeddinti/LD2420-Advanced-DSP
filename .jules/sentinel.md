## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - Missing input bounds validation on API handlers
**Vulnerability:** API endpoints handling sensor threshold adjustments lacked bounds validation, allowing potentially negative or excessively large input integers to cause undefined behavior in subsequent calculations or out-of-bounds access.
**Learning:** When retrieving numbers from query parameters or payloads (e.g. `httpServer.arg("foo").toInt()`), they are unconstrained and must always be checked against acceptable application bounds before assigning to configuration variables.
**Prevention:** Explicitly validate parsed integers against known bounds (e.g., `val >= 50 && val <= 800`) before applying state changes.
