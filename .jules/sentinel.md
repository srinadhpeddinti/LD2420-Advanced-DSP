## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - innerHTML usage in dashboard UI
**Vulnerability:** Use of innerHTML for dynamically rendering DOM elements.
**Learning:** Even with seemingly safe strings, innerHTML usage is a recognized anti-pattern that can lead to DOM-based XSS if the data inputs are ever modified to include user-controlled content.
**Prevention:** Avoid innerHTML. Use document.createElement and textContent when building UI elements dynamically.
