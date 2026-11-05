## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2026-11-05 - Prevent hardcoded secrets by using macro fallbacks
**Vulnerability:** Hardcoded sensitive default Wi-Fi passwords (AP_PASS) in source code tracked by version control.
**Learning:** In C++ embedded projects, users might accidentally commit their personal Wi-Fi credentials if the default placeholders are standard variables.
**Prevention:** Wrap default credential strings in `#ifndef` / `#define` macros. This allows credentials to be injected securely via build-time compiler flags without modifying the tracked source files.
