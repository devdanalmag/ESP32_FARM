# ESP32 Farm Data Collection System 🌱

A complete **IoT soil monitoring system** using an ESP32 microcontroller. Collects 7 soil parameters from farmers' fields, stores data locally on an SD card, syncs to a web dashboard over WiFi, and sends SMS results to farmers via a GSM module.

---

## 📋 Table of Contents

1. [Features](#features)
2. [Hardware Required](#hardware-required)
3. [Wiring Diagram](#wiring-diagram)
4. [Software Requirements](#software-requirements)
5. [Step-by-Step Setup](#step-by-step-setup)
6. [How It Works](#how-it-works)
7. [SMS Configuration](#sms-configuration)
8. [Project Structure](#project-structure)
9. [Troubleshooting](#troubleshooting)

---

## ✨ Features

- **7-in-1 Soil Sensor** — Reads Nitrogen, Phosphorus, Potassium, pH, EC, Temperature, and Humidity via Modbus RTU
- **Farmer Management** — Register farmers with ID and phone number; data stored on SD card
- **SD Card Logging** — All readings saved locally in CSV format with timestamps
- **WiFi Sync** — Upload collected data to a MySQL database via REST API
- **Web Dashboard** — Visualize soil data with charts, manage farmers, and configure settings
- **SMS Notifications** — Send soil results to farmers via SIM800L GSM module
- **Configurable SMS Templates** — Set message templates with placeholders from the dashboard
- **RTC Time Sync** — DS3231 real-time clock auto-syncs from server time during WiFi sync
- **16x2 LCD Display** — Real-time feedback with menu navigation
- **4x4 Keypad Input** — Enter farmer IDs, phone numbers, and navigate menus

---

## 🔧 Hardware Required

| Component | Model | Qty | Notes |
|-----------|-------|-----|-------|
| Microcontroller | ESP32 Dev Board | 1 | Any ESP32 with dual UART |
| Soil Sensor | NPKPHCTH-S (7-in-1) | 1 | Modbus RTU, RS485 |
| RS485 Module | MAX485 / HW-097 | 1 | TTL to RS485 converter |
| LCD Display | 16x2 I2C (PCF8574) | 1 | I2C address 0x27 |
| Keypad | 4x4 Matrix Membrane | 1 | 0-9, A-D, *, # |
| SD Card Module | SPI SD Reader | 1 | Micro SD with adapter |
| RTC Module | DS3231 | 1 | I2C, battery backup |
| GSM Module | SIM800L | 1 | With SIM card inserted |
| Power Supply | 5V 3A | 1 | USB or barrel jack |
| GSM Power | 3.7-4.2V 2A | 1 | Separate supply for SIM800L |

---

## 🔌 Wiring Diagram

### ESP32 Pin Connections

```
ESP32 Pin    →    Component
─────────────────────────────────
GPIO 13      →    Keypad Row 1
GPIO 12      →    Keypad Row 2
GPIO 14      →    Keypad Row 3
GPIO 27      →    Keypad Row 4
GPIO 26      →    Keypad Col 1
GPIO 25      →    Keypad Col 2
GPIO 33      →    Keypad Col 3
GPIO 32      →    Keypad Col 4
─────────────────────────────────
GPIO 21 (SDA) →   LCD SDA + RTC SDA (shared I2C)
GPIO 22 (SCL) →   LCD SCL + RTC SCL (shared I2C)
─────────────────────────────────
GPIO 4  (TX2) →   MAX485 DI (RS485 TX)
GPIO 2  (RX2) →   MAX485 RO (RS485 RX)
GPIO 15      →    MAX485 DE + RE (direction)
─────────────────────────────────
GPIO 5  (SS)  →   SD Card CS
GPIO 18 (SCK) →   SD Card SCK
GPIO 23 (MOSI)→   SD Card MOSI
GPIO 19 (MISO)→   SD Card MISO
─────────────────────────────────
GPIO 17 (TX1) →   SIM800L RX
GPIO 16 (RX1) →   SIM800L TX
─────────────────────────────────
```

> ⚠️ **SIM800L Power**: The SIM800L needs 3.7–4.2V at up to 2A peak. Do **NOT** power it from the ESP32's 3.3V pin. Use a separate LiPo battery or a buck converter.

---

## 💻 Software Requirements

### For ESP32 (Arduino IDE)

1. **Arduino IDE** 2.x or later — [Download](https://www.arduino.cc/en/software)
2. **ESP32 Board Package** — Add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to Board Manager URLs
3. **Libraries** (install via Library Manager):
   - `LiquidCrystal_I2C` by Frank de Brabander
   - `Keypad` by Mark Stanley & Alexander Brevig
   - `ArduinoJson` by Benoît Blanchon (v7+)
   - `RTClib` by Adafruit

### For Web Dashboard

1. **XAMPP** (or any Apache + PHP + MySQL stack) — [Download](https://www.apachefriends.org/)
2. A web browser (Chrome, Firefox, Edge)

---

## 🚀 Step-by-Step Setup

### Step 1: Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/ESP32_FARM.git
```

### Step 2: Set Up the Database

1. Start **XAMPP** and enable **Apache** and **MySQL**
2. Open **phpMyAdmin** at `http://localhost/phpmyadmin`
3. Create a new database called `esp32_farm`
4. Import the SQL file:
   - Click the `esp32_farm` database
   - Go to **Import** tab
   - Select `web/db_setup.sql`
   - Click **Go**

### Step 3: Configure the Web Server

1. Copy the entire project folder to your XAMPP web root:
   ```
   C:\xampp\htdocs\ESP32_FARM\
   ```
2. Edit `web/config.php` and set your database credentials:
   ```php
   define('DB_HOST', 'localhost');
   define('DB_NAME', 'esp32_farm');
   define('DB_USER', 'root');
   define('DB_PASS', '');
   ```
3. Open the dashboard at: `http://localhost/ESP32_FARM/web/`

### Step 4: Configure the ESP32 Firmware

1. Open `ESP32_FARM/ESP32_FARM.ino` in Arduino IDE
2. Edit `config.h` and update:
   ```cpp
   // Your WiFi credentials
   #define WIFI_SSID     "YourWiFiName"
   #define WIFI_PASSWORD "YourWiFiPassword"

   // Your server IP (computer running XAMPP)
   #define SERVER_URL    "http://192.168.1.100/ESP32_FARM/web/api/sync.php"
   #define SYNC_CHECK_URL "http://192.168.1.100/ESP32_FARM/web/api/trigger_sync.php"
   ```
3. Select Board: **ESP32 Dev Module**
4. Select the correct COM port
5. Click **Upload**

### Step 5: Insert SD Card

- Format a micro SD card as **FAT32**
- Insert it into the SD card module
- The ESP32 will automatically create the required CSV files on first boot

### Step 6: Insert SIM Card (for SMS)

- Insert an active SIM card into the SIM800L module
- Make sure the SIM has SMS credit / balance
- Power the SIM800L with a 3.7–4.2V supply

### Step 7: Power On & Test

1. Power on the ESP32
2. You should see on the LCD:
   ```
   FARM DATA
   COLLECTOR v1.0
   ```
3. Then GSM status:
   ```
   GSM: Connected
   SIM800L OK
   ```
4. Then WiFi connection attempt and sync prompt

---

## 🔄 How It Works

### System Flow

```
Boot → GSM Check → WiFi Connect → Sync Prompt
                                        ↓
                                   Main Menu
                                   ┌─────────────────┐
                                   │ F:3  L:12  G    │
                                   │ A:Sync  *:Start │
                                   └─────────────────┘
                                        ↓
                        ┌───── Press A ──┴── Press * ─────┐
                        ↓                                  ↓
                   WiFi Sync                         Enter Farmer ID
                   (send data,                            ↓
                    get SMS config,               Farmer Found / New
                    update RTC)                          ↓
                                                  Read Soil Sensor
                                                   (5 samples avg)
                                                        ↓
                                                  Show Results
                                                        ↓
                                                  Save to SD Card
                                                        ↓
                                                  Send SMS (if enabled)
                                                        ↓
                                                  Back to Main Menu
```

### Keypad Controls

| Key | Function |
|-----|----------|
| `0-9` | Enter digits (ID, phone number) |
| `*` | Confirm / Save / Sync |
| `#` | Cancel / Back |
| `A` | Sync menu (from main screen) / Backspace (during input) |
| `B-D` | Backspace (during input) / Next page (results) |

### WiFi Sync Details

During a sync, the ESP32:
1. **Uploads** farmer data and soil readings (CSV) to the server
2. **Receives** SMS settings (enabled/disabled + message template)
3. **Receives** server time and updates the DS3231 RTC module

---

## 📱 SMS Configuration

### From the Dashboard

1. Open `http://localhost/ESP32_FARM/web/`
2. Scroll down to **SMS Settings** panel
3. Toggle SMS **On/Off**
4. Edit the message template using placeholders:

| Placeholder | Replaced With |
|-------------|---------------|
| `{farmer_id}` | Farmer's ID number |
| `{humidity}` | Soil moisture (%) |
| `{temperature}` | Soil temperature (°C) |
| `{ph}` | pH level |
| `{ec}` | Electrical conductivity |
| `{nitrogen}` | Nitrogen content (mg/kg) |
| `{phosphorus}` | Phosphorus content (mg/kg) |
| `{potassium}` | Potassium content (mg/kg) |
| `{timestamp}` | Date and time of reading |

5. Click **Save Settings**
6. Next time the ESP32 syncs, it will download the new template

### Example SMS

```
Farm Report for ID:0001
Moisture:45.2%
Temp:28.3C
pH:6.8
EC:450
N:120 P:85 K:200
Date:2026-02-25 21:30:00
```

---

## 📁 Project Structure

```
ESP32_FARM/
├── ESP32_FARM/                 # Arduino firmware
│   ├── ESP32_FARM.ino          # Main sketch (state machine)
│   ├── config.h                # Pin definitions, WiFi, constants
│   ├── keypad_manager.h        # 4x4 keypad input handling
│   ├── lcd_manager.h           # 16x2 LCD display functions
│   ├── rtc_manager.h           # DS3231 RTC time management
│   ├── sd_manager.h            # SD card read/write (CSV)
│   ├── sensor_manager.h        # Soil sensor (Modbus RTU / RS485)
│   ├── gsm_manager.h           # SIM800L SMS sending
│   └── wifi_sync.h             # WiFi + server sync
│
├── web/                        # PHP web dashboard
│   ├── index.html              # Dashboard UI
│   ├── app.js                  # Frontend JavaScript
│   ├── style.css               # Dashboard styling
│   ├── config.php              # Database configuration
│   ├── db_setup.sql            # MySQL schema
│   └── api/
│       ├── sync.php            # Data sync endpoint
│       ├── farmers.php         # Farmers CRUD API
│       ├── readings.php        # Soil readings API
│       ├── sms_settings.php    # SMS config API
│       └── trigger_sync.php    # Sync trigger API
│
└── README.md
```

---

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| LCD shows nothing | Check I2C address (try 0x27 or 0x3F). Run I2C scanner sketch. |
| Sensor not reading | Check RS485 wiring. Ensure DE/RE pin is connected. Check baud rate (4800). |
| SD Card fails | Format as FAT32. Check SPI wiring. Try a different SD card. |
| WiFi won't connect | Verify SSID/password in `config.h`. Ensure ESP32 is in range. |
| Sync fails | Check server IP in `config.h`. Ensure XAMPP Apache + MySQL are running. |
| SMS not sending | Check SIM card has credit. Verify SIM800L power (3.7-4.2V, 2A). Check wiring. |
| RTC shows wrong time | Sync with server (press A → * from main menu). Or re-upload firmware to set compile time. |
| GSM: Not Found | Verify TX/RX wiring (crossed: ESP32 TX→SIM RX). Check power supply. |

---

## 📄 License

This project is open source and available under the [MIT License](LICENSE).

---

## 🙏 Acknowledgments

- Built with Arduino IDE + XAMPP
- Soil sensor communication via Modbus RTU protocol
- Dashboard uses Chart.js for data visualization
