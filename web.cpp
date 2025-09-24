
// web.cpp
#include "web.h"
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "sensors.h"
#include "config.h"
#include "thingspeak.h"

ESP8266WebServer server(80);

// Глобальные переменные для настроек
float tempOffset = DEFAULT_TEMP_OFFSET;
float humOffset = DEFAULT_HUM_OFFSET;
int co2Offset = DEFAULT_CO2_OFFSET;
bool useThingSpeak = DEFAULT_USE_THINGSPEAK;
String thingSpeakApiKey = DEFAULT_THINGSPEAK_API_KEY;
unsigned long thingSpeakChannel = DEFAULT_THINGSPEAK_CHANNEL;
bool thingSpeakStatus = false; // Статус синхронизации с ThingSpeak

// Загрузка настроек из SPIFFS при старте
void loadSettings() {
  Serial.println("Attempting to load settings from /settings.json...");
  File file = SPIFFS.open("/settings.json", "r");
  if (!file) {
    Serial.println("❌ No settings file found, using default settings");
    return;
  }

  String json = file.readString();
  file.close();
  if (json.length() == 0) {
    Serial.println("❌ Settings file is empty, using default settings");
    return;
  }
  Serial.println("Loaded settings: " + json);

  // Парсинг JSON с помощью ArduinoJson
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("❌ Failed to parse settings JSON: " + String(error.c_str()));
    return;
  }

  tempOffset = doc["temp"] | DEFAULT_TEMP_OFFSET;
  Serial.println("Parsed tempOffset: " + String(tempOffset, 1));

  humOffset = doc["hum"] | DEFAULT_HUM_OFFSET;
  Serial.println("Parsed humOffset: " + String(humOffset, 1));

  co2Offset = doc["co2"] | DEFAULT_CO2_OFFSET;
  Serial.println("Parsed co2Offset: " + String(co2Offset));

  useThingSpeak = doc["useThingSpeak"] | DEFAULT_USE_THINGSPEAK;
  Serial.println("Parsed useThingSpeak: " + String(useThingSpeak));

  thingSpeakApiKey = doc["thingSpeakApiKey"] | DEFAULT_THINGSPEAK_API_KEY;
  Serial.println("Parsed thingSpeakApiKey: " + thingSpeakApiKey);

  thingSpeakChannel = doc["thingSpeakChannel"] | DEFAULT_THINGSPEAK_CHANNEL;
  Serial.println("Parsed thingSpeakChannel: " + String(thingSpeakChannel));

  if (useThingSpeak && thingSpeakApiKey != "" && thingSpeakChannel != 0) {
    Serial.println("Settings loaded, initializing ThingSpeak...");
    initThingSpeak();
  } else {
    Serial.println("Settings loaded, but ThingSpeak not initialized due to invalid settings");
  }
}

// Сохранение настроек в SPIFFS
void saveSettings() {
  Serial.println("Saving settings to /settings.json...");
  File file = SPIFFS.open("/settings.json", "w");
  if (!file) {
    Serial.println("❌ Failed to open settings file for writing");
    return;
  }

  String json = "{";
  json += "\"temp\":" + String(tempOffset, 1) + ",";
  json += "\"hum\":" + String(humOffset, 1) + ",";
  json += "\"co2\":" + String(co2Offset) + ",";
  json += "\"useThingSpeak\":" + String(useThingSpeak ? "true" : "false") + ",";
  json += "\"thingSpeakApiKey\":\"" + thingSpeakApiKey + "\",";
  json += "\"thingSpeakChannel\":" + String(thingSpeakChannel);
  json += "}";

  file.print(json);
  file.close();
  Serial.println("✅ Settings saved: " + json);
}

void handleSettings() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Настройки CO₂ Монитора</title>
  <style>
    body { font-family: Arial; padding: 20px; }
    input, button { padding: 10px; margin: 5px; font-size: 16px; }
    .section { margin: 20px 0; padding: 15px; border: 1px solid #ccc; border-radius: 5px; }
    h2 { margin-top: 0; }
    p { margin: 5px 0; }
  </style>
</head>
<body>
  <h1>⚙️ Настройки CO₂ Монитора</h1>
  <form action="/save-settings" method="POST">
    <div class="section">
      <h2>Калибровка</h2>
      <label>Температура (°C):</label><br>
      <input type="number" step="0.1" name="temp" value=")rawliteral" + String(tempOffset, 1) + R"rawliteral("><br><br>
      <label>Влажность (%):</label><br>
      <input type="number" step="0.1" name="hum" value=")rawliteral" + String(humOffset, 1) + R"rawliteral("><br><br>
      <label>CO₂ (ppm):</label><br>
      <input type="number" name="co2" value=")rawliteral" + String(co2Offset) + R"rawliteral("><br><br>
    </div>
    <div class="section">
      <h2>ThingSpeak</h2>
      <label><input type="checkbox" name="useThingSpeak" )rawliteral" + String(useThingSpeak ? "checked" : "") + R"rawliteral("> Включить синхронизацию</label><br>
      <p>Текущий ключ: )rawliteral" + thingSpeakApiKey + R"rawliteral(</p>
      <label>API Key:</label><br>
      <input type="text" name="thingSpeakApiKey"><br><br>
      <p>Текущий канал: )rawliteral" + String(thingSpeakChannel) + R"rawliteral(</p>
      <label>Channel ID:</label><br>
      <input type="number" name="thingSpeakChannel"><br><br>
    </div>
    <button type="submit">💾 Сохранить</button>
    <button type="button" onclick="location.href='/'">⬅️ Назад</button>
  </form>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleSaveSettings() {
  if (server.hasArg("temp")) tempOffset = server.arg("temp").toFloat();
  if (server.hasArg("hum")) humOffset = server.arg("hum").toFloat();
  if (server.hasArg("co2")) co2Offset = server.arg("co2").toInt();
  useThingSpeak = server.hasArg("useThingSpeak");
  if (server.hasArg("thingSpeakApiKey") && server.arg("thingSpeakApiKey") != "") thingSpeakApiKey = server.arg("thingSpeakApiKey");
  if (server.hasArg("thingSpeakChannel") && server.arg("thingSpeakChannel") != "") thingSpeakChannel = server.arg("thingSpeakChannel").toInt();

  saveSettings();

  server.send(200, "text/html", R"rawliteral(
    <h2>✅ Настройки сохранены!</h2>
    <p>Перезагрузите устройство для применения изменений.</p>
    <a href="/settings">⬅️ Назад к настройкам</a>
  )rawliteral");
}

void handleApplySettings() {
  if (server.hasArg("temp") || server.hasArg("hum") || server.hasArg("co2") || 
      server.hasArg("useThingSpeak") || server.hasArg("thingSpeakApiKey") || server.hasArg("thingSpeakChannel")) {
    Serial.println("Received settings via POST:");
    if (server.hasArg("temp")) {
      tempOffset = server.arg("temp").toFloat();
      Serial.println("Parsed tempOffset: " + String(tempOffset, 1));
    }
    if (server.hasArg("hum")) {
      humOffset = server.arg("hum").toFloat();
      Serial.println("Parsed humOffset: " + String(humOffset, 1));
    }
    if (server.hasArg("co2")) {
      co2Offset = server.arg("co2").toInt();
      Serial.println("Parsed co2Offset: " + String(co2Offset));
    }
    useThingSpeak = server.hasArg("useThingSpeak") && server.arg("useThingSpeak") == "true";
    Serial.println("Parsed useThingSpeak: " + String(useThingSpeak));
    if (server.hasArg("thingSpeakApiKey") && server.arg("thingSpeakApiKey") != "") {
      thingSpeakApiKey = server.arg("thingSpeakApiKey");
      Serial.println("Parsed thingSpeakApiKey: " + thingSpeakApiKey);
    }
    if (server.hasArg("thingSpeakChannel") && server.arg("thingSpeakChannel") != "") {
      thingSpeakChannel = server.arg("thingSpeakChannel").toInt();
      Serial.println("Parsed thingSpeakChannel: " + String(thingSpeakChannel));
    }

    saveSettings();
    if (useThingSpeak && thingSpeakApiKey != "" && thingSpeakChannel != 0) {
      Serial.println("Initializing ThingSpeak...");
      initThingSpeak();
    }
    
    server.send(200, "text/plain", "OK");
  } else {
    Serial.println("Bad Request: No arguments provided");
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleJson() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot serve /json");
    server.send(503, "text/plain", "Service Unavailable");
    return;
  }
  String json = "{";
  json += "\"co2\":" + String(co2) + ",";
  json += "\"temp\":" + String(temperature, 1) + ",";
  json += "\"hum\":" + String(humidity, 0);
  json += "}";
  Serial.println("Sending /json: " + json);
  server.send(200, "application/json", json);
}

void handleReinitThingSpeak() {
  if (useThingSpeak && thingSpeakApiKey != "" && thingSpeakChannel != 0) {
    Serial.println("Reinitializing ThingSpeak...");
    initThingSpeak();
    server.send(200, "text/plain", "ThingSpeak reinitialized");
  } else {
    Serial.println("Cannot reinitialize ThingSpeak: invalid settings");
    server.send(400, "text/plain", "Invalid settings");
  }
}

void handleRoot() {
  // Экранирование thingSpeakApiKey для безопасной вставки в JavaScript
  String escapedApiKey = thingSpeakApiKey;
  escapedApiKey.replace("'", "\\'");
  escapedApiKey.replace("\"", "\\\"");
  escapedApiKey.replace("\n", "\\n");
  escapedApiKey.replace("\r", "\\r");

  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CO₂ Monitor</title>
  <style>
    body { font-family: Arial, sans-serif; padding: 15px; text-align: center; background: #f9f9f9; position: relative; }
    .data-box { margin: 20px 0; padding: 15px; background: #e8f5e9; border-radius: 8px; font-size: 18px; }
    h2 { color: #2c3e50; margin-bottom: 10px; }
    button { margin: 10px 5px; padding: 10px 20px; font-size: 16px; background: #3498db; color: white; border: none; border-radius: 5px; cursor: pointer; }
    button:hover { background: #2980b9; }
    .footer { margin-top: 20px; font-size: 12px; color: #7f8c8d; }
    .settings-popup {
      display: none;
      position: fixed;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 4px 8px rgba(0,0,0,0.2);
      z-index: 1000;
      width: 90%;
      max-width: 500px;
    }
    .settings-popup h3 { margin-top: 0; }
    .settings-popup input {
      width: 100%;
      padding: 8px;
      margin: 5px 0 15px 0;
      box-sizing: border-box;
    }
    .settings-popup button {
      padding: 10px 15px;
      margin: 5px;
      cursor: pointer;
    }
    .settings-overlay {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0,0,0,0.5);
      z-index: 999;
    }
    .settings-icon {
      position: absolute;
      top: 15px;
      right: 15px;
      font-size: 24px;
      color: #3498db;
      cursor: pointer;
      text-decoration: none;
    }
    .settings-icon:hover { color: #2980b9; }
    .thingspeak-status { display: inline-block; width: 10px; height: 10px; border-radius: 50%; margin-right: 5px; }
    p { margin: 5px 0; }
  </style>
</head>
<body>
  <h2>🌿 CO₂ Monitor</h2>
  <span class="thingspeak-status" style="background-color: )rawliteral" + String(thingSpeakStatus ? "green" : "red") + R"rawliteral("></span> ThingSpeak
  <a class="settings-icon" onclick="openSettings()">⚙️</a>
  <div class="settings-overlay" onclick="closeSettings()"></div>
  <div class="settings-popup" id="settingsPopup">
    <h3>⚙️ Настройки</h3>
    <label>Температура (°C):</label>
    <input type="number" step="0.1" id="tempOffset" value=")rawliteral" + String(tempOffset, 1) + R"rawliteral("><br>
    <label>Влажность (%):</label>
    <input type="number" step="0.1" id="humOffset" value=")rawliteral" + String(humOffset, 1) + R"rawliteral("><br>
    <label>CO₂ (ppm):</label>
    <input type="number" id="co2Offset" value=")rawliteral" + String(co2Offset) + R"rawliteral("><br>
    <label><input type="checkbox" id="useThingSpeak" )rawliteral" + String(useThingSpeak ? "checked" : "") + R"rawliteral("> Включить ThingSpeak</label><br>
    <p>Текущий ключ: )rawliteral" + thingSpeakApiKey + R"rawliteral(</p>
    <label>API Key:</label>
    <input type="text" id="thingSpeakApiKey"><br>
    <p>Текущий канал: )rawliteral" + String(thingSpeakChannel) + R"rawliteral(</p>
    <label>Channel ID:</label>
    <input type="number" id="thingSpeakChannel"><br>
    <button onclick="saveSettingsAjax()">💾 Сохранить</button>
    <button onclick="closeSettings()">❌ Закрыть</button>
  </div>
  <div class="data-box">
    CO₂: <b><span id="co2">--</span> ppm</b> |
    Temp: <b><span id="temp">--</span> °C</b> |
    Hum: <b><span id="hum">--</span> %</b>
  </div>
  <button onclick="location.reload()">🔄 Обновить</button>
  <div class="footer">
    Данные обновляются каждые 10 секунд. Последнее обновление: <span id="lastUpdate">--</span>
  </div>
  <script>
    var thingSpeakStatus = )rawliteral" + (thingSpeakStatus ? "true" : "false") + R"rawliteral(;
    function openSettings() {
      try {
        document.getElementById('tempOffset').value = ')rawliteral" + String(tempOffset, 1) + R"rawliteral(';
        document.getElementById('humOffset').value = ')rawliteral" + String(humOffset, 1) + R"rawliteral(';
        document.getElementById('co2Offset').value = ')rawliteral" + String(co2Offset) + R"rawliteral(';
        document.getElementById('useThingSpeak').checked = )rawliteral" + (useThingSpeak ? "true" : "false") + R"rawliteral(;
        document.getElementById('thingSpeakApiKey').value = '';
        document.getElementById('thingSpeakChannel').value = '';
        console.log('Settings popup opened, API Key: ")rawliteral" + escapedApiKey + R"rawliteral("');
        document.querySelector('.settings-overlay').style.display = 'block';
        document.getElementById('settingsPopup').style.display = 'block';
        console.log('Settings popup opened successfully');
      } catch (e) {
        console.error('Error opening settings popup:', e);
        alert('❌ Ошибка открытия настроек');
      }
    }

    function closeSettings() {
      try {
        document.querySelector('.settings-overlay').style.display = 'none';
        document.getElementById('settingsPopup').style.display = 'none';
        console.log('Settings popup closed');
      } catch (e) {
        console.error('Error closing settings popup:', e);
      }
    }

    function updateCurrentValues() {
      console.log('Fetching /json...');
      fetch('/json')
        .then(r => {
          if (!r.ok) {
            console.error('Failed to fetch /json, status:', r.status);
            thingSpeakStatus = false;
            document.querySelector('.thingspeak-status').style.backgroundColor = 'red';
            throw new Error('Failed to fetch /json');
          }
          return r.json();
        })
        .then(d => {
          console.log('Received /json data:', d);
          document.getElementById('co2').innerText = d.co2 !== undefined ? d.co2 : '--';
          document.getElementById('temp').innerText = d.temp !== undefined ? d.temp : '--';
          document.getElementById('hum').innerText = d.hum !== undefined ? d.hum : '--';
          document.getElementById('lastUpdate').innerText = new Date().toLocaleTimeString('ru-RU');
        })
        .catch(error => {
          console.error('Ошибка загрузки текущих данных:', error);
          document.getElementById('co2').innerText = '--';
          document.getElementById('temp').innerText = '--';
          document.getElementById('hum').innerText = '--';
          document.getElementById('lastUpdate').innerText = 'Ошибка';
          thingSpeakStatus = false;
          document.querySelector('.thingspeak-status').style.backgroundColor = 'red';
        });
    }

    function saveSettingsAjax() {
      try {
        const temp = parseFloat(document.getElementById('tempOffset').value) || 0;
        const hum = parseFloat(document.getElementById('humOffset').value) || 0;
        const co2 = parseInt(document.getElementById('co2Offset').value) || 0;
        const useThingSpeak = document.getElementById('useThingSpeak').checked;
        const thingSpeakApiKey = document.getElementById('thingSpeakApiKey').value || '';
        const thingSpeakChannel = parseInt(document.getElementById('thingSpeakChannel').value) || 0;
        console.log('Saving settings:', { temp, hum, co2, useThingSpeak, thingSpeakApiKey, thingSpeakChannel });
        fetch('/apply-settings', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'temp=' + encodeURIComponent(temp) + '&hum=' + encodeURIComponent(hum) + 
                '&co2=' + encodeURIComponent(co2) + '&useThingSpeak=' + encodeURIComponent(useThingSpeak) + 
                '&thingSpeakApiKey=' + encodeURIComponent(thingSpeakApiKey) + 
                '&thingSpeakChannel=' + encodeURIComponent(thingSpeakChannel)
        })
        .then(response => {
          if (response.ok) {
            alert('✅ Настройки сохранены и применены!');
            closeSettings();
            updateCurrentValues();
            if (useThingSpeak && thingSpeakApiKey && thingSpeakChannel) {
              console.log('ThingSpeak enabled, reinitializing...');
              fetch('/reinit-thingspeak', { method: 'POST' });
            }
          } else {
            alert('❌ Ошибка применения настроек');
            console.error('Failed to apply settings, status:', response.status);
            thingSpeakStatus = false;
            document.querySelector('.thingspeak-status').style.backgroundColor = 'red';
          }
        })
        .catch(error => {
          console.error('Network error:', error);
          alert('❌ Ошибка сети');
          thingSpeakStatus = false;
          document.querySelector('.thingspeak-status').style.backgroundColor = 'red';
        });
      } catch (e) {
        console.error('Error saving settings:', e);
        alert('❌ Ошибка при сохранении настроек');
        thingSpeakStatus = false;
        document.querySelector('.thingspeak-status').style.backgroundColor = 'red';
      }
    }

    // Инициализация цвета индикатора
    document.querySelector('.thingspeak-status').style.backgroundColor = thingSpeakStatus ? 'green' : 'red';
    console.log('Script loaded, starting initial data fetch');
    updateCurrentValues();
    setInterval(updateCurrentValues, 10000);
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void startWebServer() {
  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.on("/settings", handleSettings);
  server.on("/save-settings", handleSaveSettings);
  server.on("/apply-settings", HTTP_POST, handleApplySettings);
  server.on("/reinit-thingspeak", HTTP_POST, handleReinitThingSpeak);
  server.begin();
  Serial.println("HTTP server started");
}

void handleWebRequests() {
  server.handleClient();
}