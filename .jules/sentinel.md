## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Overly Permissive CORS and DNS Rebinding Risk
**Vulnerability:** A wildcard `Access-Control-Allow-Origin: *` allowed unauthorized domains to interact with state-modifying radar API endpoints, and a naive `origin.endsWith(host)` check was susceptible to DNS Rebinding attacks.
**Learning:** Embedded web servers often copy-paste generic wildcard CORS headers for ease of development, which is catastrophic when combined with local network access. Additionally, validating `Origin` against `Host` is insufficient if the device blindly trusts the `Host` header, as attackers can point their own DNS to the device IP.
**Prevention:** Implement strict CORS validation that parses and validates private IP ranges correctly, requires the `X-Requested-With` header for CSRF protection on POSTs, and restricts DNS rebinding by ensuring the `Host` header matches the device actual local IP (`WiFi.localIP()`) or `.local`.
