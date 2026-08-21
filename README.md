# ColdGuard: IoT-Based Cold Storage Monitoring System

## 📌 Project Overview

ColdGuard is an IoT-based Cold Storage Monitoring System developed using the LPC2148 ARM7 microcontroller and ESP-01 Wi-Fi module. The system continuously monitors temperature, humidity, and cold storage door status, uploads sensor data to the ThingSpeak cloud, and generates alerts whenever environmental conditions exceed predefined thresholds.

---

## 🎯 Objectives

- Monitor temperature in real time.
- Monitor humidity in real time.
- Detect door open/close status.
- Upload data to the cloud using ESP-01.
- Generate buzzer alerts for abnormal conditions.
- Store configurable set points in EEPROM.

---

## 📷 Block Diagram

![Block Diagram](images/Block%20Diagram.jpeg)

---

## 🛠 Hardware Requirements

- LPC2148 ARM7 Microcontroller
- DHT11 Temperature & Humidity Sensor
- AT24C256 EEPROM
- 4×4 Matrix Keypad
- 16×2 LCD Display
- ESP-01 Wi-Fi Module
- Buzzer
- Door Switch
- DB-9 Cable / USB-to-UART Converter

---

## 💻 Software Requirements

- Keil µVision
- Embedded C
- Flash Magic

---

## ✨ Features

- Real-time temperature monitoring
- Humidity monitoring
- Door status monitoring
- ThingSpeak cloud integration
- EEPROM-based configuration storage
- LCD display
- Buzzer alert system
- Interrupt-driven menu system

---

## ⚙️ Working Principle

1. DHT11 measures temperature and humidity.
2. LPC2148 processes sensor data.
3. Current readings are displayed on the LCD.
4. If values exceed the configured threshold, data is uploaded to ThingSpeak using the ESP-01 module.
5. A buzzer alerts nearby users.
6. Door status is monitored continuously.
7. User-defined set points are stored in EEPROM to retain settings after power loss.

---

## 📂 Project Structure

```text
ColdGuard-IoT-Cold-Storage-Monitoring/
├── src/
├── include/
├── images/
└── README.md
```

---

## 🚀 Future Improvements

- Mobile application integration
- Email and SMS notifications
- Firebase cloud support
- AI-based anomaly detection
- Support for multiple cold storage units

---

## 📚 Learning Outcomes

Through this project, I gained practical experience in:

- Embedded C programming
- ARM7 (LPC2148) microcontroller programming
- DHT11 sensor interfacing
- EEPROM interfacing using I²C
- UART communication with ESP-01 Wi-Fi module
- LCD and keypad interfacing
- Interrupt handling
- IoT cloud integration using ThingSpeak
- Real-time monitoring and alert systems
- Embedded system debugging and testing
  
## 👨‍💻 Author

**Pavan Kumar Gotla**

- **GitHub:** [Pavan-Gotla](https://github.com/Pavan-Gotla)
- **LinkedIn:** [Pavan Kumar Gotla](https://www.linkedin.com/in/pavan-gotla-074a9a280)
