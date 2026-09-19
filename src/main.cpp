#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi credentials
const char* ssid = "corp-wifi";
const char* password = "1nf1ni8m@InF0r";

// Web server on port 80
WebServer server(80);

// PCA9685 setup (default address 0x40)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// Depending on your servo make, the pulse width min and max may vary, you 
// want these to be as small/large as possible without hitting the hard stop
// for max range. You'll have to tweak them as necessary to match the servos you have!
#define SERVOMIN  150 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // This is the 'maximum' pulse length count (out of 4096)
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

// OLED setup
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Store last command to display
int lastServo = -1;
int lastAngle = 90;
int servoMinUs[16] = {500,500,500,500,500,500,500,500,500,500,500,500,500,500,500,500};
int masterLink[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
int servoMaxUs[16] = {2400,2400,2400,2400,2400,2400,2400,2400,2400,2400,2400,2400,2400,2400,2400,2400};

// HTML content for the modern web UI
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Servo Tester</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Inter', 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #121212; color: #e0e0e0; margin: 0; padding: 20px; text-align: center; }
    h1 { color: #bb86fc; margin-bottom: 30px; font-weight: 300; letter-spacing: 1px; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px; max-width: 1200px; margin: 0 auto; }
    .card { background-color: #1e1e1e; border-radius: 16px; padding: 25px; box-shadow: 0 10px 20px rgba(0,0,0,0.4); transition: transform 0.3s cubic-bezier(0.175, 0.885, 0.32, 1.275); }
    .card:hover { transform: translateY(-5px); box-shadow: 0 15px 25px rgba(0,0,0,0.5); }
    .card h2 { margin-top: 0; color: #03dac6; font-size: 1.4rem; font-weight: 400; }
    .slider { -webkit-appearance: none; width: 100%; height: 6px; border-radius: 3px; background: #333; outline: none; margin: 25px 0; }
    .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 20px; height: 20px; border-radius: 50%; background: #bb86fc; cursor: pointer; transition: background 0.2s, transform 0.1s; }
    .slider::-webkit-slider-thumb:hover { background: #9965f4; transform: scale(1.2); }
    .value-input { width: 75px; font-size: 1.5rem; font-weight: bold; color: #cf6679; background: #222; border: 1px solid #444; border-radius: 4px; text-align: center; padding: 4px; }
    .value-input:focus { outline: none; border-color: #bb86fc; }
    .step-group { display: flex; align-items: center; justify-content: center; gap: 10px; margin: 15px 0; }
    .btn-group { display: flex; justify-content: space-between; margin-top: 15px; }
    .btn { background-color: #333; color: #e0e0e0; border: none; padding: 8px 12px; border-radius: 6px; cursor: pointer; transition: background 0.2s, transform 0.1s; font-size: 0.9rem; }
    .btn:hover { background-color: #03dac6; color: #121212; transform: translateY(-2px); }
    .btn:active { transform: translateY(0); }
    .mode-toggle { display: flex; justify-content: center; gap: 10px; margin-bottom: 15px; }
    .mode-btn { background-color: #333; color: #888; border: 1px solid #444; padding: 5px 15px; border-radius: 15px; cursor: pointer; font-size: 0.8rem; transition: 0.2s; }
    .mode-btn.active { background-color: #bb86fc; color: #121212; border-color: #bb86fc; font-weight: bold; }
    .panel { display: none; }
    .panel.active { display: block; }
    .cal-label { font-size: 0.8rem; color: #888; text-align: left; margin-top: 10px; }
    .master-select { margin-bottom: 15px; text-align: left; }
    .master-select label { font-size: 0.8rem; color: #888; margin-right: 5px; }
    .master-select select { background-color: #333; color: #fff; border: 1px solid #555; border-radius: 4px; padding: 4px; }
  </style>
  <script>
    var lastSendTimes = {};
    var pendingRequests = {};
    
    function updateMaster(servoId, masterId) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/setMaster?servo=" + servoId + "&master=" + masterId, true);
      xhr.send();
    }

    function updateServo(servoId) {
      var slider = document.getElementById("slider" + servoId);
      var val = slider.value;
      document.getElementById("val" + servoId).value = val;
      
      // Update UI for any followers
      for(var i=0; i<16; i++) {
        var sel = document.getElementById("masterSelect" + i);
        if(sel && sel.value == servoId) {
          document.getElementById("slider" + i).value = val;
          document.getElementById("val" + i).value = val;
        }
      }
      
      var now = Date.now();
      if (!lastSendTimes[servoId]) lastSendTimes[servoId] = 0;
      
      if (now - lastSendTimes[servoId] > 50) { // Limit to 20 requests per second per servo
        sendRequest(servoId, val);
      } else {
        if (pendingRequests[servoId]) clearTimeout(pendingRequests[servoId]);
        pendingRequests[servoId] = setTimeout(function() {
          sendRequest(servoId, val);
        }, 50);
      }
    }
    
    function sendRequest(servoId, val) {
      lastSendTimes[servoId] = Date.now();
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/set?servo=" + servoId + "&angle=" + val, true);
      xhr.send();
    }
    
    function setAngle(servoId, val) {
      var slider = document.getElementById("slider" + servoId);
      slider.value = val;
      updateServo(servoId);
    }
    
    function stepAngle(servoId, amount) {
      var slider = document.getElementById("slider" + servoId);
      var newVal = parseInt(slider.value) + amount;
      if (newVal < 0) newVal = 0;
      if (newVal > 180) newVal = 180;
      setAngle(servoId, newVal);
    }
    
    function toggleMode(servoId, mode) {
      document.getElementById("btn-angle-" + servoId).classList.remove("active");
      document.getElementById("btn-cal-" + servoId).classList.remove("active");
      document.getElementById("panel-angle-" + servoId).classList.remove("active");
      document.getElementById("panel-cal-" + servoId).classList.remove("active");
      document.getElementById("btn-" + mode + "-" + servoId).classList.add("active");
      document.getElementById("panel-" + mode + "-" + servoId).classList.add("active");
    }
    
    function updateUs(servoId, type) {
      var slider = document.getElementById(type + "Slider" + servoId);
      var val = slider.value;
      document.getElementById(type + "Val" + servoId).value = val;
      var now = Date.now();
      if (now - lastSendTime > 50) {
        sendUsRequest(servoId, val);
      } else {
        clearTimeout(pendingRequest);
        pendingRequest = setTimeout(function() { sendUsRequest(servoId, val); }, 50);
      }
    }
    
    function sendUsRequest(servoId, val) {
      lastSendTime = Date.now();
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/setUs?servo=" + servoId + "&us=" + val, true);
      xhr.send();
    }

    function stepUs(servoId, type, amount) {
      var slider = document.getElementById(type + "Slider" + servoId);
      var newVal = parseInt(slider.value) + amount;
      var minLimit = parseInt(slider.min);
      var maxLimit = parseInt(slider.max);
      if (newVal < minLimit) newVal = minLimit;
      if (newVal > maxLimit) newVal = maxLimit;
      slider.value = newVal;
      updateUs(servoId, type);
    }
    
    function setUsDirect(servoId, type, val) {
      var slider = document.getElementById(type + "Slider" + servoId);
      slider.value = val;
      updateUs(servoId, type);
    }
    
    function saveCalibration(servoId) {
      var minUs = document.getElementById("minSlider" + servoId).value;
      var maxUs = document.getElementById("maxSlider" + servoId).value;
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/calibrate?servo=" + servoId + "&minUs=" + minUs + "&maxUs=" + maxUs, true);
      xhr.send();
      toggleMode(servoId, 'angle');
      updateServo(servoId);
    }
  </script>
</head>
<body>
  <h1>PCA9685 12-Servo Array</h1>
  <div class="grid">
)rawliteral";

void handleRoot() {
  String html = String(index_html);
  
  // Generate 16 servo sliders dynamically
  for (int i = 0; i < 16; i++) {
    String id = String(i);
    int currentMin = servoMinUs[i];
    int currentMax = servoMaxUs[i];
    html += "<div class=\"card\"><h2>Servo " + id + "</h2>";
    
    html += "<div class=\"mode-toggle\">";
    html += "<button id=\"btn-angle-" + id + "\" class=\"mode-btn active\" onclick=\"toggleMode(" + id + ", 'angle')\">Angle</button>";
    html += "<button id=\"btn-cal-" + id + "\" class=\"mode-btn\" onclick=\"toggleMode(" + id + ", 'cal')\">Calibrate</button>";
    html += "</div>";

    html += "<div class=\"master-select\">";
    html += "<label>Follow:</label>";
    html += "<select id=\"masterSelect" + id + "\" onchange=\"updateMaster(" + id + ", this.value)\">";
    html += "<option value=\"-1\">None</option>";
    for(int m=0; m<16; m++) {
      if (m != i) {
        html += "<option value=\"" + String(m) + "\">Servo " + String(m) + "</option>";
      }
    }
    html += "</select></div>";

    html += "<div id=\"panel-angle-" + id + "\" class=\"panel active\">";
    html += "<input type=\"range\" min=\"0\" max=\"180\" value=\"90\" class=\"slider\" id=\"slider" + id + "\" oninput=\"updateServo(" + id + ")\">";
    html += "<div class=\"step-group\">";
    html += "<button class=\"btn\" onclick=\"stepAngle(" + id + ", -5)\">&lt;&lt;</button>";
    html += "<button class=\"btn\" onclick=\"stepAngle(" + id + ", -1)\">&lt;</button>";
    html += "<input type=\"number\" min=\"0\" max=\"180\" class=\"value-input\" id=\"val" + id + "\" value=\"90\" onchange=\"setAngle(" + id + ", this.value)\">";
    html += "<button class=\"btn\" onclick=\"stepAngle(" + id + ", 1)\">&gt;</button>";
    html += "<button class=\"btn\" onclick=\"stepAngle(" + id + ", 5)\">&gt;&gt;</button>";
    html += "</div>";
    html += "<div class=\"btn-group\">";
    html += "<button class=\"btn\" onclick=\"setAngle(" + id + ", 0)\">0&deg;</button>";
    html += "<button class=\"btn\" onclick=\"setAngle(" + id + ", 45)\">45&deg;</button>";
    html += "<button class=\"btn\" onclick=\"setAngle(" + id + ", 90)\">90&deg;</button>";
    html += "<button class=\"btn\" onclick=\"setAngle(" + id + ", 135)\">135&deg;</button>";
    html += "<button class=\"btn\" onclick=\"setAngle(" + id + ", 180)\">180&deg;</button>";
    html += "</div></div>";

    html += "<div id=\"panel-cal-" + id + "\" class=\"panel\">";
    html += "<div class=\"cal-label\">Min Pulse (us) - 0&deg;</div>";
    html += "<input type=\"range\" min=\"400\" max=\"1000\" value=\"" + String(currentMin) + "\" class=\"slider\" id=\"minSlider" + id + "\" oninput=\"updateUs(" + id + ", 'min')\">";
    html += "<div class=\"step-group\">";
    html += "<button class=\"btn\" onclick=\"stepUs(" + id + ", 'min', -10)\">&lt;&lt;</button>";
    html += "<button class=\"btn\" onclick=\"stepUs(" + id + ", 'min', -1)\">&lt;</button>";
    html += "<input type=\"number\" min=\"400\" max=\"1000\" class=\"value-input\" id=\"minVal" + id + "\" value=\"" + String(currentMin) + "\" onchange=\"setUsDirect(" + id + ", 'min', this.value)\">";
    html += "<button class=\"btn\" onclick=\"stepUs(" + id + ", 'min', 1)\">&gt;</button>";
    html += "<button class=\"btn\" onclick=\"stepUs(" + id + ", 'min', 10)\">&gt;&gt;</button>";
    html += "</div>";
    
    html += "<div class=\"cal-label\">Max Pulse (us) - 180&deg;</div>";
    html += "<input type=\"range\" min=\"2000\" max=\"3000\" value=\"" + String(currentMax) + "\" class=\"slider\" id=\"maxSlider" + id + "\" oninput=\"updateUs(" + id + ", 'max')\">";
    html += "<div class=\"step-group\">";
    html += "<button class=\"btn\" onclick=\"stepUs(" + id + ", 'max', -10)\">&lt;&lt;</button>";
    html += "<button class=\"btn\" onclick=\"stepUs(" + id + ", 'max', -1)\">&lt;</button>";
    html += "<input type=\"number\" min=\"2000\" max=\"3000\" class=\"value-input\" id=\"maxVal" + id + "\" value=\"" + String(currentMax) + "\" onchange=\"setUsDirect(" + id + ", 'max', this.value)\">";
    html += "<button class=\"btn\" onclick=\"stepUs(" + id + ", 'max', 1)\">&gt;</button>";
    html += "<button class=\"btn\" onclick=\"stepUs(" + id + ", 'max', 10)\">&gt;&gt;</button>";
    html += "</div>";
    
    html += "<button class=\"btn\" style=\"width: 100%; margin-top: 10px; background-color: #03dac6; color: #121212;\" onclick=\"saveCalibration(" + id + ")\">Save Calibration</button>";
    html += "</div></div>";
  }
  
  html += R"rawliteral(
  </div>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void updateDisplay() {
  display.clearDisplay();
  
  // Show WiFi Status and IP
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  if(WiFi.status() == WL_CONNECTED) {
    display.print("IP: ");
    display.println(WiFi.localIP());
  } else {
    display.println("Disconnected");
  }
  
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
  
  if (lastServo != -1) {
    // Show text info (Large)
    display.setCursor(0, 16);
    display.setTextSize(2);
    display.print("S");
    display.print(lastServo);
    
    // Right align the angle
    display.setCursor(76, 16);
    if (lastAngle == -1) {
      display.print("CAL");
    } else {
      display.print(lastAngle);
      display.print((char)247); // degree symbol
    }
    
    // Draw Gauge
    int cx = 64; // Center x
    int cy = 60; // Center y
    int r = 24;  // Radius
    
    // Draw half-circle gauge background
    display.drawCircle(cx, cy, r, SSD1306_WHITE);
    // Erase bottom half of the circle
    display.fillRect(0, cy, SCREEN_WIDTH, SCREEN_HEIGHT - cy, SSD1306_BLACK);
    // Draw baseline
    display.drawLine(cx - r - 5, cy, cx + r + 5, cy, SSD1306_WHITE);
    
    // Draw 0, 90, 180 tick marks
    display.drawLine(cx - r, cy, cx - r + 3, cy, SSD1306_WHITE); // 0
    display.drawLine(cx + r, cy, cx + r - 3, cy, SSD1306_WHITE); // 180
    display.drawLine(cx, cy - r, cx, cy - r + 3, SSD1306_WHITE); // 90
    
    // Draw needle
    float rad = (180 - lastAngle) * PI / 180.0;
    int nx = cx + (r - 2) * cos(rad);
    int ny = cy - (r - 2) * sin(rad);
    display.drawLine(cx, cy, nx, ny, SSD1306_WHITE);
    
    // Draw center hub
    display.fillCircle(cx, cy, 2, SSD1306_WHITE);
  } else {
    display.setCursor(0, 30);
    display.setTextSize(2);
    display.println("Ready");
  }
  
  display.display();
}

void setCalibratedAngle(uint8_t channel, float angle) {
  int minUs = servoMinUs[channel];  
  int maxUs = servoMaxUs[channel]; 

  // Constrain the incoming angle strictly between 0.0 and 180.0
  angle = fmax(0.0f, fmin(180.0f, angle));

  // Precise float mapping to microseconds
  int pulseUs = (int)(angle * (maxUs - minUs) / 180.0f + minUs);

  // Send precise timings to PCA9685
  pwm.writeMicroseconds(channel, pulseUs);
}

void handleSetUs() {
  if (server.hasArg("servo") && server.hasArg("us")) {
    int servoNum = server.arg("servo").toInt();
    int us = server.arg("us").toInt();
    if (servoNum >= 0 && servoNum < 16 && us >= 0 && us <= 5000) {
      pwm.writeMicroseconds(servoNum, us);
      lastServo = servoNum;
      lastAngle = -1; // Indicate CAL mode on OLED
      updateDisplay();
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}

void handleCalibrate() {
  if (server.hasArg("servo") && server.hasArg("minUs") && server.hasArg("maxUs")) {
    int servoNum = server.arg("servo").toInt();
    int minUs = server.arg("minUs").toInt();
    int maxUs = server.arg("maxUs").toInt();
    if (servoNum >= 0 && servoNum < 16) {
      servoMinUs[servoNum] = minUs;
      servoMaxUs[servoNum] = maxUs;
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}

void handleSetMaster() {
  if (server.hasArg("servo") && server.hasArg("master")) {
    int servoNum = server.arg("servo").toInt();
    int masterNum = server.arg("master").toInt();
    if (servoNum >= 0 && servoNum < 16 && masterNum >= -1 && masterNum < 16) {
      masterLink[servoNum] = masterNum;
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}

void handleSet() {
  if (server.hasArg("servo") && server.hasArg("angle")) {
    int servoNum = server.arg("servo").toInt();
    int angle = server.arg("angle").toInt();
    
    // Ensure bounds
    if (servoNum >= 0 && servoNum < 16 && angle >= 0 && angle <= 180) {
      setCalibratedAngle(servoNum, angle);
      
      // Also update any followers physically
      for(int i=0; i<16; i++) {
        if(masterLink[i] == servoNum) {
          setCalibratedAngle(i, angle);
        }
      }
      
      // Update OLED
      lastServo = servoNum;
      lastAngle = angle;
      updateDisplay();
      
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}

void setup() {
  Serial.begin(115200);

  // Initialize I2C pins D6=SDA, D7=SCL
  Wire.begin(21, 22);

  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Initializing...");
  display.display();

  // Initialize PCA9685
  pwm.begin();
  //pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  // Set all 16 servos to 90 degrees initially
  uint16_t midPulse = map(90, 0, 180, SERVOMIN, SERVOMAX);
  for(int i=0; i<16; i++){
    pwm.setPWM(i, 0, midPulse);
  }

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Connecting to:");
  display.println(ssid);
  display.display();
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup Web Server Routes
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/setUs", handleSetUs);
  server.on("/setMaster", handleSetMaster);
  server.on("/calibrate", handleCalibrate);
  server.begin();
  Serial.println("HTTP server started");

  updateDisplay();
}

void loop() {
  server.handleClient();
}