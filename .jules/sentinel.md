## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-24 - [DOM-based XSS Prevention in Dashboard]
**Vulnerability:** Found `innerHTML` being used with template literals to construct HTML elements dynamically based on integer inputs (zones). Even though the input might currently be safe, this is a known anti-pattern for DOM manipulation which could lead to DOM-based XSS if the data source becomes tainted.
**Learning:** In a static HTML frontend that serves as a dashboard for an embedded device, relying on `innerHTML` for dynamic content generation is risky. It's crucial to use safe DOM APIs consistently.
**Prevention:** Avoid `innerHTML` entirely for dynamically generated content. Always use `document.createElement()`, `textContent`, and `appendChild()` to ensure data is treated strictly as text/nodes and never executed as markup.
