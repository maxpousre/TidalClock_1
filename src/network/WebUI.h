/**
 * TideClock Web User Interface
 *
 * Single-page HTML application with 4 tabs:
 * - Main Control
 * - Advanced (Motor Testing)
 * - Configuration
 * - Status & Logs
 */

#ifndef WEB_UI_H
#define WEB_UI_H

const char* getWebUI() {
    return R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TideClock Control Panel</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #4a5568 0%, #2d3748 100%);
            color: white;
            padding: 25px;
            text-align: center;
        }
        .header h1 {
            font-size: 32px;
            margin-bottom: 5px;
        }
        .header p {
            opacity: 0.9;
            font-size: 14px;
        }
        .status-bar {
            background: #f7fafc;
            padding: 15px 25px;
            border-bottom: 1px solid #e2e8f0;
            display: flex;
            justify-content: space-between;
            align-items: center;
            flex-wrap: wrap;
            gap: 10px;
        }
        .status-item {
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .status-badge {
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: bold;
        }
        .badge-ready { background: #48bb78; color: white; }
        .badge-homing { background: #ed8936; color: white; }
        .badge-testing { background: #4299e1; color: white; }
        .badge-error { background: #f56565; color: white; }
        .badge-stop { background: #c53030; color: white; }
        .tabs {
            display: flex;
            background: #edf2f7;
            border-bottom: 2px solid #cbd5e0;
        }
        .tab {
            flex: 1;
            padding: 15px;
            text-align: center;
            cursor: pointer;
            font-weight: 600;
            color: #4a5568;
            transition: all 0.3s;
            border: none;
            background: transparent;
        }
        .tab:hover {
            background: #e2e8f0;
        }
        .tab.active {
            background: white;
            color: #667eea;
            border-bottom: 3px solid #667eea;
        }
        .content {
            padding: 25px;
            min-height: 500px;
        }
        .tab-panel {
            display: none;
        }
        .tab-panel.active {
            display: block;
        }
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
            margin: 5px;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-primary:hover {
            background: #5568d3;
        }
        .btn-danger {
            background: #f56565;
            color: white;
            font-size: 18px;
            padding: 18px 36px;
        }
        .btn-danger:hover {
            background: #e53e3e;
        }
        .btn-success {
            background: #48bb78;
            color: white;
        }
        .btn-success:hover {
            background: #38a169;
        }
        .btn-warning {
            background: #ed8936;
            color: white;
        }
        .btn-secondary {
            background: #a0aec0;
            color: white;
        }
        .btn:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }
        .card {
            background: #f7fafc;
            border: 1px solid #e2e8f0;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .card h3 {
            color: #2d3748;
            margin-bottom: 15px;
            font-size: 18px;
        }
        .form-group {
            margin-bottom: 15px;
        }
        .form-group label {
            display: block;
            margin-bottom: 5px;
            font-weight: 600;
            color: #4a5568;
        }
        .form-group input, .form-group select {
            width: 100%;
            padding: 10px;
            border: 1px solid #cbd5e0;
            border-radius: 6px;
            font-size: 14px;
        }
        .form-row {
            display: flex;
            gap: 15px;
        }
        .form-row .form-group {
            flex: 1;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
        }
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #e2e8f0;
        }
        th {
            background: #edf2f7;
            font-weight: 600;
            color: #2d3748;
        }
        .switch-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(80px, 1fr));
            gap: 10px;
            margin-top: 15px;
        }
        .switch-item {
            background: #edf2f7;
            padding: 15px;
            border-radius: 6px;
            text-align: center;
            font-size: 12px;
        }
        .switch-item.triggered {
            background: #48bb78;
            color: white;
            font-weight: bold;
        }
        .log-viewer {
            background: #1a202c;
            color: #e2e8f0;
            padding: 15px;
            border-radius: 6px;
            height: 300px;
            overflow-y: auto;
            font-family: 'Courier New', monospace;
            font-size: 12px;
        }
        .log-entry {
            padding: 3px 0;
            border-bottom: 1px solid #2d3748;
        }
        .alert {
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 15px;
        }
        .alert-info {
            background: #bee3f8;
            color: #2c5282;
        }
        .alert-warning {
            background: #feebc8;
            color: #7c2d12;
        }
        .spinner {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #667eea;
            border-radius: 50%;
            width: 30px;
            height: 30px;
            animation: spin 1s linear infinite;
            display: inline-block;
            margin-left: 10px;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>TideClock Control Panel</h1>
            <p>Kinetic Art Installation - Tide Prediction Display</p>
        </div>

        <div class="status-bar">
            <div class="status-item">
                <strong>System State:</strong>
                <span class="status-badge" id="systemState">READY</span>
            </div>
            <div class="status-item">
                <strong>WiFi:</strong>
                <span id="wifiStatus">Connecting...</span>
            </div>
            <div class="status-item">
                <strong>Uptime:</strong>
                <span id="uptime">0s</span>
            </div>
        </div>

        <div class="tabs">
            <button class="tab active" onclick="switchTab(0)">Main Control</button>
            <button class="tab" onclick="switchTab(1)">Advanced</button>
            <button class="tab" onclick="switchTab(2)">Configuration</button>
            <button class="tab" onclick="switchTab(3)">Status & Logs</button>
        </div>

        <div class="content">
            <!-- TAB 1: MAIN CONTROL -->
            <div class="tab-panel active" id="tab0">
                <div class="card">
                    <h3>System Control</h3>
                    <button class="btn btn-success" onclick="homeAllMotors()">Reset to Home</button>
                    <button class="btn btn-primary" onclick="fetchAndRun()" disabled title="Coming in Phase 3">Fetch & Run Now</button>
                    <button class="btn btn-danger" onclick="emergencyStop()">EMERGENCY STOP</button>
                    <button class="btn btn-secondary" onclick="clearEmergencyStop()">Clear E-Stop</button>
                </div>

                <div class="card">
                    <h3>Tide Data (24-Hour Forecast)</h3>
                    <div class="alert alert-info">
                        <strong>Phase 3 Feature:</strong> NOAA tide data integration coming soon. This table will display hourly tide predictions and motor positioning times.
                    </div>
                    <table>
                        <thead>
                            <tr>
                                <th>Hour</th>
                                <th>Raw Tide (ft)</th>
                                <th>Motor Time (ms)</th>
                                <th>Offset</th>
                                <th>Final Time (ms)</th>
                            </tr>
                        </thead>
                        <tbody id="tideDataTable">
                            <tr>
                                <td colspan="5" style="text-align: center; padding: 30px;">
                                    No tide data available. Click "Fetch & Run Now" to retrieve data.
                                </td>
                            </tr>
                        </tbody>
                    </table>
                </div>
            </div>

            <!-- TAB 2: ADVANCED (MOTOR TESTING) -->
            <div class="tab-panel" id="tab1">
                <div class="card">
                    <h3>Motor Testing</h3>
                    <div class="form-row">
                        <div class="form-group">
                            <label>Motor Selection (0-23)</label>
                            <select id="testMotor">
                                <option value="0">Motor 0</option>
                                <option value="1">Motor 1</option>
                                <option value="2">Motor 2</option>
                                <option value="3">Motor 3</option>
                                <option value="4">Motor 4</option>
                                <option value="5">Motor 5</option>
                                <option value="6">Motor 6</option>
                                <option value="7">Motor 7</option>
                                <option value="8">Motor 8</option>
                                <option value="9">Motor 9</option>
                                <option value="10">Motor 10</option>
                                <option value="11">Motor 11</option>
                                <option value="12">Motor 12</option>
                                <option value="13">Motor 13</option>
                                <option value="14">Motor 14</option>
                                <option value="15">Motor 15</option>
                                <option value="16">Motor 16</option>
                                <option value="17">Motor 17</option>
                                <option value="18">Motor 18</option>
                                <option value="19">Motor 19</option>
                                <option value="20">Motor 20</option>
                                <option value="21">Motor 21</option>
                                <option value="22">Motor 22</option>
                                <option value="23">Motor 23</option>
                            </select>
                        </div>
                        <div class="form-group">
                            <label>Run Time (ms)</label>
                            <input type="number" id="testDuration" value="1000" min="0" max="9000">
                        </div>
                    </div>
                    <button class="btn btn-primary" onclick="testMotor('forward')">Run Forward</button>
                    <button class="btn btn-warning" onclick="testMotor('reverse')">Run Reverse</button>
                    <button class="btn btn-secondary" onclick="testMotor('stop')">Stop</button>
                </div>

                <div class="card">
                    <h3>Limit Switch Monitor</h3>
                    <button class="btn btn-secondary" onclick="refreshSwitches()">Refresh Switches</button>
                    <div class="switch-grid" id="switchGrid">
                        <!-- Populated by JavaScript -->
                    </div>
                </div>

                <div class="card">
                    <h3>Motor Offset Calibration</h3>
                    <div class="alert alert-info">
                        <strong>Phase 5 Feature:</strong> Motor offset calibration (per-motor multipliers 0.8-1.2) will be available in a future update.
                    </div>
                </div>
            </div>

            <!-- TAB 3: CONFIGURATION -->
            <div class="tab-panel" id="tab2">
                <div class="card">
                    <h3>WiFi Configuration</h3>
                    <div class="form-group">
                        <label>WiFi SSID</label>
                        <input type="text" id="wifiSSID" placeholder="Your WiFi network name">
                    </div>
                    <div class="form-group">
                        <label>WiFi Password</label>
                        <input type="password" id="wifiPassword" placeholder="Your WiFi password">
                    </div>
                    <div class="alert alert-warning">
                        <strong>Note:</strong> After saving WiFi credentials, restart the device to connect to your network.
                    </div>
                </div>

                <div class="card">
                    <h3>Motor Behavior Settings</h3>
                    <div class="form-row">
                        <div class="form-group">
                            <label>Switch Release Time (100-500ms)</label>
                            <input type="number" id="switchRelease" value="200" min="100" max="500">
                        </div>
                        <div class="form-group">
                            <label>Max Run Time (1000-9000ms)</label>
                            <input type="number" id="maxRunTime" value="9000" min="1000" max="9000">
                        </div>
                    </div>
                </div>

                <div class="card">
                    <h3>NOAA Configuration</h3>
                    <div class="alert alert-info">
                        <strong>Phase 3 Feature:</strong> NOAA station configuration (Station ID, tide range) coming soon.
                    </div>
                </div>

                <div class="card">
                    <h3>Automation Settings</h3>
                    <div class="alert alert-info">
                        <strong>Phase 4 Feature:</strong> Automatic scheduling and fetch timing will be available in a future update.
                    </div>
                </div>

                <button class="btn btn-success" onclick="saveConfiguration()">Save Configuration</button>
            </div>

            <!-- TAB 4: STATUS & LOGS -->
            <div class="tab-panel" id="tab3">
                <div class="card">
                    <h3>WiFi Information</h3>
                    <table>
                        <tr><td><strong>Mode:</strong></td><td id="wifiMode">-</td></tr>
                        <tr><td><strong>SSID:</strong></td><td id="wifiSSID_display">-</td></tr>
                        <tr><td><strong>IP Address:</strong></td><td id="wifiIP">-</td></tr>
                        <tr><td><strong>Signal Strength:</strong></td><td id="wifiRSSI">-</td></tr>
                    </table>
                </div>

                <div class="card">
                    <h3>Hardware Status</h3>
                    <table>
                        <tr><td><strong>Free Heap:</strong></td><td id="freeHeap">-</td></tr>
                        <tr><td><strong>I2C Devices:</strong></td><td>5 MCP23017 boards detected</td></tr>
                        <tr><td><strong>Emergency Stop:</strong></td><td id="emergencyStopStatus">-</td></tr>
                    </table>
                </div>

                <div class="card">
                    <h3>Data Status</h3>
                    <div class="alert alert-info">
                        <strong>Phase 3 Feature:</strong> Tide data status (last fetch, next fetch, validity) will display here.
                    </div>
                </div>

                <div class="card">
                    <h3>System Log</h3>
                    <div class="log-viewer" id="logViewer">
                        <div class="log-entry">System initialized - Web interface active</div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script>
        // Global state
        let currentTab = 0;
        let statusInterval = null;

        // Initialize
        document.addEventListener('DOMContentLoaded', function() {
            refreshStatus();
            refreshSwitches();
            loadConfiguration();
            startAutoRefresh();
        });

        // Tab switching
        function switchTab(index) {
            document.querySelectorAll('.tab').forEach((tab, i) => {
                tab.classList.toggle('active', i === index);
            });
            document.querySelectorAll('.tab-panel').forEach((panel, i) => {
                panel.classList.toggle('active', i === index);
            });
            currentTab = index;
        }

        // Auto-refresh status
        function startAutoRefresh() {
            statusInterval = setInterval(refreshStatus, 500);
        }

        // Refresh system status
        async function refreshStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();

                // Update status bar
                document.getElementById('systemState').textContent = data.state;
                document.getElementById('systemState').className = 'status-badge badge-' + data.state.toLowerCase().replace('_', '');
                document.getElementById('uptime').textContent = formatUptime(data.uptime);
                document.getElementById('wifiStatus').textContent = data.wifi.ssid + ' (' + data.wifi.ip + ')';

                // Update status tab
                document.getElementById('wifiMode').textContent = data.wifi.mode;
                document.getElementById('wifiSSID_display').textContent = data.wifi.ssid;
                document.getElementById('wifiIP').textContent = data.wifi.ip;
                document.getElementById('wifiRSSI').textContent = data.wifi.rssi ? data.wifi.rssi + ' dBm' : 'N/A';
                document.getElementById('freeHeap').textContent = formatBytes(data.freeHeap);
                document.getElementById('emergencyStopStatus').textContent = data.motor.emergencyStop ? 'ACTIVE' : 'Clear';

            } catch (error) {
                console.error('Status refresh failed:', error);
            }
        }

        // Refresh switch states
        async function refreshSwitches() {
            try {
                const response = await fetch('/api/switches');
                const data = await response.json();

                const grid = document.getElementById('switchGrid');
                grid.innerHTML = '';

                data.switches.forEach(sw => {
                    const div = document.createElement('div');
                    div.className = 'switch-item' + (sw.triggered ? ' triggered' : '');
                    div.innerHTML = '<strong>SW ' + sw.id + '</strong><br>' + (sw.triggered ? 'CLOSED' : 'Open');
                    grid.appendChild(div);
                });

            } catch (error) {
                console.error('Switch refresh failed:', error);
            }
        }

        // Load configuration values into form fields
        async function loadConfiguration() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();

                // Load config values into form
                document.getElementById('switchRelease').value = data.config.switchRelease;
                document.getElementById('maxRunTime').value = data.config.maxRunTime;

            } catch (error) {
                console.error('Config load failed:', error);
            }
        }

        // API calls
        async function homeAllMotors() {
            if (!confirm('Home all motors? This will reset all positions.')) return;

            try {
                const response = await fetch('/api/home', { method: 'POST' });
                const data = await response.json();
                alert(data.message || data.error);
            } catch (error) {
                alert('Homing failed: ' + error);
            }
        }

        async function emergencyStop() {
            try {
                const response = await fetch('/api/emergency-stop', { method: 'POST' });
                const data = await response.json();
                alert(data.message);
            } catch (error) {
                alert('Emergency stop failed: ' + error);
            }
        }

        async function clearEmergencyStop() {
            try {
                const response = await fetch('/api/clear-stop', { method: 'POST' });
                const data = await response.json();
                alert(data.message || data.error);
            } catch (error) {
                alert('Clear failed: ' + error);
            }
        }

        async function testMotor(action) {
            const motor = parseInt(document.getElementById('testMotor').value);
            const duration = parseInt(document.getElementById('testDuration').value);

            try {
                const response = await fetch('/api/test-motor', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ motor, action, duration })
                });
                const data = await response.json();
                if (!data.success) alert(data.error);
            } catch (error) {
                alert('Motor test failed: ' + error);
            }
        }

        async function saveConfiguration() {
            const config = {
                wifiSSID: document.getElementById('wifiSSID').value,
                wifiPassword: document.getElementById('wifiPassword').value,
                switchRelease: parseInt(document.getElementById('switchRelease').value),
                maxRunTime: parseInt(document.getElementById('maxRunTime').value)
            };

            try {
                const response = await fetch('/api/save-config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                });
                const data = await response.json();
                alert(data.message || data.error);

                // Reload config to confirm saved values
                if (data.success) {
                    loadConfiguration();
                }
            } catch (error) {
                alert('Save failed: ' + error);
            }
        }

        async function fetchAndRun() {
            alert('NOAA integration coming in Phase 3');
        }

        // Utility functions
        function formatUptime(seconds) {
            const h = Math.floor(seconds / 3600);
            const m = Math.floor((seconds % 3600) / 60);
            const s = seconds % 60;
            return h + 'h ' + m + 'm ' + s + 's';
        }

        function formatBytes(bytes) {
            return (bytes / 1024).toFixed(1) + ' KB';
        }
    </script>
</body>
</html>
)=====";
}

#endif // WEB_UI_H
