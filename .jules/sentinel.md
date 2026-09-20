## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - Omitting CORS Headers vs Denying Explicitly
**Vulnerability:** Sending `Access-Control-Allow-Origin: null` as a fallback allows origin bypass via sandboxed iframes (which generate opaque `null` origins) to pass preflight checks and perform CSRF on internal systems.
**Learning:** `null` is a valid origin representation for privacy/security contexts (data URIs, sandboxed iframes). A server should never explicitly authorize `"null"` unless intentionally supporting these contexts.
**Prevention:** To reject a cross-origin request securely, completely omit the `Access-Control-Allow-Origin` header rather than providing a fallback value.
