## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-11-04 - DOM-based XSS vulnerability with `innerHTML`
**Vulnerability:** Use of `innerHTML` for constructing DOM elements dynamically in `LD2420_Dashboard.html` could lead to DOM-based Cross-Site Scripting (XSS).
**Learning:** Even when interpolating seemingly safe variables, using `innerHTML` opens the door to potential injection attacks if input is ever controlled or influenced by untrusted sources (e.g. manipulated zone names or indices in a real-world scenario).
**Prevention:** Avoid `innerHTML` entirely when creating DOM elements dynamically. Instead, use safer methods such as `document.createElement()` along with `.textContent` to safely assign dynamic data without interpreting it as HTML.
