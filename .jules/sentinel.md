## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2025-02-18 - XSS Risk in web dashboard via innerHTML
**Vulnerability:** Use of innerHTML in `renderZoneMap` without sanitization creates a potential DOM-based Cross-Site Scripting (XSS) vulnerability, even if the current interpolated variables seem safe (e.g., integers).
**Learning:** Even static HTML dashboards reading data from embedded hardware are susceptible to XSS if the data payloads are ever maliciously modified or the code is later refactored to include user inputs.
**Prevention:** Always use safe DOM manipulation alternatives like `document.createElement()`, `classList`, and `textContent` instead of `innerHTML` when dynamically building UI elements.
