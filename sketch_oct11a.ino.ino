#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include "model.h"  // Include your trained TinyML model

// =====================================
// 1. SENSOR PIN CONFIGURATION
// =====================================
#define MQ135_PIN 34
#define DHT_PIN   4
#define DHT_TYPE  DHT22

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

// Instance of your microML classifier
Eloquent::ML::Port::DecisionTree aiModel;

// =====================================
// 2. ESP32 WI-FI ACCESS POINT
// =====================================
const char* ssid = "AirMonitor-ESP32";
const char* password = "Air12345";

// =====================================
// 3. SENSOR & AI VARIABLES
// =====================================
int gasValue = 0;
float sensorVoltage = 0.0;
float temperature = 25.0; 
float humidity = 50.0;    

String aiPredictedQuality = "Initializing AI...";
int aiClassLabel = -1;

// 15-Minute Future Prediction Variables
int futureGasValue = 0;
String futurePredictedQuality = "Calculating...";
float rateOfChangePerMin = 0.0;

// History tracking for rate of change calculation (30-second buffer)
const int HISTORY_SIZE = 20;
int gasHistoryBuffer[HISTORY_SIZE];
int historyIndex = 0;
bool bufferFilled = false;

unsigned long previousMillis = 0;
const unsigned long interval = 1500;

// Threshold classifier
String getLabelFromClass(int classId, int rawGas) {
  if (rawGas > 1300) {
    return "Poor / High Risk (AI)";
  } else if (rawGas > 1000) {
    return "Moderate Pollution (AI)";
  }

  switch(classId) {
    case 0: return "Good Air Quality (AI)";
    case 1: return "Moderate Pollution (AI)";
    case 2: return "Poor / High Risk (AI)";
    default: return "Good Air Quality (AI)";
  }
}

// =====================================
// 4. READ SENSORS & RUN PREDICTION ENGINE
// =====================================
void readSensorsAndPredict() {
  gasValue = analogRead(MQ135_PIN);
  sensorVoltage = gasValue * 3.3f / 4095.0f;

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;

  // Run TinyML Inference on ESP32
  float inputFeatures[3] = { temperature, humidity, (float)gasValue };
  aiClassLabel = aiModel.predict(inputFeatures);
  aiPredictedQuality = getLabelFromClass(aiClassLabel, gasValue);

  // Buffer gas history to calculate rate of change
  gasHistoryBuffer[historyIndex] = gasValue;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyIndex == 0) bufferFilled = true;

  // Calculate Rate of Change over last buffer period
  int oldestIndex = bufferFilled ? historyIndex : 0;
  int oldGas = gasHistoryBuffer[oldestIndex];
  float deltaGas = gasValue - oldGas;
  float timeSpanMinutes = (bufferFilled ? HISTORY_SIZE : (historyIndex > 0 ? historyIndex : 1)) * 1.5f / 60.0f;
  
  rateOfChangePerMin = deltaGas / timeSpanMinutes;

  // Extrapolate projected gas level 15 minutes ahead
  futureGasValue = gasValue + (int)(rateOfChangePerMin * 15.0f);
  if (futureGasValue < 0) futureGasValue = 0;
  if (futureGasValue > 4095) futureGasValue = 4095;

  futurePredictedQuality = getLabelFromClass(-1, futureGasValue);
}

// =====================================
// 5. DASHBOARD HTML WITH PREDICTION PANEL
// =====================================
const char webpage[] PROGMEM = R"===AIR_APP=(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>AI Air Quality Monitor</title>
<style>
* { box-sizing: border-box; }
body { margin: 0; font-family: Arial, sans-serif; background: #0b1220; color: #eef4ff; transition: background 0.3s; }
header { text-align: center; padding: 25px 15px; background: #111c30; border-bottom: 1px solid #293953; }
header h1 { margin: 0; font-size: 25px; }
header p { color: #a7bad5; margin: 5px 0 0 0; }
.time-badge { font-size: 14px; font-weight: bold; color: #38bdf8; margin-top: 8px; display: block; }
.container { max-width: 850px; margin: auto; padding: 18px; }
.status { background: #15243a; border: 1px solid #2b405d; border-radius: 18px; padding: 20px; margin-bottom: 16px; transition: all 0.3s; }
.status small, .label, .muted { color: #a7bad5; }
.status h2 { font-size: 28px; margin: 12px 0; color: #60a5fa; }
.pill { display: inline-block; padding: 8px 13px; border-radius: 20px; background: #29415f; font-size: 13px; }
.controls { text-align: center; margin: 15px 0; display: flex; gap: 10px; justify-content: center; flex-wrap: wrap; }
.btn { background: #2563eb; color: white; border: none; padding: 10px 20px; border-radius: 10px; font-size: 15px; font-weight: bold; cursor: pointer; transition: background 0.2s; }
.btn:hover { background: #1d4ed8; }
.btn-alert { background: #d97706; }
.btn-alert:hover { background: #b45309; }

/* ADVICE BLOCK STYLES */
.advice-box { background: rgba(15, 23, 42, 0.7); border: 1px solid #334155; border-radius: 12px; padding: 15px; margin-top: 15px; text-align: left; }
.advice-title { font-weight: bold; font-size: 14px; margin-bottom: 8px; display: flex; align-items: center; gap: 6px; color: #38bdf8; }
.advice-list { margin: 0; padding-left: 20px; font-size: 13px; color: #cbd5e1; line-height: 1.6; }

.grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 13px; }
.card { padding: 18px; border-radius: 17px; background: #15243a; border: 1px solid #2b405d; min-width: 0; }
.icon { font-size: 27px; }
.label { font-size: 14px; margin-top: 12px; }
.value { font-size: 30px; font-weight: bold; margin: 10px 0; overflow-wrap: anywhere; }
.unit { font-size: 14px; color: #a7bad5; }
.track { height: 8px; background: #2a3b53; border-radius: 10px; margin-top: 15px; overflow: hidden; }
.bar { height: 100%; width: 0%; background: #60a5fa; transition: width 0.4s; }

/* PREDICTION PANEL STYLES */
.prediction-panel { background: #111e36; border: 1px dashed #38bdf8; border-radius: 17px; padding: 20px; margin-top: 16px; text-align: left; }
.prediction-header { font-size: 16px; font-weight: bold; color: #38bdf8; margin-bottom: 12px; display: flex; align-items: center; gap: 8px; }
.prediction-grid { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 10px; }
.pred-card { background: #182842; padding: 12px; border-radius: 10px; border: 1px solid #293b59; text-align: center; }
.pred-card small { display: block; color: #94a3b8; font-size: 11px; margin-bottom: 4px; }
.pred-card span { font-size: 16px; font-weight: bold; color: #e2e8f0; }

.footer { text-align: center; color: #a7bad5; font-size: 12px; padding: 22px 0; }

/* HAZARD VISUAL STATE */
body.hazard-bg { background: #2a0808 !important; }
.status.hazard { border-color: #ef4444 !important; background: #450a0a !important; animation: pulse 1s infinite alternate; }
.status.hazard h2 { color: #f87171 !important; }
.status.hazard .advice-box { border-color: #f87171; background: rgba(127, 29, 29, 0.6); }
.status.hazard .advice-title { color: #fca5a5; }
@keyframes pulse { from { box-shadow: 0 0 10px #ef4444; } to { box-shadow: 0 0 25px #ef4444; } }

#hazardBanner { display: none; background: #dc2626; color: white; text-align: center; padding: 12px; font-weight: bold; font-size: 16px; position: sticky; top: 0; z-index: 100; }
</style>
</head>
<body>
<div id="hazardBanner">⚠️ HAZARD WARNING: High Gas Concentration Detected!</div>
<header>
  <h1>AI-Powered Air Quality Monitor</h1>
  <p>ESP32 | TinyML | MQ-135 | DHT22</p>
  <span class="time-badge" id="clock">Current Time: --:--:--</span>
</header>
<div class="container">
  <div class="status" id="statusCard">
    <small>AI PREDICTED STATUS</small>
    <h2 id="quality">Analyzing...</h2>
    <span class="pill" id="connection">Connecting to ESP32</span>
    <p class="muted">Real-time prediction generated on-device using Onboard Decision Tree Model.</p>
    
    <div class="controls">
      <button class="btn" onclick="triggerManualPrediction()">⚡ Predict Now</button>
      <button class="btn btn-alert" id="alertBtn" onclick="requestNotificationPermission()">🔔 Enable Hazard Alerts</button>
    </div>

    <!-- AI ADVICE & SAFETY RECOMMENDATIONS BLOCK -->
    <div class="advice-box">
      <div class="advice-title" id="adviceTitle">💡 Safety Guidance & Recommended Actions</div>
      <ul class="advice-list" id="adviceList">
        <li>Monitoring environment... Recommendations will update automatically based on air status.</li>
      </ul>
    </div>
  </div>

  <div class="grid">
    <div class="card">
      <div class="icon">🌡️</div>
      <div class="label">Temperature</div>
      <div class="value"><span id="temp">--</span> <span class="unit">°C</span></div>
      <div class="muted">DHT22 sensor</div>
    </div>
    <div class="card">
      <div class="icon">💧</div>
      <div class="label">Humidity</div>
      <div class="value"><span id="hum">--</span> <span class="unit">%</span></div>
      <div class="muted">Relative humidity</div>
    </div>
    <div class="card">
      <div class="icon">🫧</div>
      <div class="label">Gas level</div>
      <div class="value" id="gas">--</div>
      <div class="muted">Raw ADC: 0–4095</div>
      <div class="track"><div class="bar" id="gasbar"></div></div>
    </div>
    <div class="card">
      <div class="icon">⚡</div>
      <div class="label">Sensor voltage</div>
      <div class="value"><span id="volt">--</span> <span class="unit">V</span></div>
      <div class="muted">Voltage at ESP32 GPIO34</div>
    </div>
  </div>

  <!-- BOTTOM PREDICTION ANALYTICS PANEL -->
  <div class="prediction-panel">
    <div class="prediction-header">🔮 15-Minute Trend & Predictive Forecast</div>
    <div class="prediction-grid">
      <div class="pred-card">
        <small>Projected Gas (15m)</small>
        <span id="futureGas">--</span>
      </div>
      <div class="pred-card">
        <small>Projected Risk Level</small>
        <span id="futureQuality">--</span>
      </div>
      <div class="pred-card">
        <small>Gas Trend Rate</small>
        <span id="trendRate">--</span>
      </div>
    </div>
  </div>

  <div class="footer">
    Live AI inference running locally on ESP32<br>
    <p id="updated">Waiting for readings...</p>
  </div>
</div>
<script>
let lastAlertTime = 0;
let audioCtx = null;

function updateClock() {
  const now = new Date();
  document.getElementById('clock').textContent = 'Current Time: ' + now.toLocaleTimeString();
}
setInterval(updateClock, 1000);
updateClock();

function checkNotificationStatus() {
  if (Notification.permission === "granted") {
    document.getElementById('alertBtn').textContent = "✅ Alerts Active";
    document.getElementById('alertBtn').style.background = "#059669";
  }
}

function requestNotificationPermission() {
  if (!audioCtx) {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  }
  if ("Notification" in window) {
    Notification.requestPermission().then(permission => {
      checkNotificationStatus();
    });
  }
  alert("Audio & Visual Hazard Alerts are enabled!");
}

function playWarningBeep() {
  try {
    if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    const osc = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.type = 'sawtooth';
    osc.frequency.setValueAtTime(880, audioCtx.currentTime);
    gain.gain.setValueAtTime(0.3, audioCtx.currentTime);
    osc.connect(gain);
    gain.connect(audioCtx.destination);
    osc.start();
    osc.stop(audioCtx.currentTime + 0.5);
  } catch(e) {}
}

function triggerHazardAlert(statusText, gasValue) {
  playWarningBeep();

  const now = Date.now();
  if (now - lastAlertTime < 30000) return;
  lastAlertTime = now;

  if ("Notification" in window && Notification.permission === "granted") {
    new Notification("⚠️ HAZARDOUS AIR QUALITY DETECTED", {
      body: "Status: " + statusText + " | Gas ADC: " + gasValue,
      icon: "https://cdn-icons-png.flaticon.com/512/564/564619.png"
    });
  }
}

function updateAdviceBlock(quality) {
  const adviceTitle = document.getElementById('adviceTitle');
  const adviceList = document.getElementById('adviceList');

  if (quality.includes("Poor") || quality.includes("High Risk")) {
    adviceTitle.innerHTML = "🚨 EMERGENCY HAZARD ACTION PLAN";
    adviceList.innerHTML = `
      <li><b>Open Windows & Doors Immediately:</b> Increase fresh air ventilation to dilute gas concentration.</li>
      <li><b>Turn on Exhaust Fans / Air Purifier:</b> Set air filters or ventilation systems to high speed.</li>
      <li><b>Identify & Isolate Pollution Source:</b> Extinguish open flames, check gas valves, or remove combustion sources.</li>
      <li><b>Evacuate Area if Symptoms Occur:</b> Step outside to clean air if you experience dizziness, coughing, or eye irritation.</li>
    `;
  } else if (quality.includes("Moderate")) {
    adviceTitle.innerHTML = "⚠️ MODERATE AIR QUALITY ADVICE";
    adviceList.innerHTML = `
      <li><b>Increase Airflow:</b> Open room windows or turn on a fan to refresh the indoor climate.</li>
      <li><b>Monitor Air Purifiers:</b> If available, run an indoor air filter on standard power.</li>
      <li><b>Avoid Indoor Burning:</b> Limit incense, candles, or indoor combustion until air levels normalize.</li>
    `;
  } else {
    adviceTitle.innerHTML = "💡 OPTIMAL AIR QUALITY ADVICE";
    adviceList.innerHTML = `
      <li><b>Air Quality is Excellent:</b> No immediate safety actions required.</li>
      <li><b>Maintain Natural Ventilation:</b> Keep standard ventilation cycles active to maintain low gas buildup.</li>
      <li><b>Comfort Zone Matched:</b> Room climate and air composition match recommended healthy indoor standards.</li>
    `;
  }
}

async function updateDashboard() {
  try {
    const response = await fetch('/api', { cache: 'no-store' });
    if (!response.ok) throw new Error('API unavailable');
    const d = await response.json();

    document.getElementById('connection').textContent = 'Connected to ESP32';
    document.getElementById('temp').textContent = d.temperature === null ? '--' : d.temperature.toFixed(1);
    document.getElementById('hum').textContent = d.humidity === null ? '--' : d.humidity.toFixed(1);
    document.getElementById('gas').textContent = d.gas;
    document.getElementById('volt').textContent = d.voltage.toFixed(2);
    document.getElementById('gasbar').style.width = (d.gas / 4095 * 100) + '%';
    document.getElementById('quality').textContent = d.quality;
    document.getElementById('updated').textContent = 'Last Prediction: ' + new Date().toLocaleTimeString();

    // Update Bottom Prediction Analytics Panel
    if (d.future_gas !== undefined) {
      document.getElementById('futureGas').textContent = d.future_gas;
      document.getElementById('futureQuality').textContent = d.future_quality;
      document.getElementById('trendRate').textContent = (d.rate_per_min > 0 ? '+' : '') + d.rate_per_min.toFixed(1) + ' ADC/min';
    }

    const statusCard = document.getElementById('statusCard');
    const hazardBanner = document.getElementById('hazardBanner');

    updateAdviceBlock(d.quality);

    if (d.quality.includes("Poor") || d.quality.includes("High Risk")) {
      statusCard.classList.add('hazard');
      document.body.classList.add('hazard-bg');
      hazardBanner.style.display = 'block';
      triggerHazardAlert(d.quality, d.gas);
    } else {
      statusCard.classList.remove('hazard');
      document.body.classList.remove('hazard-bg');
      hazardBanner.style.display = 'none';
    }

  } catch (error) {
    document.getElementById('connection').textContent = 'Connection lost - reconnecting';
    document.getElementById('quality').textContent = 'Waiting for ESP32';
  }
}

async function triggerManualPrediction() {
  document.getElementById('quality').textContent = 'Predicting...';
  try {
    await fetch('/predict', { cache: 'no-store' });
    await updateDashboard();
  } catch(e) {
    console.error(e);
  }
}

checkNotificationStatus();
updateDashboard();
setInterval(updateDashboard, 1500);
</script>
</body>
</html>
)===AIR_APP=";

// =====================================
// 6. SERVE DASHBOARD & API
// =====================================
void handleRoot() {
  server.send_P(200, "text/html", webpage);
}

void handleAPI() {
  String json = "{";
  json += "\"gas\":" + String(gasValue);
  json += ",\"voltage\":" + String(sensorVoltage, 3);
  json += ",\"temperature\":" + (isnan(temperature) ? "null" : String(temperature, 1));
  json += ",\"humidity\":" + (isnan(humidity) ? "null" : String(humidity, 1));
  json += ",\"quality\":\"" + aiPredictedQuality + "\",";
  json += "\"future_gas\":" + String(futureGasValue) + ",";
  json += "\"future_quality\":\"" + futurePredictedQuality + "\",";
  json += "\"rate_per_min\":" + String(rateOfChangePerMin, 1);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handlePredict() {
  readSensorsAndPredict();
  handleAPI();
}

// =====================================
// 7. SETUP & MAIN LOOP
// =====================================
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  dht.begin();

  for (int i = 0; i < HISTORY_SIZE; i++) gasHistoryBuffer[i] = 0;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api", HTTP_GET, handleAPI);
  server.on("/predict", HTTP_GET, handlePredict);
  server.begin();

  readSensorsAndPredict();
  previousMillis = millis();
}

void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readSensorsAndPredict();

    Serial.println("-----------------------------");
    Serial.print("MQ-135 ADC: "); Serial.println(gasValue);
    Serial.print("Temperature: "); Serial.print(temperature, 1); Serial.println(" C");
    Serial.print("Humidity: "); Serial.print(humidity, 1); Serial.println(" %");
    Serial.print("AI Prediction: "); Serial.println(aiPredictedQuality);
  }
}