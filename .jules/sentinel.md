## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-15 - Hardcoded Wi-Fi Setup Credentials
**Vulnerability:** Default Wi-Fi AP setup passwords (AP_PASS) and SSIDs were hardcoded directly in the ESP32 and ESP8266 sketch files.
**Learning:** Hardcoding default Wi-Fi passwords in version-controlled firmware source code exposes shared secrets to anyone with repository access and makes it harder for downstream users to securely override them during the build process without modifying tracked files.
**Prevention:** Wrap default credential variables (like AP_SSID and AP_PASS) in `#ifndef` preprocessor guards. This allows build systems to inject secrets via compiler flags (e.g., `-DAP_PASS="secret"`) safely.
