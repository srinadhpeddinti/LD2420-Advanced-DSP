## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-19 - [DOM-based XSS via innerHTML]
**Vulnerability:** Use of `innerHTML` in `renderZoneMap` within `web_dashboard/LD2420_Dashboard.html` allows potential DOM-based XSS if attacker-controlled input reaches the string interpolation.
**Learning:** Even if data seems safe, using `innerHTML` with dynamically constructed strings in frontend code creates a risk of XSS execution.
**Prevention:** Always use safe DOM APIs like `document.createElement`, `textContent`, and `appendChild` instead of `innerHTML` for dynamic content insertion.
