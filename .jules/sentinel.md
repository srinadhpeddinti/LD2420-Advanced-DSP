## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-10-06 - Replacing innerHTML with safe DOM manipulation
**Vulnerability:** Use of innerHTML in `web_dashboard/LD2420_Dashboard.html` for rendering zone data dynamically. While this instance interpolated numerical data, it sets a dangerous precedent and invites DOM-based XSS if the structure is later refactored to include unsanitized user inputs.
**Learning:** Even if data is controlled, using `innerHTML` with template literals poses a latent risk and triggers static analysis warnings.
**Prevention:** Consistently use safe DOM creation methods like `document.createElement()`, setting properties, and `textContent` rather than interpolating strings into `innerHTML`.
