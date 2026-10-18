## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Replacing innerHTML with programmatic DOM creation
**Vulnerability:** Use of innerHTML for constructing DOM elements, even with seemingly safe interpolations, can lead to DOM-based XSS vulnerabilities if the inputs are ever derived from user-controlled data.
**Learning:** It's a common anti-pattern to use innerHTML for convenience. Removing it in favor of programmatic DOM creation is a classic "defense in depth" security enhancement.
**Prevention:** Always use safe DOM manipulation methods like document.createElement, textContent, and appendChild instead of innerHTML.
