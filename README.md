# PCA9685 Servo Tester 🛠️

A modern, web-based, 16-channel servo tester built for the **ESP32** using the **PCA9685** PWM driver and an **SSD1306 OLED screen**. 

This firmware hosts an interactive web dashboard directly from the ESP32, allowing you to manually control, calibrate, and fine-tune up to 16 servos instantly from your phone or PC over WiFi.

## Features ✨
- **16-Channel Support**: Controls all 16 ports on the PCA9685 independently.
- **Real-Time Web Dashboard**: A sleek, dark-themed responsive UI hosted by the ESP32.
- **Ultra-Precise Control**: 
  - Drag the sliders to move the servos in real-time.
  - Use `<< < > >>` buttons for precise 1° or 5° adjustments.
  - Click on the angle textbox to manually type in an exact target angle.
  - Quick-jump preset buttons for `0°`, `45°`, `90°`, `135°`, and `180°`.
- **OLED Display Output**: The connected SSD1306 OLED dynamically shows the device's IP address, the active servo index, and a graphical sweeping gauge of the current angle.

## Hardware Requirements 🔌
- **ESP32** (e.g. DEVKIT V1)
- **PCA9685** 16-Channel 12-bit PWM Servo Driver
- **SSD1306 OLED** Display (128x64)
- **Servos** (e.g. MG90S)
- Breadboard & Jumper Wires

### Pin Connections (I2C)
Both the PCA9685 and the SSD1306 communicate over the same standard ESP32 I2C bus:
- **SDA**: D21
- **SCL**: D22

## Setup Instructions 🚀

1. Clone this repository.
2. Open the project in **PlatformIO**.
3. Open `src/main.cpp` and update the WiFi credentials to match your local network:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
4. Build and upload the firmware to your ESP32.
5. Once booted, the OLED screen will display the assigned local **IP Address**.
6. Type that IP Address into a web browser on a device connected to the same WiFi network to access the control panel.

## License
MIT License
