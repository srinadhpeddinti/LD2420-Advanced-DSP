## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-24 - Cross-Site Request Forgery (CSRF) via Permissive CORS
**Vulnerability:** The API allowed unauthenticated POST requests from any origin (`Access-Control-Allow-Origin: *`) to state-changing endpoints without validating the source.
**Learning:** Returning wildcard CORS headers and relying on `httpServer.hasArg()` allows malicious sites to craft hidden forms that execute state-modifying requests against local devices.
**Prevention:** Dynamically validate the `Origin` header against internal/private IP blocks and enforce `X-Requested-With: XMLHttpRequest` on all POST requests to ensure they originated from a controlled JS environment and are subject to CORS preflights.

## 2024-05-24 - arduino/compile-sketches GitHub Action Library Configuration
**Learning:** The `arduino/compile-sketches` action does not properly resolve local library paths using `cli-compile-flags: - --library ...`. This causes fatal "No such file or directory" errors when the sketch attempts to include the local library header.
**Action:** Always use the `libraries` array with `- source-path: <path-to-library>` instead of passing the library via CLI flags when configuring the `arduino/compile-sketches` GitHub Action.
