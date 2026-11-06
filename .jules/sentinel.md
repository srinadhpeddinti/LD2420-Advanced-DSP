## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Hardcoded Wi-Fi Credentials
**Vulnerability:** Default Wi-Fi credentials (`AP_SSID` and `AP_PASS`) were hardcoded as `const char*` directly in the source files, making it difficult to securely override them during builds.
**Learning:** Hardcoded secrets in C++ source files for embedded devices can leak sensitive default configurations. Using preprocessor macros allows for secure build-time injection.
**Prevention:** Always wrap default configuration variables like passwords in `#ifndef` blocks to allow them to be overridden by compiler flags (e.g., `-DAP_PASS="secret"`).
