#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ================== Wi-Fi ==================
const char* ssid     = "POCO M6 Pro 5g";
const char* password = "chinmay12";

// ================== Pins ===================
#define SENSOR_PIN 15
#define BUZZER_PIN 5
#define MOTOR_PIN 19

#define EYE_CLOSED_THRESHOLD 2000
#define BUZZER_THRESHOLD      2000

unsigned long eyeClosedStart = 0;
unsigned long buzzerStart    = 0;

bool eyePreviouslyClosed = false;
bool buzzerOn            = false;
bool motorOn             = false;

// ======== ESP32-CAM IP (EDIT THIS!) ========
const char* cameraIP = "10.173.208.243";   // <--- YOUR CAM IP

// ============== GPS ======================
WebServer server(80);
float lastLat = 0.0;
float lastLon = 0.0;
bool gpsReceived = false;

// ======== LIVE LOG BUFFER ==========
String liveLog = "";

// ======== LOG FUNCTION ==========
void sendLog(String msg) {
  Serial.println(msg);
  liveLog += msg + "\n";
}

// ======== CAMERA CONTROL ==========
void turnCameraOn() {
  HTTPClient http;
  http.begin("http://" + String(cameraIP) + "/on");
  http.GET();
  http.end();
  sendLog("Camera TURNED ON");
}

void turnCameraOff() {
  HTTPClient http;
  http.begin("http://" + String(cameraIP) + "/off");
  http.GET();
  http.end();
  sendLog("Camera TURNED OFF");
}

// ======== STATUS JSON ENDPOINT ==========
void handleStatus() {
  StaticJsonDocument<300> doc;
  doc["buzzerOn"] = buzzerOn;
  doc["motorOn"] = motorOn;
  doc["eyeClosed"] = eyePreviouslyClosed;
  doc["gpsReceived"] = gpsReceived;
  doc["lat"] = lastLat;
  doc["lon"] = lastLon;
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
}

// =========== GPS HTTP POST HANDLER ===========
void handleGpsPost() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<200> doc;

  if (deserializeJson(doc, body)) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  lastLat = doc["lat"].as<float>();
  lastLon = doc["lon"].as<float>();
  gpsReceived = true;

  sendLog("GPS: Lat=" + String(lastLat,6) + " Lon=" + String(lastLon,6));
  server.send(200, "text/plain", "OK");
}

void handleLog() {
  server.send(200, "text/plain", liveLog);
}

// ================== DASHBOARD WEBPAGE ==================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Drowsiness Detection System</title>
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
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
  transition: background 0.5s ease;
}

body.alert-mode {
  background: linear-gradient(135deg, #dc2626 0%, #991b1b 100%) !important;
  animation: alertPulse 1s infinite;
}

@keyframes alertPulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.85; }
}

.container {
  max-width: 1200px;
  margin: 0 auto;
}

.header {
  background: rgba(255, 255, 255, 0.95);
  padding: 25px 30px;
  border-radius: 15px;
  box-shadow: 0 8px 32px rgba(0,0,0,0.1);
  margin-bottom: 25px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  transition: all 0.3s ease;
}

.alert-mode .header {
  background: rgba(220, 38, 38, 0.95);
  box-shadow: 0 8px 32px rgba(220, 38, 38, 0.5);
}

.alert-mode .header h1,
.alert-mode .header .card-title {
  color: white !important;
}

.header h1 {
  color: #2d3748;
  font-size: 28px;
  font-weight: 700;
  display: flex;
  align-items: center;
  gap: 12px;
  transition: color 0.3s ease;
}

.status-badge {
  display: inline-block;
  padding: 8px 16px;
  border-radius: 20px;
  font-size: 14px;
  font-weight: 600;
  background: #48bb78;
  color: white;
  animation: pulse 2s infinite;
  transition: all 0.3s ease;
}

.status-badge.danger {
  background: #fbbf24;
  animation: alertBlink 0.5s infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.7; }
}

@keyframes alertBlink {
  0%, 100% { opacity: 1; transform: scale(1); }
  50% { opacity: 0.8; transform: scale(1.05); }
}

.alert-banner {
  display: none;
  background: #dc2626;
  color: white;
  padding: 20px;
  border-radius: 15px;
  margin-bottom: 25px;
  text-align: center;
  font-size: 24px;
  font-weight: 700;
  box-shadow: 0 8px 32px rgba(220, 38, 38, 0.5);
  animation: alertShake 0.5s infinite;
}

.alert-banner.show {
  display: block;
}

@keyframes alertShake {
  0%, 100% { transform: translateX(0); }
  25% { transform: translateX(-5px); }
  75% { transform: translateX(5px); }
}

.card {
  background: rgba(255, 255, 255, 0.95);
  border-radius: 15px;
  padding: 25px;
  box-shadow: 0 8px 32px rgba(0,0,0,0.1);
  margin-bottom: 25px;
  transition: transform 0.3s ease, box-shadow 0.3s ease, background 0.3s ease;
}

.alert-mode .card {
  background: rgba(254, 226, 226, 0.95);
  border: 2px solid #dc2626;
}

.card:hover {
  transform: translateY(-2px);
  box-shadow: 0 12px 40px rgba(0,0,0,0.15);
}

.card-title {
  font-size: 20px;
  font-weight: 600;
  color: #2d3748;
  margin-bottom: 20px;
  padding-bottom: 12px;
  border-bottom: 3px solid #667eea;
  display: flex;
  align-items: center;
  gap: 10px;
  transition: all 0.3s ease;
}

.alert-mode .card-title {
  border-bottom-color: #dc2626;
}

.controls {
  display: flex;
  gap: 15px;
  flex-wrap: wrap;
  margin-bottom: 20px;
}

button {
  flex: 1;
  min-width: 140px;
  padding: 16px 28px;
  font-size: 16px;
  font-weight: 600;
  border: none;
  border-radius: 10px;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 4px 15px rgba(0,0,0,0.1);
  position: relative;
  overflow: hidden;
}

button::before {
  content: '';
  position: absolute;
  top: 50%;
  left: 50%;
  width: 0;
  height: 0;
  border-radius: 50%;
  background: rgba(255,255,255,0.3);
  transform: translate(-50%, -50%);
  transition: width 0.6s, height 0.6s;
}

button:hover::before {
  width: 300px;
  height: 300px;
}

#onBtn {
  background: linear-gradient(135deg, #48bb78 0%, #38a169 100%);
  color: white;
}

#onBtn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(72,187,120,0.4);
}

#offBtn {
  background: linear-gradient(135deg, #f56565 0%, #c53030 100%);
  color: white;
}

#offBtn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(245,101,101,0.4);
}

button:active {
  transform: translateY(0);
}

#camBox {
  width: 100%;
  height: 450px;
  border-radius: 12px;
  background: linear-gradient(135deg, #1a202c 0%, #2d3748 100%);
  overflow: hidden;
  position: relative;
  box-shadow: inset 0 2px 10px rgba(0,0,0,0.3);
}

#camView {
  width: 100%;
  height: 100%;
  object-fit: contain;
  display: block;
}

.no-stream {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  color: #a0aec0;
  font-size: 18px;
  text-align: center;
  pointer-events: none;
}

#map {
  width: 100%;
  height: 400px;
  border-radius: 12px;
  box-shadow: inset 0 2px 10px rgba(0,0,0,0.1);
}

.gps-info {
  margin-top: 15px;
  padding: 15px;
  background: #f7fafc;
  border-radius: 8px;
  font-size: 14px;
  color: #2d3748;
}

.gps-info strong {
  color: #667eea;
}

.alert-mode .gps-info {
  background: #fee2e2;
  border: 1px solid #dc2626;
}

#logbox {
  width: 100%;
  height: 350px;
  background: #1a202c;
  border-radius: 12px;
  padding: 20px;
  overflow-y: auto;
  font-family: 'Courier New', monospace;
  font-size: 14px;
  line-height: 1.6;
  color: #48bb78;
  box-shadow: inset 0 2px 10px rgba(0,0,0,0.3);
}

.alert-mode #logbox {
  background: #7f1d1d;
  color: #fca5a5;
}

#logbox::-webkit-scrollbar {
  width: 8px;
}

#logbox::-webkit-scrollbar-track {
  background: #2d3748;
  border-radius: 10px;
}

#logbox::-webkit-scrollbar-thumb {
  background: #667eea;
  border-radius: 10px;
}

#logbox::-webkit-scrollbar-thumb:hover {
  background: #764ba2;
}

.log-entry {
  margin-bottom: 5px;
  padding: 5px;
  border-left: 3px solid #667eea;
  padding-left: 10px;
}

.alert-mode .log-entry {
  border-left-color: #fca5a5;
}

.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
  gap: 25px;
}

.icon {
  width: 24px;
  height: 24px;
  display: inline-block;
}

@media (max-width: 768px) {
  .header {
    flex-direction: column;
    align-items: flex-start;
    gap: 15px;
  }
  
  .header h1 {
    font-size: 24px;
  }
  
  .controls {
    flex-direction: column;
  }
  
  button {
    width: 100%;
  }
  
  #camBox, #map {
    height: 300px;
  }
}

.timestamp {
  font-size: 12px;
  color: #a0aec0;
  margin-left: 10px;
}

.footer {
  text-align: center;
  color: rgba(255,255,255,0.8);
  padding: 20px;
  font-size: 14px;
}
</style>
</head>
<body>

<div class="container">
  <div class="header">
    <h1>
      <svg class="icon" fill="currentColor" viewBox="0 0 20 20">
        <path d="M10 2a6 6 0 00-6 6v3.586l-.707.707A1 1 0 004 14h12a1 1 0 00.707-1.707L16 11.586V8a6 6 0 00-6-6zM10 18a3 3 0 01-3-3h6a3 3 0 01-3 3z"/>
      </svg>
      Drowsiness Detection System
    </h1>
    <span class="status-badge" id="statusBadge">● ACTIVE</span>
  </div>

  <div class="alert-banner" id="alertBanner">
    ⚠️ CRITICAL ALERT: DRIVER UNRESPONSIVE! ⚠️
  </div>

  <div class="card">
    <div class="card-title">
      <svg class="icon" fill="currentColor" viewBox="0 0 20 20">
        <path d="M5.05 4.05a7 7 0 119.9 9.9L10 18.9l-4.95-4.95a7 7 0 010-9.9zM10 11a2 2 0 100-4 2 2 0 000 4z"/>
      </svg>
      GPS Location Tracker
    </div>
    
    <div id="map"></div>
    
    <div class="gps-info" id="gpsInfo">
      <strong>Status:</strong> <span id="gpsStatus">Waiting for GPS data...</span><br>
      <strong>Coordinates:</strong> <span id="gpsCoords">--</span><br>
      <strong>Address:</strong> <span id="gpsAddress">--</span>
    </div>
  </div>

  <div class="card">
    <div class="card-title">
      <svg class="icon" fill="currentColor" viewBox="0 0 20 20">
        <path d="M2 6a2 2 0 012-2h6a2 2 0 012 2v8a2 2 0 01-2 2H4a2 2 0 01-2-2V6zM14.553 7.106A1 1 0 0014 8v4a1 1 0 00.553.894l2 1A1 1 0 0018 13V7a1 1 0 00-1.447-.894l-2 1z"/>
      </svg>
      Camera Controls
    </div>
    
    <div class="controls">
      <button id="onBtn" onclick="camOn()">
        ▶ START CAMERA
      </button>
      <button id="offBtn" onclick="camOff()">
        ■ STOP CAMERA
      </button>
    </div>

    <div id="camBox">
      <div class="no-stream" id="noStream">Camera Offline - Click START to begin</div>
      <img id="camView" src="" alt="Camera Stream" style="display:none;">
    </div>
  </div>

  <div class="card">
    <div class="card-title">
      <svg class="icon" fill="currentColor" viewBox="0 0 20 20">
        <path fill-rule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-7-4a1 1 0 11-2 0 1 1 0 012 0zM9 9a1 1 0 000 2v3a1 1 0 001 1h1a1 1 0 100-2v-3a1 1 0 00-1-1H9z" clip-rule="evenodd"/>
      </svg>
      System Log
      <span class="timestamp" id="lastUpdate"></span>
    </div>
    <div id="logbox"></div>
  </div>

  <div class="footer">
    Powered by ESP32 | Real-time Monitoring System
  </div>
</div>

<script>
let audioContext = null;
let isAlertMode = false;
let alertSoundInterval = null;
let map = null;
let marker = null;
let currentLat = 0;
let currentLon = 0;

// Initialize Leaflet Map
function initMap() {
  map = L.map('map').setView([20.5937, 78.9629], 5); // Default: India center
  
  // Add OpenStreetMap tiles (FREE!)
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19,
    attribution: '© OpenStreetMap contributors'
  }).addTo(map);
}

// Update Map Location with Marker
function updateMapLocation(lat, lon) {
  if (!map) return;
  
  // Remove existing marker
  if (marker) {
    map.removeLayer(marker);
  }
  
  // Add new marker
  marker = L.marker([lat, lon]).addTo(map);
  marker.bindPopup('<b>Alert Location</b><br>Driver Unresponsive').openPopup();
  
  // Center map on location
  map.setView([lat, lon], 15);
  
  // Update coordinates display
  document.getElementById('gpsCoords').textContent = 
    lat.toFixed(6) + ', ' + lon.toFixed(6);
  
  // Reverse geocode to get address
  fetch(`https://nominatim.openstreetmap.org/reverse?format=json&lat=${lat}&lon=${lon}`)
    .then(res => res.json())
    .then(data => {
      if (data.display_name) {
        document.getElementById('gpsAddress').textContent = data.display_name;
      }
    })
    .catch(err => console.error('Geocoding error:', err));
}

// Initialize Audio Context
function initAudio() {
  if (!audioContext) {
    audioContext = new (window.AudioContext || window.webkitAudioContext)();
  }
}

// Generate Alert Beep Sound
function playAlertBeep() {
  if (!audioContext) initAudio();
  
  const oscillator = audioContext.createOscillator();
  const gainNode = audioContext.createGain();
  
  oscillator.connect(gainNode);
  gainNode.connect(audioContext.destination);
  
  oscillator.frequency.value = 1200;
  oscillator.type = 'sine';
  
  gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
  gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);
  
  oscillator.start(audioContext.currentTime);
  oscillator.stop(audioContext.currentTime + 0.5);
}

// Check System Status
function checkStatus() {
  fetch('/status')
    .then(res => res.json())
    .then(data => {
      const { buzzerOn, motorOn, gpsReceived, lat, lon } = data;
      
      // Update GPS location if received
      if (gpsReceived && lat !== 0 && lon !== 0) {
        if (lat !== currentLat || lon !== currentLon) {
          currentLat = lat;
          currentLon = lon;
          updateMapLocation(lat, lon);
          document.getElementById('gpsStatus').textContent = 'GPS Active ✓';
        }
      }
      
      // Alert Mode: Both buzzer and motor are ON
      if (buzzerOn && motorOn) {
        if (!isAlertMode) {
          enterAlertMode();
        }
      } else {
        if (isAlertMode) {
          exitAlertMode();
        }
      }
    })
    .catch(err => console.error('Status check error:', err));
}

function enterAlertMode() {
  isAlertMode = true;
  document.body.classList.add('alert-mode');
  document.getElementById('alertBanner').classList.add('show');
  document.getElementById('statusBadge').textContent = '⚠️ ALERT';
  document.getElementById('statusBadge').classList.add('danger');
  document.getElementById('gpsStatus').textContent = '🚨 EMERGENCY ALERT ACTIVE';
  
  initAudio();
  playAlertBeep();
  alertSoundInterval = setInterval(() => {
    playAlertBeep();
  }, 2000);
  
  console.log('ALERT MODE ACTIVATED');
}

function exitAlertMode() {
  isAlertMode = false;
  document.body.classList.remove('alert-mode');
  document.getElementById('alertBanner').classList.remove('show');
  document.getElementById('statusBadge').textContent = '● ACTIVE';
  document.getElementById('statusBadge').classList.remove('danger');
  document.getElementById('gpsStatus').textContent = 'GPS Active ✓';
  
  if (alertSoundInterval) {
    clearInterval(alertSoundInterval);
    alertSoundInterval = null;
  }
  
  console.log('Alert mode deactivated');
}

function refreshLog(){
  fetch('/log')
  .then(res => res.text())
  .then(data => {
    let box = document.getElementById('logbox');
    let entries = data.trim().split('\n').filter(line => line.length > 0);
    box.innerHTML = entries.map(entry => 
      '<div class="log-entry">' + entry + '</div>'
    ).join('');
    box.scrollTop = box.scrollHeight;
    
    let now = new Date();
    document.getElementById('lastUpdate').textContent = 
      'Updated: ' + now.toLocaleTimeString();
  })
  .catch(err => console.error('Log fetch error:', err));
}

// Check status every 500ms for quick response
setInterval(checkStatus, 500);
setInterval(refreshLog, 1000);

// Initialize on load
window.onload = function() {
  initMap();
  checkStatus();
  refreshLog();
};

function camOn(){
  fetch('/camOn')
    .then(() => {
      document.getElementById('camView').src = 'http://10.173.208.243:82';
      document.getElementById('camView').style.display = 'block';
      document.getElementById('noStream').style.display = 'none';
    })
    .catch(err => console.error('Camera ON error:', err));
}

function camOff(){
  fetch('/camOff')
    .then(() => {
      document.getElementById('camView').src = '';
      document.getElementById('camView').style.display = 'none';
      document.getElementById('noStream').style.display = 'block';
      document.getElementById('noStream').textContent = 'Camera Stopped';
    })
    .catch(err => console.error('Camera OFF error:', err));
}

// Enable audio on first user interaction
document.addEventListener('click', initAudio, { once: true });
</script>

</body>
</html>
)rawliteral";

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);

  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  sendLog("WiFi Connected!");
  sendLog("IP: " + WiFi.localIP().toString());
  sendLog("Dashboard Ready");

  server.on("/", [](){ server.send_P(200, "text/html", webpage); });
  server.on("/log", handleLog);
  server.on("/status", handleStatus);  // Returns GPS + status
  server.on("/gps", HTTP_POST, handleGpsPost);
  server.on("/camOn",  [](){ turnCameraOn();  server.send(200,"text/plain","OK"); });
  server.on("/camOff", [](){ turnCameraOff(); server.send(200,"text/plain","OK"); });

  server.begin();
}

// ================== LOOP ===================
void loop() {
  server.handleClient();

  unsigned long now = millis();
  int sensorValue = digitalRead(SENSOR_PIN);

  if (sensorValue == LOW) {
    if (!eyePreviouslyClosed) {
      eyeClosedStart = now;
      eyePreviouslyClosed = true;
      sendLog("Eye Closed");
    }

    unsigned long closedDuration = now - eyeClosedStart;

    if (closedDuration >= EYE_CLOSED_THRESHOLD && !buzzerOn) {
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerStart = now;
      buzzerOn = true;
      sendLog("Drowsiness Detected! Buzzer ON");
    }

    if (buzzerOn && !motorOn && (now - buzzerStart >= BUZZER_THRESHOLD)) {
      digitalWrite(MOTOR_PIN, HIGH);
      motorOn = true;
      sendLog("No Response! Motor ON + Camera ON");

      turnCameraOn();

      if (gpsReceived) {
        sendLog("Alert Lat=" + String(lastLat,6));
        sendLog("Alert Lon=" + String(lastLon,6));
      } else {
        sendLog("GPS Missing at Alert!");
      }
    }

  } else {
    if (eyePreviouslyClosed) {
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(MOTOR_PIN, LOW);

      buzzerOn = false;
      motorOn = false;
      eyePreviouslyClosed = false;

      turnCameraOff();
      sendLog("Eye Open — System Reset");
    }
  }

  delay(5);
}
