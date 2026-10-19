## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Safe DOM manipulation to avoid XSS in frontend dashboards
**Vulnerability:** DOM-based XSS potential in static HTML frontend file (`web_dashboard/LD2420_Dashboard.html`) due to usage of `innerHTML` with dynamic template strings.
**Learning:** Even if data is controlled locally or comes from a trusted serial connection, using `innerHTML` to construct DOM nodes creates unnecessary security risks and fails standard SAST checks. Using programmatic DOM creation methods is safer.
**Prevention:** Avoid using `innerHTML` for dynamic UI generation. Prefer safe DOM API methods like `document.createElement`, `.className`, `.id`, `.textContent`, and `.appendChild` to construct UI elements programmatically.
## 2024-05-18 - Missing CMSIS headers when using arduino-cli
**Vulnerability:** Fatal compilation error when building for RP2040 or STM32 due to unnecessary inclusion of `<arm_math.h>` without linking the appropriate DSP libraries in the GitHub Actions runner.
**Learning:** Unused headers, especially hardware-specific ones like CMSIS, should not be included if their functions aren't being invoked. Leaving them in can cause CI pipelines to fail if the action environment lacks those exact pre-compiled libraries or include paths.
**Prevention:** Verify and remove unused headers to maintain cross-platform build stability, especially when compiling C++ for different embedded architectures.
## 2024-05-18 - Hardware macros missing before library headers in .ino files
**Vulnerability:** Fatal compilation errors ( or macro redefinitions) in Arduino sketches when hardware pin macros (e.g., ) are defined after the inclusion of library headers that rely on them.
**Learning:** In projects like LD2420, libraries often depend on macros defined in the main  file. Including headers like  before the macros are defined causes the library to compile without knowing the hardware pins, leading to fatal scope errors.
**Prevention:** Always define necessary hardware macros before including custom project headers in the main sketch. Wrap them in  guards to prevent redefinition warnings from core packages.
## 2024-05-18 - Hardware macros missing before library headers in .ino files
**Vulnerability:** Fatal compilation errors ('not declared in this scope' or macro redefinitions) in Arduino sketches when hardware pin macros (e.g., '#define PIN_LED') are defined after the inclusion of library headers that rely on them.
**Learning:** In projects like LD2420, libraries often depend on macros defined in the main '.ino' file. Including headers like 'LD2420_AppLogic.hpp' before the macros are defined causes the library to compile without knowing the hardware pins, leading to fatal scope errors.
**Prevention:** Always define necessary hardware macros before including custom project headers in the main sketch. Wrap them in '#ifndef' guards to prevent redefinition warnings from core packages.
