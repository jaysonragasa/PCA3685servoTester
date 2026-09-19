#pragma once

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
      var minVal = parseInt(document.getElementById("minSlider" + servoId).value);
      var maxVal = parseInt(document.getElementById("maxSlider" + servoId).value);
      var centerVal = Math.round((minVal + maxVal) / 2);
      var centerEl = document.getElementById("centerVal" + servoId);
      if(centerEl) centerEl.innerText = centerVal;
      var now = Date.now();
      if (!lastSendTimes[servoId]) lastSendTimes[servoId] = 0;
      if (now - lastSendTimes[servoId] > 50) {
        sendUsRequest(servoId, val);
      } else {
        if (pendingRequests[servoId]) clearTimeout(pendingRequests[servoId]);
        pendingRequests[servoId] = setTimeout(function() { sendUsRequest(servoId, val); }, 50);
      }
    }
    
    function sendUsRequest(servoId, val) {
      lastSendTimes[servoId] = Date.now();
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
    

    function centerServo(servoId) {
      var minVal = parseInt(document.getElementById("minSlider" + servoId).value);
      var maxVal = parseInt(document.getElementById("maxSlider" + servoId).value);
      var centerVal = Math.round((minVal + maxVal) / 2);
      var centerEl = document.getElementById("centerVal" + servoId);
      if(centerEl) centerEl.innerText = centerVal;
      sendUsRequest(servoId, centerVal);
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
