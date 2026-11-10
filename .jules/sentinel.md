## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - DOM-based XSS via innerHTML
**Vulnerability:** DOM-based Cross-Site Scripting (XSS) vulnerability due to constructing HTML string with `innerHTML` inside `web_dashboard/LD2420_Dashboard.html` (specifically `row.innerHTML`).
**Learning:** Even if data seems safe or derived from loop counters, `innerHTML` with template literals can be flagged by security tools or exploited if future code modifies the data flow.
**Prevention:** Always use safe DOM manipulation methods like `document.createElement`, `className`, and `textContent` instead of `innerHTML` when building UI elements.
