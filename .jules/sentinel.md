## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2025-05-18 - DOM-based XSS via innerHTML
**Vulnerability:** DOM-based Cross-Site Scripting (XSS) vulnerability caused by dynamically constructing HTML elements using `innerHTML` with variable interpolation in `web_dashboard/LD2420_Dashboard.html`.
**Learning:** Using `innerHTML` for DOM updates is prone to XSS, even if the data seems safe (like numerical zone indices), as any external manipulation or future change to the variables could lead to script injection.
**Prevention:** Always use safe DOM manipulation methods such as `document.createElement()`, `element.className`, `element.id`, and `element.textContent` instead of `innerHTML`.
