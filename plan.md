1. **Identify the vulnerability:** The `web_dashboard/LD2420_Dashboard.html` file uses `innerHTML` for DOM manipulation (around line 1174), which can lead to DOM-based Cross-Site Scripting (XSS) vulnerabilities, even with seemingly safe interpolations.
2. **Fix the vulnerability:** I will replace the `innerHTML` assignment with safe DOM methods using `document.createElement` and `textContent`. This ensures that data is inserted safely and prevents any potential XSS.
3. **Pre-commit Steps:** Complete pre-commit steps to make sure proper testing, verifications, reviews and reflections are done.
4. **Submit:** Commit and submit the code with the appropriate title "🛡️ Sentinel: [HIGH] Fix DOM XSS vulnerability".
