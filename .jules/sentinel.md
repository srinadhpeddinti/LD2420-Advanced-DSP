## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-24 - CORS Preflight Options and Custom Headers
**Vulnerability:** OPTIONS preflight request blocking legitimate POST requests.
**Learning:** Browsers do not send custom headers like X-Requested-With on preflight OPTIONS requests, only Access-Control-Request-Headers. If the server incorrectly mandates X-Requested-With in the OPTIONS handler, the preflight is rejected with 403, and the browser blocks the POST.
**Prevention:** In custom CORS validation functions, provide a boolean flag (e.g. isOptions) to bypass custom header enforcement during preflights while continuing to enforce it on state-modifying requests.

## 2024-05-24 - Over-restrictive CORS causing CRLF Injection Risk and Functional Breakage
**Vulnerability:** Complex, manual string parsing of Origin headers can lead to CRLF injection if reflected directly, and breaks legitimate use cases like local file:// execution (null Origin).
**Learning:** When mitigating CSRF on simple IoT APIs, relying solely on a custom header (e.g., X-Requested-With) is often sufficient and much safer than attempting complex, manual Origin whitelisting in C++. Reflecting unvalidated, user-controlled headers like Origin creates an HTTP Response Splitting risk.
**Prevention:** Rely on custom headers for CSRF protection on POSTs. If CORS is needed, either use a strict, exact-match whitelist, or simply use Access-Control-Allow-Origin: * combined with the custom header requirement for state-changing requests.
