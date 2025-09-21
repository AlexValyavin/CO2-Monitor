// web.cpp
#include "web.h"
#include <ESP8266WebServer.h>
#include <FS.h> // для SPIFFS
#include "sensors.h"
#include "config.h"

ESP8266WebServer server(80);

// Глобальные переменные для настроек
float tempOffset = DEFAULT_TEMP_OFFSET;
float humOffset = DEFAULT_HUM_OFFSET;
int co2Offset = DEFAULT_CO2_OFFSET;
String googleScriptUrl = GOOGLE_SCRIPT_URL;

// Загрузка настроек из SPIFFS при старте
void loadSettings() {
  File file = SPIFFS.open("/settings.json", "r");
  if (!file) {
    Serial.println("No settings file found");
    return;
  }

  String json = file.readString();
  file.close();
  Serial.println("Loaded settings: " + json);

  // Парсим JSON (простой способ)
  int t1 = json.indexOf("\"temp\":");
  int t2 = json.indexOf(",", t1);
  if (t1 != -1 && t2 != -1) {
    tempOffset = json.substring(t1 + 7, t2).toFloat();
    Serial.println("Temp offset: " + String(tempOffset, 1));
  }

  int h1 = json.indexOf("\"hum\":");
  int h2 = json.indexOf(",", h1);
  if (h1 != -1 && h2 != -1) {
    humOffset = json.substring(h1 + 6, h2).toFloat();
    Serial.println("Hum offset: " + String(humOffset, 1));
  }

  int c1 = json.indexOf("\"co2\":");
  int c2 = json.indexOf("}", c1);
  if (c1 == -1) c2 = json.indexOf(",", c1); // если есть запятая
  if (c1 != -1 && c2 != -1) {
    co2Offset = json.substring(c1 + 6, c2).toInt();
    Serial.println("CO2 offset: " + String(co2Offset));
  }

  int u1 = json.indexOf("\"url\":\"");
  int u2 = json.indexOf("\"", u1 + 7);
  if (u1 != -1 && u2 != -1) {
    googleScriptUrl = json.substring(u1 + 7, u2);
    Serial.println("Google URL: " + googleScriptUrl);
  }
}

// Сохранение настроек в SPIFFS
void saveSettings() {
  File file = SPIFFS.open("/settings.json", "w");
  if (!file) {
    Serial.println("Failed to open settings file for writing");
    return;
  }

  String json = "{";
  json += "\"temp\":" + String(tempOffset, 1) + ",";
  json += "\"hum\":" + String(humOffset, 1) + ",";
  json += "\"co2\":" + String(co2Offset) + ",";
  json += "\"url\":\"" + googleScriptUrl + "\"";
  json += "}";

  file.print(json);
  file.close();
  Serial.println("Settings saved: " + json);
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
      <h2>Google Script</h2>
      <label>URL:</label><br>
      <input type="text" name="url" size="60" value=")rawliteral" + googleScriptUrl + R"rawliteral("><br><br>
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
  if (server.hasArg("url")) googleScriptUrl = server.arg("url");

  // Сохраняем настройки
  saveSettings();

  server.send(200, "text/html", R"rawliteral(
    <h2>✅ Настройки сохранены!</h2>
    <p>Перезагрузите устройство для применения изменений.</p>
    <a href="/settings">⬅️ Назад к настройкам</a>
  )rawliteral");
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CO₂ Monitor</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { font-family: Arial, sans-serif; padding: 15px; text-align: center; background: #f9f9f9; position: relative; }
    .chart-container { width: 95%; margin: 20px auto; background: white; padding: 15px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    .data-box { margin: 10px 0; padding: 10px; background: #e8f5e9; border-radius: 8px; font-size: 18px; }
    h2 { color: #2c3e50; margin-bottom: 5px; }
    button { margin: 10px 5px; padding: 10px 20px; font-size: 16px; background: #3498db; color: white; border: none; border-radius: 5px; cursor: pointer; }
    button:hover { background: #2980b9; }
    .footer { margin-top: 20px; font-size: 12px; color: #7f8c8d; }
    
    /* Всплывающее меню */
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

    .settings-popup h3 {
      margin-top: 0;
    }

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
    .settings-icon:hover {
      color: #2980b9;
    }
  </style>
</head>
<body>
  <h2>🌿 CO₂ Monitor</h2>
  <!-- Шестерёнка -->
  <a class="settings-icon" onclick="openSettings()">⚙️</a>

  <!-- Оверлей -->
  <div class="settings-overlay" onclick="closeSettings()"></div>

  <!-- Всплывающее меню -->
  <div class="settings-popup" id="settingsPopup">
    <h3>⚙️ Настройки</h3>
    
    <label>Температура (°C):</label>
    <input type="number" step="0.1" id="tempOffset" value="0.0"><br>

    <label>Влажность (%):</label>
    <input type="number" step="0.1" id="humOffset" value="-47.0"><br>

    <label>CO₂ (ppm):</label>
    <input type="number" id="co2Offset" value="0"><br>

    <label>Google Script URL:</label>
    <input type="text" id="googleUrl" value="https://script.google.com/macros/s/..."><br><br>

    <button onclick="saveSettingsAjax()">💾 Сохранить</button>
    <button onclick="closeSettings()">❌ Закрыть</button>
  </div>
  
  <div class="data-box">
    CO₂: <b><span id="co2">--</span> ppm</b> |
    Temp: <b><span id="temp">--</span> °C</b> |
    Hum: <b><span id="hum">--</span> %</b>
  </div>

  <div class="chart-container">
    <canvas id="myChart"></canvas>
  </div>

  <button onclick="location.reload()">🔄 Обновить</button>

  <div class="footer">
    Данные сохраняются каждый час. Последнее обновление: <span id="lastUpdate"></span>
  </div>

  <script>
    // Открыть меню
    function openSettings() {
      // Загружаем текущие значения
      document.getElementById('tempOffset').value = ')rawliteral" + String(tempOffset, 1) + R"rawliteral(';
      document.getElementById('humOffset').value = ')rawliteral" + String(humOffset, 1) + R"rawliteral(';
      document.getElementById('co2Offset').value = ')rawliteral" + String(co2Offset) + R"rawliteral(';
      document.getElementById('googleUrl').value = ')rawliteral" + googleScriptUrl + R"rawliteral(';

      document.querySelector('.settings-overlay').style.display = 'block';
      document.getElementById('settingsPopup').style.display = 'block';
    }

    // Закрыть меню
    function closeSettings() {
      document.querySelector('.settings-overlay').style.display = 'none';
      document.getElementById('settingsPopup').style.display = 'none';
    }

    // Обновить текущие значения на странице
    function updateCurrentValues() {
      fetch('/json')
        .then(r => r.json())
        .then(d => {
          document.getElementById('co2').innerText = d.co2;
          document.getElementById('temp').innerText = d.temp;
          document.getElementById('hum').innerText = d.hum;
        });
    }

    // Сохранить настройки через AJAX
    function saveSettingsAjax() {
      const temp = parseFloat(document.getElementById('tempOffset').value);
      const hum = parseFloat(document.getElementById('humOffset').value);
      const co2 = parseInt(document.getElementById('co2Offset').value);
      const url = document.getElementById('googleUrl').value;

      // Отправляем настройки на устройство через AJAX
      fetch('/apply-settings', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          temp: temp,
          hum: hum,
          co2: co2,
          url: url
        })
      })
      .then(response => {
        if (response.ok) {
          alert('✅ Настройки сохранены и применены!');
          closeSettings();
          // Обновляем текущие значения на странице
          updateCurrentValues();
        } else {
          alert('❌ Ошибка применения настроек');
        }
      })
      .catch(error => {
        console.error('Ошибка:', error);
        alert('❌ Ошибка сети');
      });
    }

    // Получаем текущие значения с датчика
    fetch('/json')
      .then(r => r.json())
      .then(d => {
        document.getElementById('co2').innerText = d.co2;
        document.getElementById('temp').innerText = d.temp;
        document.getElementById('hum').innerText = d.hum;
        document.getElementById('lastUpdate').innerText = new Date().toLocaleTimeString();
      })
      .catch(error => {
        console.error('Ошибка загрузки текущих данных:', error);
      });

    // Загружаем исторические данные из Google Sheets
    fetch(')rawliteral" + String(googleScriptUrl) + R"rawliteral(')
      .then(response => {
        if (!response.ok) {
          throw new Error('Network response was not ok');
        }
        return response.json();
      })
      .then(data => {
        if (data.length === 0) {
          document.querySelector('.chart-container').innerHTML = '<p>Нет данных для отображения. Подождите, пока пройдёт первый час.</p>';
          return;
        }

        const labels = data.map(d => {
          const dt = new Date(d.t.replace(' ', 'T'));
          if (isNaN(dt)) return d.t;
          const hours = String(dt.getHours()).padStart(2, '0');
          const mins = String(dt.getMinutes()).padStart(2, '0');
          return `${hours}:${mins}`;
        });

        const ctx = document.getElementById('myChart').getContext('2d');
        new Chart(ctx, {
          type: 'line',
          data: {
            labels: labels,
            datasets: [
              {
                label: 'CO₂ (ppm)',
                data: data.map(d => d.c),
                borderColor: '#e74c3c',
                tension: 0.2,
                fill: false
              },
              {
                label: 'Температура (°C)',
                data: data.map(d => d.tmp),
                borderColor: '#3498db',
                tension: 0.2,
                fill: false,
                yAxisID: 'y-temp'
              },
              {
                label: 'Влажность (%)',
                data: data.map(d => d.h),
                borderColor: '#2ecc71',
                tension: 0.2,
                fill: false,
                yAxisID: 'y-hum'
              }
            ]
          },
          options: {
            responsive: true,
            scales: {
              y: {
                beginAtZero: false,
                title: { display: true, text: 'CO₂ (ppm)' }
              },
              yTemp: {
                position: 'right',
                beginAtZero: false,
                title: { display: true, text: 'Темп (°C)' },
                grid: { drawOnChartArea: false }
              },
              yHum: {
                position: 'right',
                beginAtZero: false,
                title: { display: true, text: 'Влажн (%)' },
                grid: { drawOnChartArea: false }
              }
            },
            plugins: {
              legend: { position: 'top' },
              tooltip: { mode: 'index', intersect: false }
            }
          }
        });
      })
      .catch(error => {
        console.error('Ошибка загрузки данных из Google Sheets:', error);
        document.querySelector('.chart-container').innerHTML = '<p>Ошибка загрузки исторических данных</p>';
      });
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleJson() {
  String json = "{";
  json += "\"co2\":" + String(co2) + ",";
  json += "\"temp\":" + String(temperature, 1) + ",";
  json += "\"hum\":" + String(humidity, 0);
  json += "}";
  server.send(200, "application/json", json);
}

void handleApplySettings() {
  if (server.hasArg("plain")) {
    // Получаем JSON
    String json = server.arg("plain");
    Serial.println("Received settings: " + json);
    
    // Простой парсинг
    int t1 = json.indexOf("\"temp\":");
    int t2 = json.indexOf(",", t1);
    if (t1 != -1 && t2 != -1) {
      tempOffset = json.substring(t1 + 7, t2).toFloat();
    }

    int h1 = json.indexOf("\"hum\":");
    int h2 = json.indexOf(",", h1);
    if (h1 != -1 && h2 != -1) {
      humOffset = json.substring(h1 + 6, h2).toFloat();
    }

    int c1 = json.indexOf("\"co2\":");
    int c2 = json.indexOf(",", c1);
    if (c1 == -1) c2 = json.indexOf("}", c1);
    if (c1 != -1 && c2 != -1) {
      co2Offset = json.substring(c1 + 6, c2).toInt();
    }

    int u1 = json.indexOf("\"url\":\"");
    int u2 = json.indexOf("\"", u1 + 7);
    if (u1 != -1 && u2 != -1) {
      googleScriptUrl = json.substring(u1 + 7, u2);
    }

    // Сохраняем в SPIFFS
    saveSettings();
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void startWebServer() {
  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.on("/settings", handleSettings);
  server.on("/save-settings", handleSaveSettings);
  server.on("/apply-settings", HTTP_POST, handleApplySettings);
  server.begin();
  Serial.println("HTTP server started");
}

void handleWebRequests() {
  server.handleClient();
}