## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2025-02-23 - DOM-based XSS via innerHTML
**Vulnerability:** Use of innerHTML for DOM manipulation.
**Learning:** Even with seemingly safe interpolations, using innerHTML can lead to DOM-based XSS vulnerabilities if user input is not properly sanitized.
**Prevention:** Avoid using innerHTML and instead use safe alternatives like document.createElement and textContent.
