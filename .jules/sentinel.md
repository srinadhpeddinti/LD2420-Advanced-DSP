## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Hardcoded Wi-Fi Credentials
**Vulnerability:** Default Wi-Fi credentials (`AP_SSID` and `AP_PASS`) were hardcoded as `const char*` directly in the source files, making it difficult to securely override them during builds.
**Learning:** Hardcoded secrets in C++ source files for embedded devices can leak sensitive default configurations. Using preprocessor macros allows for secure build-time injection.
**Prevention:** Always wrap default configuration variables like passwords in `#ifndef` blocks to allow them to be overridden by compiler flags (e.g., `-DAP_PASS="secret"`).
## 2024-05-18 - C++ Struct Missing Namespace Scope
**Vulnerability:** A `struct TelemetryPacket` was placed inside a header file (`LD2420_AppLogic.hpp`) but not within the `AppLogic` namespace, despite other functions assuming it was part of that namespace, leading to "not a member of AppLogic" compilation errors in the `.ino` file.
**Learning:** In C++, when organizing code into namespaces within header files, ensure that all interdependent structs and functions are correctly encapsulated.
**Prevention:** Always verify the namespace scope of structs and functions in C++ headers.
## 2024-05-18 - ArduinoJson Deprecations
**Vulnerability:** Used deprecated ArduinoJson v6 API (`StaticJsonDocument`, `createNestedArray`, `createNestedObject`) instead of the updated v7 API (`JsonDocument`, `doc[key].to<JsonArray>()`, `add<JsonObject>()`), leading to warnings during CI builds.
**Learning:** Outdated library usages can lead to warnings and eventually breaking changes.
**Prevention:** Keep abreast of library API updates (like ArduinoJson v7) and ensure code uses the most recent, non-deprecated forms.
## 2024-05-18 - Missing Fallbacks for Architecture-Specific Attributes
**Vulnerability:** Used architecture-specific attributes like `IRAM_ATTR` without providing a fallback for architectures where they are not defined, leading to "expected initializer" compilation errors.
**Learning:** Cross-platform C++ code needs to be resilient to missing macros/attributes when compiled on different architectures.
**Prevention:** Always provide fallback definitions (`#ifndef IRAM_ATTR \n #define IRAM_ATTR \n #endif`) for architecture-specific attributes.
## 2024-05-18 - Unused Variables Breaking CI
**Vulnerability:** Unused variables (like `state_dur_s`, `rms_if`, `height_estimate_cm`, `current_velocity`) were present in the code, causing compilation to fail under strict CI rules (e.g., `-Werror=unused-variable`).
**Learning:** CI pipelines often treat warnings as errors to maintain code quality.
**Prevention:** Explicitly cast unused variables to `(void)` (e.g., `(void)unused_var;`) or comment out the parameter names in function signatures (e.g., `double /*param*/`) to suppress the warnings.
## 2024-05-18 - Missing Include Guards for Macros
**Vulnerability:** Redefined macros (like `PIN_LED`) during compilation because they were defined both in the core pins definitions and the `.ino` file without include guards.
**Learning:** Hardcoding pin macros without checking if they already exist can lead to redefinition warnings.
**Prevention:** Wrap custom pin macro definitions in `#ifndef` guards (e.g., `#ifndef PIN_LED \n #define PIN_LED 3 \n #endif`).
## 2024-05-18 - Missing Include for Extern Variable
**Vulnerability:** A function in the `.hpp` file referenced an `extern` struct (`radar`) before the `extern` definition was declared in the file.
**Learning:** In C++, the order of declarations matters. You cannot reference an `extern` variable inside an `inline` function before the `extern` variable has been declared in the translation unit.
**Prevention:** Ensure that the `extern` declaration of a variable is placed *before* any inline functions that reference it.
