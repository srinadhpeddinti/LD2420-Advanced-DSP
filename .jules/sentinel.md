## 2024-05-18 - State-modifying API endpoints allowing GET requests
**Vulnerability:** CSRF vulnerability due to state-modifying endpoints (/api/cmd and /api/thresholds) allowing simple GET requests.
**Learning:** In simple ESP-based web servers, it is common to define routes without a specific HTTP method restriction. This allows unintended state changes via cross-origin GET requests.
**Prevention:** Always specify HTTP_POST as the method for state-modifying endpoints in the httpServer.on definition, and ensure CORS preflight OPTIONS requests are handled to support legitimate client applications.
## 2024-05-18 - Missing input validation on threshold configuration endpoints
**Vulnerability:** The endpoints configuring radar thresholds (`/api/thresholds`) allow configuring out-of-bounds integer parameters which may cause undefined behaviour or crashes in the radar detection algorithms. Moreover, the command endpoint (`/api/cmd`) doesn't fail securely when provided an invalid action parameter.
**Learning:** For embedded systems, input validation and enforcing bounds (like integer min/max constraints corresponding to UI ranges) is vital to ensure robust hardware performance and avoid overflows or invalid memory states. It's important to never trust client-side validation.
**Prevention:** Always parse and explicitly bounds-check incoming integer or string arguments against expected constraints before assigning them to state variables or passing them down to hardware config.
