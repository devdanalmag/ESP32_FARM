#ifndef CONFIG_H
#define CONFIG_H

// ==========================================
//  ESP32 FARM DATA COLLECTION SYSTEM
//  Configuration File
// ==========================================

// ---------- WiFi Credentials ----------
#define WIFI_SSID "GIDAN MAASHALLAH"
#define WIFI_PASSWORD "@Mal2k7mal2k7"
#define WIFI_TIMEOUT 10000 // ms to wait for WiFi connection

// ---------- Server URL (XAMPP) ----------
#define SERVER_URL "http://192.168.1.66/esp32_farm/web/api/sync.php"
#define SYNC_CHECK_URL "http://192.168.1.66/esp32_farm/web/api/trigger_sync.php"

// ---------- I2C LCD (16x2) ----------
#define LCD_ADDR 0x27 // I2C address (try 0x3F if 0x27 doesn't work)
#define LCD_COLS 16
#define LCD_ROWS 2

// ---------- 4x4 Keypad ----------
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

const byte ROW_PINS[KEYPAD_ROWS] = {12, 13, 32, 33};
const byte COL_PINS[KEYPAD_COLS] = {25, 26, 27, 14};

const char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {{'1', '2', '3', 'A'},
                                                    {'4', '5', '6', 'B'},
                                                    {'7', '8', '9', 'C'},
                                                    {'*', '0', '#', 'D'}};

// ---------- SD Card (VSPI) ----------
#define SD_CS_PIN 2
// MOSI=23, MISO=19, SCK=18 (default VSPI)

// ---------- MAX485 / RS485 ----------
#define RS485_DE_PIN 15 // Driver Enable
#define RS485_RE_PIN 5  // Receiver Enable
#define RS485_TX_PIN 4  // Serial2 TX → MAX485 DI
#define RS485_RX_PIN 0  // Serial2 RX → MAX485 RO
#define RS485_BAUD 4800

// ---------- Soil Sensor (Modbus RTU) ----------
#define SENSOR_ADDR 0x01
#define SENSOR_NUM_REGS 7      // 7 parameters to read
#define SENSOR_TIMEOUT_MS 1500 // timeout waiting for response
#define NUM_SAMPLES 5          // samples to average

// ---------- SD Card File Paths ----------
#define FARMERS_FILE "/farmers.csv"
#define DATALOG_FILE "/datalog.csv"

// ---------- Farmer ID ----------
#define FARMER_ID_LENGTH 4 // 4-digit IDs: 0001-9999

// ---------- DS3231 RTC Module (I2C) ----------
// Shares I2C bus with LCD: SDA=21, SCL=22 (ESP32 default)
// No extra pin defines needed

// ---------- SIM800L GSM Module (UART1) ----------
#define GSM_TX_PIN 17 // ESP32 TX → SIM800L RX
#define GSM_RX_PIN 16 // ESP32 RX ← SIM800L TX
#define GSM_BAUD 9600
#define SMS_CONFIG_FILE "/sms_config.txt"

// ---------- Timing ----------
#define SENSOR_READ_DELAY 1000 // ms between sensor readings
#define DEBOUNCE_DELAY 200     // ms keypad debounce
#define LCD_SCROLL_DELAY 2000  // ms for scrolling messages

#endif // CONFIG_H
