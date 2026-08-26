## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-18 - State-modifying API endpoints allowing cross-origin requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (`/api/cmd` and `/api/thresholds`) not requiring a custom header. A malicious site could use `<form method="POST">` to change device state.
**Learning:** In simple ESP-based web servers, simply restricting methods to `HTTP_POST` is not enough to prevent CSRF, because simple POST requests do not trigger CORS preflight. Adding `Access-Control-Allow-Origin: *` makes this even riskier if the device is accessible on a local network or exposed.
**Prevention:** Always require a custom header (like `X-Requested-With: XMLHttpRequest`) for state-modifying requests and use `httpServer.collectHeaders` to read it. Simple cross-origin requests cannot set custom headers without triggering a preflight `OPTIONS` request, which effectively blocks the CSRF attack.
