#ifndef INDEX_H
#define INDEX_H

#include <pgmspace.h>

const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>AI Air Quality Monitor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
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

.advice-box { background: rgba(15, 23, 42, 0.7); border: 1px solid #334155; border-radius: 12px; padding: 15px; margin-top: 15px; text-align: left; }
.advice-title { font-weight: bold; font-size: 14px; margin-bottom: 8px; display: flex; align-items: center; gap: 6px; color: #38bdf8; }
.advice-list { margin: 0; padding-left: 20px; font-size: 13px; color: #cbd5e1; line-height: 1.6; }

.chart-card { background: #15243a; border: 1px solid #2b405d; border-radius: 18px; padding: 20px; margin-bottom: 16px; }
.chart-title { font-size: 16px; font-weight: bold; margin-bottom: 10px; color: #38bdf8; }

.grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 13px; }
.card { padding: 18px; border-radius: 17px; background: #15243a; border: 1px solid #2b405d; min-width: 0; }
.icon { font-size: 27px; }
.label { font-size: 14px; margin-top: 12px; }
.value { font-size: 30px; font-weight: bold; margin: 10px 0; overflow-wrap: anywhere; }
.unit { font-size: 14px; color: #a7bad5; }
.track { height: 8px; background: #2a3b53; border-radius: 10px; margin-top: 15px; overflow: hidden; }
.bar { height: 100%; width: 0%; background: #60a5fa; transition: width 0.4s; }
.footer { text-align: center; color: #a7bad5; font-size: 12px; padding: 22px 0; }

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

    <div class="advice-box">
      <div class="advice-title" id="adviceTitle">💡 Safety Guidance & Recommended Actions</div>
      <ul class="advice-list" id="adviceList">
        <li>Monitoring environment... Recommendations will update automatically based on air status.</li>
      </ul>
    </div>
  </div>

  <div class="chart-card">
    <div class="chart-title">📈 Live Gas History & AI Risk Trend</div>
    <div style="position: relative; height: 250px; width: 100%;">
      <canvas id="airChart"></canvas>
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
  <div class="footer">
    Live AI inference running locally on ESP32<br>
    <p id="updated">Waiting for readings...</p>
  </div>
</div>
<script>
let lastAlertTime = 0;
let audioCtx = null;
const maxHistory = 15;
let timeLabels = [];
let gasHistory = [];
let airChart = null;

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
      <li><b>Evacuate Area if Symptoms Occur:</b> Step outside to clean air if you experience dizziness or coughing.</li>
    `;
  } else if (quality.includes("Moderate")) {
    adviceTitle.innerHTML = "⚠️ MODERATE AIR QUALITY ADVICE";
    adviceList.innerHTML = `
      <li><b>Increase Airflow:</b> Open room windows or turn on a fan to refresh indoor air.</li>
      <li><b>Monitor Air Purifiers:</b> If available, run an indoor air filter on standard power.</li>
      <li><b>Avoid Indoor Burning:</b> Limit incense, candles, or indoor combustion until levels normalize.</li>
    `;
  } else {
    adviceTitle.innerHTML = "💡 OPTIMAL AIR QUALITY ADVICE";
    adviceList.innerHTML = `
      <li><b>Air Quality is Excellent:</b> No immediate safety actions required.</li>
      <li><b>Maintain Natural Ventilation:</b> Keep standard ventilation active to maintain low gas buildup.</li>
      <li><b>Comfort Zone Matched:</b> Room climate matches recommended healthy indoor standards.</li>
    `;
  }
}

function initChart() {
  const ctx = document.getElementById('airChart').getContext('2d');
  airChart = new Chart(ctx, {
    type: 'line',
    data: {
      labels: timeLabels,
      datasets: [{
        label: 'Gas ADC (MQ-135)',
        data: gasHistory,
        borderColor: '#38bdf8',
        backgroundColor: 'rgba(56, 189, 248, 0.15)',
        borderWidth: 3,
        fill: true,
        tension: 0.35,
        pointRadius: 4,
        pointBackgroundColor: '#38bdf8'
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
        x: { ticks: { color: '#a7bad5', font: { size: 10 } }, grid: { color: '#293953' } },
        y: { ticks: { color: '#a7bad5' }, grid: { color: '#293953' }, min: 0, max: 4095 }
      },
      plugins: { legend: { display: false } }
    }
  });
}

function updateChart(gas, quality) {
  const timeStr = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
  timeLabels.push(timeStr);
  gasHistory.push(gas);

  if (timeLabels.length > maxHistory) {
    timeLabels.shift();
    gasHistory.shift();
  }

  if (quality.includes("Poor") || quality.includes("High Risk")) {
    airChart.data.datasets[0].borderColor = '#ef4444';
    airChart.data.datasets[0].backgroundColor = 'rgba(239, 68, 68, 0.25)';
    airChart.data.datasets[0].pointBackgroundColor = '#ef4444';
  } else if (quality.includes("Moderate")) {
    airChart.data.datasets[0].borderColor = '#f59e0b';
    airChart.data.datasets[0].backgroundColor = 'rgba(245, 158, 11, 0.25)';
    airChart.data.datasets[0].pointBackgroundColor = '#f59e0b';
  } else {
    airChart.data.datasets[0].borderColor = '#38bdf8';
    airChart.data.datasets[0].backgroundColor = 'rgba(56, 189, 248, 0.15)';
    airChart.data.datasets[0].pointBackgroundColor = '#38bdf8';
  }

  airChart.update();
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

    const statusCard = document.getElementById('statusCard');
    const hazardBanner = document.getElementById('hazardBanner');

    updateAdviceBlock(d.quality);
    updateChart(d.gas, d.quality);

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

window.onload = () => {
  initChart();
  checkNotificationStatus();
  updateDashboard();
  setInterval(updateDashboard, 1500);
};
</script>
</body>
</html>
)=====";

#endif