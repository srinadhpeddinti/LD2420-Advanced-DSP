## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-05-24 - CSRF and CORS Validation Fix
**Vulnerability:** ESP32/ESP8266 web servers had overly permissive CORS (`Access-Control-Allow-Origin: *`) and lacked CSRF protection on state-changing API endpoints, making them vulnerable to cross-site request forgery and data exfiltration from the local network context.
**Learning:** Returning wildcard CORS headers and omitting CSRF tokens/headers on local IoT devices is highly dangerous, especially when combined with a lack of authentication. The combination allows malicious websites to execute actions on the device.
**Prevention:** To mitigate CSRF on ESP8266/ESP32 web servers: require `X-Requested-With: XMLHttpRequest` on POST endpoints. Dynamically validate `Origin` by strictly checking for private IPs (e.g., starts with `192.168.`, `10.`, `172.`, `127.`) and local hostnames (`localhost`, `ld2420.local`). Crucially, omit the `Access-Control-Allow-Origin` header entirely on validation failure—never fallback to `*`—to properly block CORS preflight requests. Also ensure to call `httpServer.collectHeaders()` in `setup()`.
