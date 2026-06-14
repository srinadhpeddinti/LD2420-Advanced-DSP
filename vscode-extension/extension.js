const vscode = require('vscode');

function activate(context) {
    console.log('LD2420 Extension Active');

    let disposable = vscode.commands.registerCommand('ld2420.startDashboard', function () {
        const panel = vscode.window.createWebviewPanel(
            'ld2420Dashboard',
            'LD2420 Radar Live',
            vscode.ViewColumn.One,
            { enableScripts: true }
        );

        panel.webview.html = getWebviewContent();
    });

    context.subscriptions.push(disposable);
}

function getWebviewContent() {
    return `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>LD2420</title>
    <style>
        body { font-family: sans-serif; padding: 20px; color: white; background: #1e1e1e; }
        .card { border: 1px solid #444; padding: 10px; margin-bottom: 10px; }
    </style>
</head>
<body>
    <h1>LD2420 Live Telemetry</h1>
    <div class="card" id="status">Waiting for Web Serial Connection...</div>
    <button onclick="requestSerial()">Connect Radar</button>
    
    <script>
        async function requestSerial() {
            try {
                const port = await navigator.serial.requestPort();
                await port.open({ baudRate: 115200 });
                document.getElementById('status').innerText = 'Connected! Awaiting binary packets...';
                // Implement binary packet parser here...
            } catch (err) {
                document.getElementById('status').innerText = 'Error: ' + err.message;
            }
        }
    </script>
</body>
</html>`;
}

function deactivate() {}

module.exports = {
    activate,
    deactivate
}
