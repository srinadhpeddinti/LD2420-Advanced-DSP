## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Safe DOM manipulation to avoid XSS in frontend dashboards
**Vulnerability:** DOM-based XSS potential in static HTML frontend file (`web_dashboard/LD2420_Dashboard.html`) due to usage of `innerHTML` with dynamic template strings.
**Learning:** Even if data is controlled locally or comes from a trusted serial connection, using `innerHTML` to construct DOM nodes creates unnecessary security risks and fails standard SAST checks. Using programmatic DOM creation methods is safer.
**Prevention:** Avoid using `innerHTML` for dynamic UI generation. Prefer safe DOM API methods like `document.createElement`, `.className`, `.id`, `.textContent`, and `.appendChild` to construct UI elements programmatically.
