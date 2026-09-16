## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2025-02-23 - CSRF Mitigation and Secure CORS Origin Validation
**Vulnerability:** The firmware web server endpoints lacked CSRF protection and blindly echoed `Access-Control-Allow-Origin: *`. Furthermore, simple string matching on the `Origin` header was bypassed.
**Learning:** For embedded systems without full URL parsing libraries, validating CORS origins based on IP rules requires explicitly converting the hostname string to an `IPAddress` object. This prevents attacks where adversaries host pages on bypassing hostnames like `192.168.attacker.com` since `.fromString()` correctly parses out the IP bytes which can be rigorously verified against local network ranges (e.g. 10.x, 172.16-31.x, 192.168.x).
**Prevention:** Always require `X-Requested-With: XMLHttpRequest` for state-changing HTTP methods. Dynamically reflect the `Origin` only if it matches `localhost`, `.local`, or a strict private IP structure parsed explicitly with the `IPAddress` class, stripping trailing ports and protocol headers properly.
