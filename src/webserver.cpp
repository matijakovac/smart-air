#include "webserver.h"

WebServerManager::WebServerManager(SensorManager& sensors, FanController& fan)
  : server(80), sensorManager(sensors), fanController(fan) {
}

void WebServerManager::begin() {
  setupRoutes();
  server.begin();
  Serial.println("✓ Web server started on port 80");
  Serial.printf("   Access: http://%s.local or http://%s\n", 
                config.hostname.c_str(), WiFi.localIP().toString().c_str());
}

void WebServerManager::setupRoutes() {
  // Main page
  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
    request->send(200, "text/html", getMainHTML());
  });
  
  // API: Get status
  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request){
    request->send(200, "application/json", getStatusJSON());
  });
  
  // API: Get config
  server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request){
    request->send(200, "application/json", getConfigJSON());
  });
  
  // API: Set mode
  server.on("/api/mode", HTTP_POST, [this](AsyncWebServerRequest *request){
    handleSetMode(request);
  });
  
  // API: Set config
  server.on("/api/config", HTTP_POST, [this](AsyncWebServerRequest *request){
    handleSetConfig(request);
  });
  
  // API: Manual speed control
  server.on("/api/speed", HTTP_POST, [this](AsyncWebServerRequest *request){
    if (request->hasParam("value", true)) {
      int speed = request->getParam("value", true)->value().toInt();
      fanController.setManualSpeed(speed);
      request->send(200, "application/json", "{\"success\":true}");
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing value parameter\"}");
    }
  });
  
  // OTA Upload
  server.on("/update", HTTP_POST, 
    [](AsyncWebServerRequest *request){
      bool shouldReboot = !Update.hasError();
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", 
        shouldReboot ? "OK - Rebooting..." : "FAIL");
      response->addHeader("Connection", "close");
      request->send(response);
      if (shouldReboot) {
        delay(1000);
        ESP.restart();
      }
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      handleOTAUpload(request, filename, index, data, len, final);
    }
  );
  
  // 404 handler
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });
}

String WebServerManager::getMainHTML() const {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Cellar Ventilation Control</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { 
      font-family: Arial, sans-serif; 
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
    }
    .container { 
      max-width: 800px; 
      margin: 0 auto; 
      background: white;
      border-radius: 15px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.2);
      overflow: hidden;
    }
    .header {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 30px;
      text-align: center;
    }
    .header h1 { font-size: 28px; margin-bottom: 5px; }
    .header p { opacity: 0.9; font-size: 14px; }
    .content { padding: 20px; }
    .card {
      background: #f8f9fa;
      border-radius: 10px;
      padding: 20px;
      margin-bottom: 20px;
      border-left: 4px solid #667eea;
    }
    .card h2 {
      font-size: 18px;
      margin-bottom: 15px;
      color: #333;
    }
    .sensor-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 15px;
      margin-top: 10px;
    }
    .sensor-item {
      background: white;
      padding: 15px;
      border-radius: 8px;
      text-align: center;
    }
    .sensor-label {
      font-size: 12px;
      color: #666;
      text-transform: uppercase;
      margin-bottom: 5px;
    }
    .sensor-value {
      font-size: 24px;
      font-weight: bold;
      color: #667eea;
    }
    .sensor-unit {
      font-size: 14px;
      color: #999;
    }
    .fan-status {
      display: flex;
      align-items: center;
      justify-content: space-between;
      background: white;
      padding: 20px;
      border-radius: 8px;
      margin-top: 10px;
    }
    .fan-status.on { border-left: 4px solid #28a745; }
    .fan-status.off { border-left: 4px solid #dc3545; }
    .status-indicator {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      margin-right: 10px;
    }
    .status-on { background: #28a745; animation: pulse 2s infinite; }
    .status-off { background: #dc3545; }
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.5; }
    }
    .controls {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 10px;
      margin-top: 15px;
    }
    button {
      padding: 12px 20px;
      border: none;
      border-radius: 8px;
      font-size: 14px;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s;
    }
    .btn-primary {
      background: #667eea;
      color: white;
    }
    .btn-primary:hover {
      background: #5568d3;
      transform: translateY(-2px);
    }
    .btn-success {
      background: #28a745;
      color: white;
    }
    .btn-warning {
      background: #ffc107;
      color: #333;
    }
    .btn-danger {
      background: #dc3545;
      color: white;
    }
    button:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
    .footer {
      text-align: center;
      padding: 20px;
      color: #666;
      font-size: 12px;
    }
    .update-time {
      text-align: right;
      color: #999;
      font-size: 12px;
      margin-top: 10px;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🌡️ Cellar Ventilation Control</h1>
      <p>Smart Climate Management System</p>
    </div>
    
    <div class="content">
      <!-- Sensor Data Card -->
      <div class="card">
        <h2>📊 Environmental Conditions</h2>
        <div class="sensor-grid">
          <div class="sensor-item">
            <div class="sensor-label">Internal Temp</div>
            <div class="sensor-value" id="temp-in">--</div>
            <div class="sensor-unit">°C</div>
          </div>
          <div class="sensor-item">
            <div class="sensor-label">Internal Humidity</div>
            <div class="sensor-value" id="humid-in">--</div>
            <div class="sensor-unit">%</div>
          </div>
          <div class="sensor-item">
            <div class="sensor-label">External Temp</div>
            <div class="sensor-value" id="temp-out">--</div>
            <div class="sensor-unit">°C</div>
          </div>
          <div class="sensor-item">
            <div class="sensor-label">External Humidity</div>
            <div class="sensor-value" id="humid-out">--</div>
            <div class="sensor-unit">%</div>
          </div>
        </div>
        <div class="update-time">Last update: <span id="last-update">--</span></div>
      </div>
      
      <!-- Fan Status Card -->
      <div class="card">
        <h2>💨 Fan Status</h2>
        <div class="fan-status" id="fan-status">
          <div style="display: flex; align-items: center;">
            <div class="status-indicator" id="status-led"></div>
            <div>
              <div style="font-weight: bold; font-size: 18px;" id="fan-state">--</div>
              <div style="color: #666; font-size: 14px;" id="fan-reason">--</div>
            </div>
          </div>
          <div style="font-size: 32px; font-weight: bold; color: #667eea;" id="fan-speed">--%</div>
        </div>
      </div>
      
      <!-- Control Card -->
      <div class="card">
        <h2>🎛️ Manual Control</h2>
        <div style="margin-bottom: 10px; color: #666; font-size: 14px;">
          Current Mode: <strong id="current-mode">AUTO</strong>
        </div>
        <div class="controls">
          <button class="btn-primary" onclick="setMode('auto')">AUTO</button>
          <button class="btn-danger" onclick="setMode('off')">OFF</button>
          <button class="btn-warning" onclick="setMode('low')">LOW (60%)</button>
          <button class="btn-success" onclick="setMode('high')">HIGH (100%)</button>
        </div>
      </div>
      
      <!-- Info Card -->
      <div class="card">
        <h2>ℹ️ System Information</h2>
        <div style="display: grid; gap: 10px; font-size: 14px;">
          <div style="display: flex; justify-content: space-between;">
            <span>WiFi Signal:</span>
            <strong id="wifi-signal">--</strong>
          </div>
          <div style="display: flex; justify-content: space-between;">
            <span>Uptime:</span>
            <strong id="uptime">--</strong>
          </div>
          <div style="display: flex; justify-content: space-between;">
            <span>Free Memory:</span>
            <strong id="free-mem">--</strong>
          </div>
        </div>
      </div>
    </div>
    
    <div class="footer">
      Smart Cellar Ventilation System v1.0.0
    </div>
  </div>

  <script>
    function updateData() {
      fetch('/api/status')
        .then(response => response.json())
        .then(data => {
          // Update sensors
          document.getElementById('temp-in').textContent = data.internal.temperature.toFixed(1);
          document.getElementById('humid-in').textContent = Math.round(data.internal.humidity);
          document.getElementById('temp-out').textContent = data.external.temperature.toFixed(1);
          document.getElementById('humid-out').textContent = Math.round(data.external.humidity);
          
          // Update fan status
          const fanStatus = document.getElementById('fan-status');
          const statusLed = document.getElementById('status-led');
          
          if (data.fan.speed > 0) {
            fanStatus.className = 'fan-status on';
            statusLed.className = 'status-indicator status-on';
            document.getElementById('fan-state').textContent = 'RUNNING';
          } else {
            fanStatus.className = 'fan-status off';
            statusLed.className = 'status-indicator status-off';
            document.getElementById('fan-state').textContent = 'STOPPED';
          }
          
          document.getElementById('fan-speed').textContent = data.fan.speed + '%';
          document.getElementById('fan-reason').textContent = data.fan.reason;
          document.getElementById('current-mode').textContent = data.mode;
          
          // Update system info
          document.getElementById('wifi-signal').textContent = data.wifi_rssi + ' dBm';
          document.getElementById('uptime').textContent = formatUptime(data.uptime);
          document.getElementById('free-mem').textContent = Math.round(data.free_heap / 1024) + ' KB';
          document.getElementById('last-update').textContent = new Date().toLocaleTimeString();
        })
        .catch(error => console.error('Error:', error));
    }
    
    function setMode(mode) {
      const formData = new FormData();
      formData.append('mode', mode);
      formData.append('duration', mode === 'auto' ? '0' : '60');
      
      fetch('/api/mode', {
        method: 'POST',
        body: formData
      })
      .then(response => response.json())
      .then(data => {
        if (data.success) {
          updateData();
        }
      })
      .catch(error => console.error('Error:', error));
    }
    
    function formatUptime(seconds) {
      const days = Math.floor(seconds / 86400);
      const hours = Math.floor((seconds % 86400) / 3600);
      const minutes = Math.floor((seconds % 3600) / 60);
      
      if (days > 0) return `${days}d ${hours}h`;
      if (hours > 0) return `${hours}h ${minutes}m`;
      return `${minutes}m`;
    }
    
    // Update every 3 seconds
    setInterval(updateData, 3000);
    updateData();
  </script>
</body>
</html>
  )rawliteral";
  
  return html;
}

String WebServerManager::getStatusJSON() const {
  JsonDocument doc;
  
  SensorData internal = sensorManager.getInternalData();
  SensorData external = sensorManager.getExternalData();
  
  doc["internal"]["temperature"] = internal.temperature;
  doc["internal"]["humidity"] = internal.humidity;
  doc["internal"]["pressure"] = internal.pressure;
  doc["internal"]["dewpoint"] = internal.dewPoint;
  doc["internal"]["valid"] = internal.valid;
  
  doc["external"]["temperature"] = external.temperature;
  doc["external"]["humidity"] = external.humidity;
  doc["external"]["pressure"] = external.pressure;
  doc["external"]["dewpoint"] = external.dewPoint;
  doc["external"]["valid"] = external.valid;
  
  doc["fan"]["speed"] = fanController.getCurrentSpeed();
  doc["fan"]["reason"] = fanController.getStatusText();
  
  const char* modeNames[] = {"AUTO", "MANUAL_OFF", "MANUAL_LOW", "MANUAL_HIGH", "DIAGNOSTIC"};
  doc["mode"] = modeNames[currentMode];
  
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  
  String output;
  serializeJson(doc, output);
  return output;
}

String WebServerManager::getConfigJSON() const {
  JsonDocument doc;
  
  doc["target_temp"] = config.target_temp;
  doc["target_humidity"] = config.target_humidity;
  doc["temp_differential"] = config.temp_differential;
  doc["humidity_differential"] = config.humidity_differential;
  doc["low_speed"] = config.low_speed;
  doc["high_speed"] = config.high_speed;
  
  String output;
  serializeJson(doc, output);
  return output;
}

void WebServerManager::handleSetMode(AsyncWebServerRequest *request) {
  if (!request->hasParam("mode", true)) {
    request->send(400, "application/json", "{\"error\":\"Missing mode parameter\"}");
    return;
  }
  
  String mode = request->getParam("mode", true)->value();
  unsigned long duration = 0;
  
  if (request->hasParam("duration", true)) {
    duration = request->getParam("duration", true)->value().toInt();
  }
  
  if (mode == "auto") {
    fanController.setMode(MODE_AUTO);
  } else if (mode == "off") {
    fanController.setMode(MODE_MANUAL_OFF, duration);
  } else if (mode == "low") {
    fanController.setMode(MODE_MANUAL_LOW, duration);
  } else if (mode == "high") {
    fanController.setMode(MODE_MANUAL_HIGH, duration);
  } else {
    request->send(400, "application/json", "{\"error\":\"Invalid mode\"}");
    return;
  }
  
  request->send(200, "application/json", "{\"success\":true}");
}

void WebServerManager::handleSetConfig(AsyncWebServerRequest *request) {
  // Handle config updates
  bool changed = false;
  
  if (request->hasParam("target_temp", true)) {
    config.target_temp = request->getParam("target_temp", true)->value().toFloat();
    changed = true;
  }
  
  if (request->hasParam("target_humidity", true)) {
    config.target_humidity = request->getParam("target_humidity", true)->value().toFloat();
    changed = true;
  }
  
  if (changed) {
    saveConfig();
    request->send(200, "application/json", "{\"success\":true}");
  } else {
    request->send(400, "application/json", "{\"error\":\"No parameters to update\"}");
  }
}

void WebServerManager::handleOTAUpload(AsyncWebServerRequest *request, String filename, 
                                       size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    Serial.printf("OTA Update Start: %s\n", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  }
  
  if (!Update.hasError()) {
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }
  }
  
  if (final) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\n", index + len);
    } else {
      Update.printError(Serial);
    }
  }
}