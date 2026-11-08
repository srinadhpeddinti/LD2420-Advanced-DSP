## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2025-02-18 - XSS Risk in web dashboard via innerHTML
**Vulnerability:** Use of innerHTML in `renderZoneMap` without sanitization creates a potential DOM-based Cross-Site Scripting (XSS) vulnerability, even if the current interpolated variables seem safe (e.g., integers).
**Learning:** Even static HTML dashboards reading data from embedded hardware are susceptible to XSS if the data payloads are ever maliciously modified or the code is later refactored to include user inputs.
**Prevention:** Always use safe DOM manipulation alternatives like `document.createElement()`, `classList`, and `textContent` instead of `innerHTML` when dynamically building UI elements.
## 2025-02-18 - CI Action compilation failure due to incorrect library path
**Vulnerability:** Not a direct vulnerability, but a workflow bug affecting the stability and testability of code which is crucial for security.
**Learning:** `arduino/compile-sketches` action relies on exact path resolutions. Using a relative path for custom local libraries like `./libraries/LibraryName` in `cli-compile-flags` can fail to locate local files during CI builds on GitHub.
**Prevention:** Always use the absolute path variable `${{ github.workspace }}` for custom local libraries to ensure proper resolution by the `arduino-cli`.
## 2025-02-18 - Extraneous CMSIS headers causing CI compilation failure
**Vulnerability:** Workflow/build stability issue.
**Learning:** Including hardware-specific CMSIS headers like `<arm_math.h>` when their functions are not utilized can cause fatal 'No such file or directory' compilation errors if the DSP libraries are not explicitly linked, specifically affecting `arduino/compile-sketches` builds for RP2040/STM32.
**Prevention:** Do not unnecessarily include `<arm_math.h>` or similar CMSIS headers in cross-platform DSP header files if the CMSIS DSP library features are not directly invoked.
