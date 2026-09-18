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

## 2024-05-24 - Missing Header Inclusion Guard for ARM Math Library
**Vulnerability:** Not a security vulnerability, but a critical build failure. `arm_math.h` is conditionally included when `ARDUINO_ARCH_RP2040` is defined. However, some board cores or setups may not provide this CMSIS DSP header out of the box, causing compilation failures.
**Learning:** Hard-failing when an optional/platform-specific header isn't present breaks CI and portability.
**Prevention:** Wrap optional headers in `__has_include(<...>)` to prevent build breakage when compiling against cores that lack them.

## 2024-05-24 - Pico RP2040 Compilation Issues
**Learning:** `arduino/compile-sketches` builds can fail due to multiple reasons:
1.  **Missing `arm_math.h`**: Explicit platform condition required in header file when `<arm_math.h>` is not guaranteed to exist (added `__has_include` check).
2.  **Redefinition of `PIN_LED`**: Arduino Pico core defines `PIN_LED` in `variants/rpipico/pins_arduino.h`. Firmware sketches must wrap manual definition of `PIN_LED` in `#ifndef PIN_LED` to avoid compiler warnings treating redefinitions as errors in strict CI pipelines.
3.  **Namespacing issues**: Using classes/structs (like `TelemetryPacket`) outside of their expected namespaces, or incorrectly prefixing namespaces (`AppLogic::TelemetryPacket` vs `TelemetryPacket`) will cause build failures. Also, accessing static fields from a namespace from a global inline function in the same header caused scope errors. Passed state context directly to `getTelemetryBinary` function.
**Action:** Verify `#ifndef` around pin definitions, ensure `#include <arm_math.h>` is wrapped safely, and watch out for namespace boundaries in inline helper functions.
