## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.

## 2024-08-20 - Hardcoded AP Password in Setup Portal
**Vulnerability:** The `AP_PASS` variable was hardcoded to a default value (`"YOUR_SETUP_PASSWORD"`) in the ESP8266 and ESP32 WiFi setup routines, allowing unauthorized access to the device's configuration portal if the user never changed the default.
**Learning:** Default, hardcoded credentials are a significant risk for IoT devices, especially in captive portals used for provisioning. They are trivially exploitable.
**Prevention:** Avoid hardcoded credentials. Generate dynamic, device-specific credentials based on unique hardware identifiers (like the MAC address) if a user-supplied password is not provided.
