## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - DOM-based XSS Risk via innerHTML
**Vulnerability:** Use of `innerHTML` in `renderZoneMap` function within the static dashboard (`web_dashboard/LD2420_Dashboard.html`).
**Learning:** Even if the inputs are currently safe, using `innerHTML` can easily lead to DOM-based XSS vulnerabilities if input validation changes.
**Prevention:** Avoid `innerHTML`. Use safer alternatives like `document.createElement`, `textContent`, and `appendChild` when generating dynamic content in the frontend.
