# ESP32 Drowsiness Detection System

A complete real-time drowsiness detection and alert system using ESP32, ESP32-CAM, IR eye-blink sensor, buzzer, vibration motor, GPS, and a web-based dashboard.
This project is designed to monitor a driver’s alertness and prevent accidents caused by fatigue by triggering alerts and sending live location and camera feed when the driver becomes unresponsive.

# Project Overview

Driver drowsiness is one of the major causes of road accidents. When a driver becomes sleepy, their eyes remain closed for longer than normal and their reaction time drops. This system continuously monitors the driver’s eye activity using an IR sensor and camera and triggers alerts if drowsiness is detected.
The system works in three stages:
Stage 1 – Buzzer alert when eyes stay closed too long
Stage 2 – Vibration motor activates if no response
Stage 3 – ESP32-CAM turns on, live video starts and GPS location is sent to the dashboard
A web dashboard allows real-time monitoring of system status, camera feed, and driver location.

# System Architecture

The system uses two microcontrollers:
ESP32 (Main Controller)
Handles sensor reading, buzzer, vibration motor, GPS, and web dashboard.
ESP32-CAM
Provides live video streaming of the driver.
Both devices are connected over Wi-Fi and communicate using HTTP requests.

# Block Diagram

The ESP32 receives data from the IR sensor and GPS module. It controls the buzzer, vibration motor, and sends camera ON/OFF commands to the ESP32-CAM.
The user accesses the system through a browser dashboard.
<img width="833" height="472" alt="image" src="https://github.com/user-attachments/assets/21c1d344-4fac-4c4a-a498-95721f796f80" />

# Flow of Operation

1.System starts and initializes all modules
2.IR sensor and camera input are continuously monitored
3.If eye closure exceeds threshold:
  Buzzer is triggered
4.If no response:
  Vibration motor turns ON
5.If still no response:
  Camera starts streaming
  GPS location is sent
6.System resets when the driver opens their eyes
<img width="692" height="632" alt="image" src="https://github.com/user-attachments/assets/2dd5ec8a-39c6-4b5c-ae4e-94b3f2dd3012" />

# Hardware Components

| Component              | Purpose              |
| ---------------------- | -------------------- |
| ESP32 Dev Board        | Main controller      |
| ESP32-CAM              | Live video streaming |
| IR Eye Blink Sensor    | Detects eye closure  |
| GPS Module (NEO-6M)    | Driver location      |
| Buzzer                 | Audio alert          |
| Vibration Motor        | Physical alert       |
| Battery / Power Supply | System power         |
| Breadboard & Wires     | Connections          |

# Hardware Setup

ESP32-CAM
<img width="254" height="379" alt="image" src="https://github.com/user-attachments/assets/ee8d9b16-52e8-4f1e-b57f-d1570dcd96cd" />
GPS Module
<img width="962" height="796" alt="image" src="https://github.com/user-attachments/assets/195592b3-8239-4ce5-a1c0-b17e778e5e8c" />
IR Sensor
<img width="296" height="313" alt="image" src="https://github.com/user-attachments/assets/bd86589f-3ef5-4e83-a221-6a823ad662e3" />
Buzzer and Vibration Motor
<img width="440" height="380" alt="image" src="https://github.com/user-attachments/assets/e988e9a9-4db4-487e-90f5-36717f6a4579" />
Complete Circuit
<img width="696" height="519" alt="image" src="https://github.com/user-attachments/assets/2e8adc29-edea-422f-8325-13ec821d5311" />
Web Dashboard
The ESP32 hosts a live dashboard that shows system logs, camera feed, and GPS location.
<img width="863" height="451" alt="image" src="https://github.com/user-attachments/assets/0a5b422b-ccfe-479a-8d46-5a0aa5e5f444" />
Live Monitoring
Stage-2 Alert (Vibration Motor)
<img width="870" height="460" alt="image" src="https://github.com/user-attachments/assets/eeb7dda4-50db-4a72-8768-cd886cca9aec" />
Stage-3 Emergency Mode (Live Camera)
GPS Location Tracking
<img width="807" height="420" alt="image" src="https://github.com/user-attachments/assets/a7119f09-57d8-47eb-9a95-fc658f08e4f9" />

# How the System Works

The IR sensor checks whether the driver’s eyes are open or closed.
If eyes stay closed longer than the set time, the buzzer is activated.
If the driver does not respond, the vibration motor turns on.
If there is still no response, the ESP32:
Turns on the ESP32-CAM
Streams live video
Sends GPS location to the dashboard
All events are logged on the dashboard in real time.

# Applications

Driver safety systems
Smart vehicles
Fleet monitoring
Accident prevention
Long-distance driving assistance

# Future Improvements

AI-based face and eye tracking
Mobile app integration
Cloud data logging
SMS and emergency calling
Infrared camera for night use
