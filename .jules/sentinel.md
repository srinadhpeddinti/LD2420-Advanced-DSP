## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Replacing innerHTML with programmatic DOM creation
**Vulnerability:** Use of innerHTML for constructing DOM elements, even with seemingly safe interpolations, can lead to DOM-based XSS vulnerabilities if the inputs are ever derived from user-controlled data.
**Learning:** It's a common anti-pattern to use innerHTML for convenience. Removing it in favor of programmatic DOM creation is a classic "defense in depth" security enhancement.
**Prevention:** Always use safe DOM manipulation methods like document.createElement, textContent, and appendChild instead of innerHTML.
## 2024-05-18 - Unused hardware-specific CMSIS headers causing CI failures
**Vulnerability:** Not a direct vulnerability, but an operational issue causing pipeline breakage. Unnecessarily including hardware-specific CMSIS headers like `arm_math.h` when their functions are not used causes fatal "No such file or directory" compilation errors in CI environments that don't have the DSP libraries explicitly linked or present.
**Learning:** Only include headers that are actually used by the code. Extraneous architecture-specific headers create brittle builds.
**Prevention:** Verify that included libraries are actually utilized. Remove unused `#include <arm_math.h>`.
## 2024-05-18 - Deprecated ArduinoJson methods and namespace issues
**Vulnerability:** Not a direct vulnerability, but compilation errors caused by deprecated library methods (`createNestedArray`, `createNestedObject`, `StaticJsonDocument`) and missing namespace qualifiers (like `AppLogic::`) for structures.
**Learning:** Libraries evolve, and relying on deprecated methods or not properly scoping structures within namespaces can lead to broken builds, especially when dependencies are updated in CI.
**Prevention:** Keep code updated with library changes (e.g., migrating to ArduinoJson v7 syntax) and ensure proper namespace scope is used when accessing structures defined within them.
