## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - Use of innerHTML with seemingly safe interpolations
**Vulnerability:** DOM-based XSS (Cross-Site Scripting) vulnerability due to the use of `innerHTML` in `web_dashboard/LD2420_Dashboard.html`.
**Learning:** Using `innerHTML` for DOM manipulation is inherently dangerous, even if the interpolated values appear safe (like loop indices). It sets a bad precedent, fails static analysis checks, and makes the code brittle to future changes where unsanitized user input might be introduced.
**Prevention:** Avoid `innerHTML` entirely for dynamically constructing DOM nodes. Always use secure alternatives like `document.createElement`, `textContent`, and `appendChild`.
