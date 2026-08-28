1. **Frontend modification:** Edit `web_dashboard/LD2420_Dashboard.html` to add the `X-Requested-With: XMLHttpRequest` header to all POST requests in `apiFetch`.
2. **Frontend verification:** Verify the changes in `web_dashboard/LD2420_Dashboard.html` using `read_file`.
3. **ESP32 backend modification:** Edit `firmware/LD2420_ESP32_WiFi/LD2420_ESP32_WiFi.ino`. In `setup()`, call `httpServer.collectHeaders` for `X-Requested-With` and `Origin`. In `handleApiCmd()` and `handleApiThresholds()`, add CSRF header checks (require `X-Requested-With: XMLHttpRequest`). For the CORS policy (`HTTP_OPTIONS` and regular handlers), instead of sending `Access-Control-Allow-Origin: *`, we will read the incoming `Origin` header. If it's a valid local network IP (or empty/null for local file access), we echo it back. Otherwise, we deny access or simply don't send the permissive headers. Actually, a simpler and robust way for this isolated device is to check the `Origin` header dynamically. If the origin is present, ensure it matches `http://<device_ip>` or similar, or just echo it back safely.
Wait, let me rethink based on Sentinel's journal: "To mitigate CSRF on local ESP8266/ESP32 web servers without breaking legitimate cross-origin requests, require a custom HTTP header (e.g., 'X-Requested-With') on state-modifying endpoints (POST) instead of simply removing CORS headers or OPTIONS preflight routes. Note: `httpServer.collectHeaders()` must be called in `setup()` for the ESP web server to parse custom headers. On the frontend, ensure this custom header is only injected into POST requests (e.g., `if (options.method === 'POST')`) to prevent breaking GET requests with CORS preflight failures."
Wait, the code review said: "The security enhancement is fundamentally flawed and acts as "security theater"... Because the API explicitly allows cross-origin requests from any origin (Access-Control-Allow-Origin: *), and the patch updates the preflight OPTIONS handler to explicitly permit the custom header... the protection is trivially bypassed. An attacker on a malicious website (https://evil.com) can simply use fetch() to send the request with the custom header. The browser's preflight check will succeed due to the wildcard CORS policy, and the malicious POST request will be executed."
Ah! The reviewer is saying that if I return `Access-Control-Allow-Origin: *` AND `Access-Control-Allow-Headers: X-Requested-With`, then the custom header CSRF protection doesn't work because `fetch` from *any* origin is allowed by the preflight.
So I should **NOT** add `Access-Control-Allow-Headers: X-Requested-With` to the `OPTIONS` handler. I should just remove it! If I remove it, the browser will NOT allow a cross-origin `fetch` with a custom header because the preflight response won't contain `Access-Control-Allow-Headers: X-Requested-With`.
Wait, the memory specifically says: "instead of simply removing CORS headers or OPTIONS preflight routes."
If I remove `Access-Control-Allow-Headers: X-Requested-With`, then a legitimate cross-origin request from a local dashboard (e.g. `file://` or a separate web server) will fail the preflight for POST requests.
Let's re-read the Sentinel memory:
"To mitigate CSRF on local ESP8266/ESP32 web servers without breaking legitimate cross-origin requests, require a custom HTTP header (e.g., 'X-Requested-With') on state-modifying endpoints (POST) instead of simply removing CORS headers or OPTIONS preflight routes."

If I just don't add `Access-Control-Allow-Headers: X-Requested-With`, then any cross-origin POST request with that header will fail preflight. But wait, if the user explicitly wants to host the dashboard elsewhere and needs cross-origin POST, how does it work without `Access-Control-Allow-Headers: X-Requested-With`? It doesn't.
The review says: "To truly fix this without authentication, the server would need a restrictive CORS policy (e.g., verifying the Origin against a whitelist or restricting it entirely), which might conflict with how the dashboard is currently intended to be hosted."

Wait, how do I secure this?
If the dashboard is hosted locally (e.g. `http://ld2420.local` or `http://192.168.1.100`), the `Origin` is `http://ld2420.local`. If it's loaded from `file://`, `Origin` is `null`.
I can't just use `Access-Control-Allow-Origin: *`. I should probably implement a dynamic CORS policy where I read the `Origin` header. If it's `null` (file://) or matches the device's IP/hostname, I return it. Otherwise, I reject it.
Wait, if it's a custom header, I can just not send `Access-Control-Allow-Headers`? No, if I want to support cross origin for the frontend (like the memory says: "without breaking legitimate cross-origin requests"), maybe I shouldn't use wildcard `*` for the origin?

Let's look at the memory again:
"To mitigate CSRF on local ESP8266/ESP32 web servers without breaking legitimate cross-origin requests, require a custom HTTP header (e.g., 'X-Requested-With') on state-modifying endpoints (POST) instead of simply removing CORS headers or OPTIONS preflight routes. Note: httpServer.collectHeaders() must be called in setup() for the ESP web server to parse custom headers. On the frontend, ensure this custom header is only injected into POST requests (e.g., if (options.method === 'POST')) to prevent breaking GET requests with CORS preflight failures."

The reviewer complained about:
"Because the API explicitly allows cross-origin requests from any origin (Access-Control-Allow-Origin: *), and the patch updates the preflight OPTIONS handler to explicitly permit the custom header... the protection is trivially bypassed. An attacker on a malicious website (https://evil.com) can simply use fetch() to send the request with the custom header. The browser's preflight check will succeed due to the wildcard CORS policy, and the malicious POST request will be executed."

Ah! The reviewer says that returning `Access-Control-Allow-Origin: *` AND `Access-Control-Allow-Headers: X-Requested-With` makes it insecure because *any* origin can send the custom header.
So I should NOT return `Access-Control-Allow-Origin: *` for OPTIONS or POST requests if they have the custom header? No. I must remove `Access-Control-Allow-Origin: *` completely and instead echo back the `Origin` header if it's considered safe, OR since we don't have authentication, maybe I should just NOT add `Access-Control-Allow-Headers: X-Requested-With`? No, if I don't add it, legitimate cross-origin POST fails.
Wait, if the memory says "without breaking legitimate cross-origin requests, require a custom HTTP header", maybe the memory is wrong according to the reviewer? The reviewer expects the memory instruction to be implemented securely.

How to make custom header CSRF protection secure while supporting CORS?
Instead of `Access-Control-Allow-Origin: *`, we should reflect the `Origin` header back, but ONLY if we trust it? No, if we reflect it back for *any* origin, it's the same as `*`.
But wait! According to the CORS specification, `Access-Control-Allow-Origin: *` CANNOT be used with credentials (cookies/auth). But we don't use credentials here.
If we don't use credentials, then `fetch('http://device/', {headers: {'X-Requested-With': '...'}})` from `https://evil.com` will trigger a preflight. If the preflight returns `Access-Control-Allow-Origin: *` and `Access-Control-Allow-Headers: X-Requested-With`, the browser ALLOWS the request. `evil.com` can send commands. This is what the reviewer pointed out.

To prevent `evil.com` from sending commands via `fetch`, we need to restrict which origins are allowed to send the `X-Requested-With` header. But we don't know the exact origin of the legitimate dashboard (could be a local file, could be hosted on another server like Home Assistant).
Wait! What if we DO NOT include `Access-Control-Allow-Headers: X-Requested-With` in the preflight response?
If we do not include it, then `https://evil.com` CANNOT send the custom header via `fetch`, because the browser will block it.
BUT, if we don't include it, how can the *legitimate* dashboard (if hosted cross-origin) send the custom header? It can't either!
Wait. If the dashboard is hosted on the ESP itself, it is NOT cross-origin! It's same-origin. Same-origin requests do NOT trigger preflight, and they CAN send custom headers.
What if the dashboard is a `file://` URL? `file://` might be treated as cross-origin or opaque origin.
If the dashboard is hosted on a CDN or Home Assistant? Then it IS cross-origin.

How to allow legitimate cross-origin requests but block `evil.com`?
We can't, unless we know which origins are legitimate.
Wait, the reviewer said: "The patch attempts to add Cross-Site Request Forgery (CSRF) protection by requiring the X-Requested-With: XMLHttpRequest header on state-changing API endpoints, and it updates the frontend dashboard to append this header to fetch calls... However, the implementation is fundamentally flawed. While this prevents basic HTML <form>-based CSRF, the agent also updated the CORS preflight (OPTIONS) handlers to return Access-Control-Allow-Headers: X-Requested-With while leaving Access-Control-Allow-Origin: * intact. Because the server explicitly allows any origin (*) to send this custom header, a malicious website can easily bypass the protection by using a simple fetch() request with the custom header included."

So the reviewer explicitly points out that leaving `Access-Control-Allow-Origin: *` intact while adding `Access-Control-Allow-Headers: X-Requested-With` is the flaw.
If I simply remove `Access-Control-Allow-Headers: X-Requested-With`, then the reviewer's first review complained:
"Because the patch fails to update the server's OPTIONS handlers to include X-Requested-With in the Access-Control-Allow-Headers response, the browser will reject the preflight and block the actual request. This completely breaks the dashboard's ability to communicate with the ESP device."

Ah! So the reviewer rejected BOTH:
1. When I DID NOT add `Access-Control-Allow-Headers: X-Requested-With` (rejected because it breaks legitimate cross-origin requests).
2. When I DID add `Access-Control-Allow-Headers: X-Requested-With` (rejected because with `Allow-Origin: *` it allows evil.com).

This means I MUST change `Access-Control-Allow-Origin: *` to something more restrictive, OR I must validate the `Origin` header.
Wait, if I validate the `Origin` header, what should I validate it against?
It's an ESP device on a local network. Valid origins could be `http://ld2420.local`, `http://192.168.x.x`, or `null` (for `file://`), or `http://localhost`.
If I check if the `Origin` header is missing, or matches local network patterns?
Or what if I just use CORS properly: I read the `Origin` header from the request, and ONLY if it's a private IP, localhost, or `file://` (`null`), I echo it back as the `Access-Control-Allow-Origin` header?
Actually, `evil.com`'s `Origin` is `https://evil.com`. A local dashboard's origin is `file://` (null), `http://localhost:8080`, or the device's IP itself.
I can check if the `Origin` is missing (same-origin request, or non-browser) and just allow it (by not sending CORS headers, or sending `*`). But if the `Origin` is present, I can check if it's a local address.

Let's look at how to implement this in ESP8266WebServer.
```cpp
void handleCors(ESP8266WebServer& server) {
    String origin = server.header("Origin");
    if (origin == "") {
        // Not a CORS request, or same-origin (depending on browser, usually omitted for same-origin)
        server.sendHeader("Access-Control-Allow-Origin", "*");
    } else {
        // It's a cross-origin request. Check if it's safe.
        // For simplicity, we could allow 'null' (file://), localhost, and local IPs (192.168.*, 10.*, 172.16.*)
        if (origin == "null" || origin.startsWith("http://192.168.") || origin.startsWith("http://10.") || origin.startsWith("http://172.") || origin.startsWith("http://localhost")) {
            server.sendHeader("Access-Control-Allow-Origin", origin);
        } else {
            // Reject or just don't send the allow header
        }
    }
}
```
Wait, if I just do that, it's a lot of code. Is there a simpler way?
What if I use a CSRF token? There is no session.
What if I use a custom header but I only allow CORS for `GET` requests, and forbid CORS for `POST` requests?
But the memory says: "To mitigate CSRF on local ESP8266/ESP32 web servers without breaking legitimate cross-origin requests, require a custom HTTP header (e.g., 'X-Requested-With') on state-modifying endpoints (POST) instead of simply removing CORS headers or OPTIONS preflight routes... On the frontend, ensure this custom header is only injected into POST requests (e.g., if (options.method === 'POST')) to prevent breaking GET requests with CORS preflight failures."
If the custom header is ONLY injected into POST requests, this means GET requests don't have the custom header, so they don't trigger preflight, so they work fine with `Access-Control-Allow-Origin: *`.
For POST requests, they DO have the custom header, so they DO trigger preflight.
If I don't add `Access-Control-Allow-Headers: X-Requested-With`, then the preflight for POST fails, which breaks cross-origin POST requests. The reviewer specifically said that breaks the dashboard.
If I DO add it, the preflight for POST succeeds for ALL origins (because of `*`), which the reviewer said is insecure.

So the ONLY solution is to change `Access-Control-Allow-Origin: *` on the `OPTIONS` and `POST` handlers!
How can I change it?
```cpp
    httpServer.on("/api/cmd", HTTP_OPTIONS, []() {
        String origin = httpServer.header("Origin");
        if (origin.startsWith("http://192.168.") || origin.startsWith("http://10.") || origin == "null" || origin.startsWith("http://localhost")) {
            httpServer.sendHeader("Access-Control-Allow-Origin", origin);
        }
        httpServer.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        httpServer.sendHeader("Access-Control-Allow-Headers", "X-Requested-With");
        httpServer.send(204);
    });
```
Wait, the ESP doesn't know the full network range perfectly, but this is a good approximation.
Wait! What if the user is using `ld2420.local`?
`origin.startsWith("http://ld2420.local")`

Is there a simpler rule?
What if we just check that the `Origin` header does NOT end in a public TLD? E.g., doesn't have `.com`, `.net`, etc.
Or what if we just check if the `Host` header matches the `Origin`? No, cross-origin means they don't match.

What if we just generate a random token on boot, and send it to the frontend via a `GET` request (or embed it in the initial payload), and the frontend must send it back?
```cpp
// In setup:
long csrf_token = random(0, 9999999);

// In getTelemetryJson:
doc["csrf_token"] = csrf_token;

// In frontend:
const token = data.csrf_token;
apiFetch('/api/cmd?action=...', { method: 'POST', headers: { 'X-CSRF-Token': token } });
```
Wait, if `evil.com` makes a `fetch` GET request, it CAN read the JSON response because of `Access-Control-Allow-Origin: *`!
Yes! `evil.com` can just fetch the GET endpoint, read the `csrf_token`, and then send the POST request.
So `Access-Control-Allow-Origin: *` makes CSRF tokens useless if the API is entirely open.

Therefore, `Access-Control-Allow-Origin: *` is the root cause of the insecurity!
To secure the device against malicious websites while maintaining functionality, we MUST restrict `Access-Control-Allow-Origin`.

But how to restrict it without breaking legitimate usage?
If a legitimate user downloads the HTML file and opens it in their browser (`file:///.../LD2420_Dashboard.html`), the `Origin` is `null`.
If we allow `Origin: null`, then any iframe with a `data:` URI or sandboxed iframe can also have an `Origin` of `null` and bypass the check. BUT modern browsers have tightened `Origin: null`.
Still, checking if `origin == "null"` is a common pattern for local files.

Let's rethink. Is there any way to allow `*` safely?
No. If you allow `*` to read, then any site can read data. If you allow `*` to write (POST) with the custom header, any site can write.

What if we only allow the POST request if the `Origin` is omitted (which happens for same-origin requests)?
If we do `if (httpServer.header("Origin") != "") return 403;` then cross-origin POST is completely blocked. This violates the memory instruction: "without breaking legitimate cross-origin requests".

What if we require `X-Requested-With: XMLHttpRequest` and we DON'T check the Origin, but we DON'T put `Access-Control-Allow-Origin: *` on the POST endpoints and OPTIONS endpoints?
If we remove `Access-Control-Allow-Origin: *` from the POST and OPTIONS endpoints, then `evil.com`'s browser will fail the preflight because there is no `Access-Control-Allow-Origin`.
But legitimate cross-origin requests will ALSO fail!

Wait. If we want legitimate cross-origin requests to succeed, the server MUST send `Access-Control-Allow-Origin: <Legitimate-Origin>`.
This means the server MUST know what `<Legitimate-Origin>` is.
If the server just echoes back the `Origin` header blindly:
```cpp
httpServer.sendHeader("Access-Control-Allow-Origin", httpServer.header("Origin"));
```
Then `evil.com`'s browser will see `Access-Control-Allow-Origin: https://evil.com` and ALLOW the request! This is exactly equivalent to `*`.

So the server MUST validate the `Origin`.
```cpp
bool isSafeOrigin(String origin) {
    if (origin == "" || origin == "null") return true;
    if (origin.indexOf("localhost") != -1) return true;
    if (origin.indexOf("192.168.") != -1) return true;
    if (origin.indexOf("10.") != -1) return true;
    if (origin.indexOf("172.") != -1) return true;
    if (origin.indexOf(".local") != -1) return true;
    return false;
}
```
If we do this, it will protect against `https://evil.com` because `evil.com` is not in the safe list.
This allows legitimate local network cross-origin requests.

Let's refine `isSafeOrigin`.
```cpp
bool isSafeOrigin(const String& origin) {
    if (origin.length() == 0 || origin == "null") return true;
    if (origin.indexOf("://localhost") != -1) return true;
    if (origin.indexOf("://192.168.") != -1) return true;
    if (origin.indexOf("://10.") != -1) return true;
    if (origin.indexOf("://172.") != -1) return true; // covers 172.16.x.x to 172.31.x.x
    if (origin.indexOf(".local") != -1) return true;
    return false;
}
```

Wait, `httpServer.hasHeader("Origin")` can be used.

Let's check if the memory explicitly says:
"To mitigate CSRF on local ESP8266/ESP32 web servers without breaking legitimate cross-origin requests, require a custom HTTP header (e.g., 'X-Requested-With') on state-modifying endpoints (POST) instead of simply removing CORS headers or OPTIONS preflight routes."

If I implement this:
1. `setup()`: `const char* headers[] = {"X-Requested-With", "Origin"}; httpServer.collectHeaders(headers, 2);`
2. Update the `HTTP_OPTIONS` handler:
```cpp
    httpServer.on("/api/cmd", HTTP_OPTIONS, []() {
        String origin = httpServer.header("Origin");
        if (isSafeOrigin(origin)) {
            httpServer.sendHeader("Access-Control-Allow-Origin", origin.length() ? origin : "*");
            httpServer.sendHeader("Access-Control-Allow-Headers", "X-Requested-With");
        } else {
            httpServer.sendHeader("Access-Control-Allow-Origin", "http://localhost"); // Dummy to fail CORS
        }
        httpServer.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        httpServer.send(204);
    });
```
This is getting complicated.
Is there an easier way?
What if I use `Access-Control-Allow-Origin: *` for GET requests (so `evil.com` can read), but for POST requests, I DO NOT send `Access-Control-Allow-Origin: *`.
Wait, the reviewer said:
"While this prevents basic HTML <form>-based CSRF, the agent also updated the CORS preflight (OPTIONS) handlers to return Access-Control-Allow-Headers: X-Requested-With while leaving Access-Control-Allow-Origin: * intact. Because the server explicitly allows any origin (*) to send this custom header, a malicious website can easily bypass the protection by using a simple fetch() request with the custom header included. The browser's preflight check will approve it, nullifying the CSRF defense against modern web attackers."

If the server explicitly allows any origin (`*`) to send this custom header, it's insecure.
So I should NOT allow any origin `*` to send the custom header.
How do I allow ONLY the frontend to send the custom header?
If the frontend is served from the ESP itself (e.g., `http://192.168.1.100/`), it is SAME ORIGIN.
Same origin requests DO NOT send an `OPTIONS` preflight, AND they are allowed to send custom headers!
But the memory says: "without breaking legitimate cross-origin requests".
If I have to support cross-origin requests, I MUST use `OPTIONS` preflight.
How to support legitimate cross origin requests but not malicious ones?
I can just echo the `Origin` header dynamically!
```cpp
String origin = httpServer.header("Origin");
if (origin != "") {
    httpServer.sendHeader("Access-Control-Allow-Origin", origin);
} else {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
}
```
Wait, echoing the `Origin` header dynamically is functionally EQUIVALENT to `*`. It means `https://evil.com` sends a request, the ESP echoes `Access-Control-Allow-Origin: https://evil.com`, and the browser ALLOWS the request. The reviewer will fail this too!

The ONLY way is to validate the `Origin`.

```cpp
bool isSafeOrigin(const String& origin) {
    if (origin == "" || origin == "null") return true;
    if (origin.startsWith("http://192.168.")) return true;
    if (origin.startsWith("http://10.")) return true;
    if (origin.startsWith("http://172.")) return true;
    if (origin.startsWith("http://localhost")) return true;
    if (origin.endsWith(".local")) return true;
    return false;
}
```

Wait, `origin.endsWith(".local")` would match `http://ld2420.local`. It would also match `https://evil.local`, but that's a local address.

Let's define a function in both `.ino` files:
```cpp
bool isSafeOrigin(const String& origin) {
    if (origin == "" || origin == "null") return true;
    if (origin.indexOf("://192.168.") != -1) return true;
    if (origin.indexOf("://10.") != -1) return true;
    if (origin.indexOf("://172.") != -1) return true;
    if (origin.indexOf("://localhost") != -1) return true;
    if (origin.indexOf(".local") != -1) return true;
    return false;
}

void setCorsHeaders() {
    String origin = httpServer.header("Origin");
    if (isSafeOrigin(origin)) {
        httpServer.sendHeader("Access-Control-Allow-Origin", origin.length() ? origin : "*");
        httpServer.sendHeader("Access-Control-Allow-Headers", "X-Requested-With");
    }
}
```

Then in `handleApiCmd`:
```cpp
void handleApiCmd() {
    setCorsHeaders();
    if (httpServer.header("X-Requested-With") != "XMLHttpRequest") {
        httpServer.send(403, "text/plain", "CSRF detected");
        return;
    }
    // ...
}
```

And in `HTTP_OPTIONS`:
```cpp
    httpServer.on("/api/cmd", HTTP_OPTIONS, []() {
        setCorsHeaders();
        httpServer.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        httpServer.send(204);
    });
```
This is robust and secure!

Wait! Does `isSafeOrigin` handle everything?
What if the user accesses the device via an IP address that doesn't start with those? E.g., a public IP? If they expose it to the internet, then they might access it via a public IP. But they shouldn't do that without auth.
For a local device, this is a very standard and accepted practice to prevent CSRF.

Let's double check the files.
`firmware/LD2420_ESP32_WiFi/LD2420_ESP32_WiFi.ino`
`firmware/LD2420_ESP8266_WiFi/LD2420_ESP8266_WiFi.ino`

Wait! Is there an even simpler way?
What if I use `Origin` validation directly in `handleApiCmd` instead of custom header?
The journal explicitly says:
"To mitigate CSRF on local ESP8266/ESP32 web servers without breaking legitimate cross-origin requests, require a custom HTTP header (e.g., 'X-Requested-With') on state-modifying endpoints (POST) instead of simply removing CORS headers or OPTIONS preflight routes. Note: httpServer.collectHeaders() must be called in setup() for the ESP web server to parse custom headers. On the frontend, ensure this custom header is only injected into POST requests (e.g., if (options.method === 'POST')) to prevent breaking GET requests with CORS preflight failures."
It specifically asked to use the custom header.
So I MUST use the custom header.
But the reviewer said the custom header doesn't work if I leave `Access-Control-Allow-Origin: *` AND add `Access-Control-Allow-Headers: X-Requested-With` for all origins.
Therefore, the instruction is to use the custom header AND securely handle CORS.

```cpp
bool isSafeOrigin(const String& origin) {
    if (origin == "" || origin == "null") return true;
    if (origin.indexOf("://192.168.") != -1) return true;
    if (origin.indexOf("://10.") != -1) return true;
    if (origin.indexOf("://172.") != -1) return true;
    if (origin.indexOf("://localhost") != -1) return true;
    if (origin.indexOf(".local") != -1) return true;
    return false;
}

void setSecureCorsHeaders() {
    String origin = httpServer.header("Origin");
    if (isSafeOrigin(origin)) {
        httpServer.sendHeader("Access-Control-Allow-Origin", origin.length() ? origin : "*");
        httpServer.sendHeader("Access-Control-Allow-Headers", "X-Requested-With");
    }
}
```
Wait, the frontend `apiFetch` does `apiFetch('/api/cmd...')`. If I change `handleApiData` to also use `setSecureCorsHeaders`? No, `handleApiData` is a GET request, so it doesn't need to be restricted. It currently has `httpServer.sendHeader("Access-Control-Allow-Origin", "*");`.

Let's check `handleApiCmd`:
```cpp
void handleApiCmd() {
    String origin = httpServer.header("Origin");
    if (isSafeOrigin(origin)) {
        httpServer.sendHeader("Access-Control-Allow-Origin", origin.length() ? origin : "*");
    }
    if (httpServer.header("X-Requested-With") != "XMLHttpRequest") {
        httpServer.send(403, "text/plain", "CSRF detected");
        return;
    }
```
Wait, `handleApiCmd` doesn't need to send `Access-Control-Allow-Headers`, because that's for the `OPTIONS` preflight response.
So for `handleApiCmd`:
```cpp
void handleApiCmd() {
    String origin = httpServer.header("Origin");
    if (isSafeOrigin(origin)) {
        httpServer.sendHeader("Access-Control-Allow-Origin", origin.length() ? origin : "*");
    }
    if (httpServer.header("X-Requested-With") != "XMLHttpRequest") {
        httpServer.send(403, "text/plain", "CSRF detected");
        return;
    }
    // ...
```
And for `OPTIONS`:
```cpp
    httpServer.on("/api/cmd", HTTP_OPTIONS, []() {
        String origin = httpServer.header("Origin");
        if (isSafeOrigin(origin)) {
            httpServer.sendHeader("Access-Control-Allow-Origin", origin.length() ? origin : "*");
            httpServer.sendHeader("Access-Control-Allow-Headers", "X-Requested-With");
        }
        httpServer.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
        httpServer.send(204);
    });
```
This is perfect! It only allows safe origins to make the cross-origin request, AND it requires the custom header.

Let's write a replacement block. I will define `isSafeOrigin` just before `handleApiCmd`.

```cpp
bool isSafeOrigin(const String& origin) {
    if (origin == "" || origin == "null") return true;
    if (origin.indexOf("://192.168.") != -1) return true;
    if (origin.indexOf("://10.") != -1) return true;
    if (origin.indexOf("://172.") != -1) return true;
    if (origin.indexOf("://localhost") != -1) return true;
    if (origin.indexOf(".local") != -1) return true;
    return false;
}

void handleApiCmd() {
    String origin = httpServer.header("Origin");
    if (isSafeOrigin(origin)) {
        httpServer.sendHeader("Access-Control-Allow-Origin", origin.length() ? origin : "*");
    }
    if (httpServer.header("X-Requested-With") != "XMLHttpRequest") {
        httpServer.send(403, "text/plain", "CSRF detected");
        return;
    }
    if (!httpServer.hasArg("action")) {
```

Wait, `isSafeOrigin` is great, but what if they access via a hostname that doesn't end in `.local`?
If they do that, it's safer to just check if `Origin` matches the `Host` header.
```cpp
    String origin = httpServer.header("Origin");
    String host = httpServer.header("Host");
    // If origin is exactly http:// + host, then it's same origin!
    if (origin == "" || origin == "null" || origin == ("http://" + host)) {
```
But wait, if it's same origin, the browser might not send `Origin` anyway.
If it's cross origin, they want it to work (e.g. from Home Assistant). Home assistant uses `http://192.168...` which matches my `isSafeOrigin` checks.

Let's use the `isSafeOrigin` approach.
Wait, `httpServer.collectHeaders` needs to collect `"Origin"`.
```cpp
    const char * headerkeys[] = {"X-Requested-With", "Origin"};
    httpServer.collectHeaders(headerkeys, 2);
```

Let's update the files.
