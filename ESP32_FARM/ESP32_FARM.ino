// ==========================================
//  ESP32 FARM DATA COLLECTION SYSTEM
//  Main Sketch - Arduino IDE
// ==========================================
//
//  Hardware:
//    - ESP32 Dev Board
//    - 16x2 I2C LCD (PCF8574)
//    - 4x4 Matrix Keypad (0-9, A-D, *, #)
//    - SD Card Module (SPI)
//    - MAX485 Module (RS485)
//    - 7-in-1 Soil Sensor (NPKPHCTH-S, Modbus RTU)
//    - DS3231 RTC Module (I2C)
//    - SIM800L GSM Module (UART1)
//
//  Libraries needed (install via Library Manager):
//    - LiquidCrystal_I2C by Frank de Brabander
//    - Keypad by Mark Stanley & Alexander Brevig
//    - ArduinoJson by Benoît Blanchon
//    - RTClib by Adafruit
//
// ==========================================

#include "config.h"
#include "gsm_manager.h"
#include "keypad_manager.h"
#include "lcd_manager.h"
#include "rtc_manager.h"
#include "sd_manager.h"
#include "sensor_manager.h"
#include "wifi_sync.h"

// ==========================================
//  STATE MACHINE
// ==========================================
enum SystemState {
  STATE_BOOT,
  STATE_WIFI_CHECK,
  STATE_SYNC_PROMPT,
  STATE_SYNCING,
  STATE_MAIN_MENU,
  STATE_ENTER_ID,
  STATE_FARMER_FOUND,
  STATE_NEW_FARMER,
  STATE_READING_SOIL,
  STATE_SHOW_RESULTS,
  STATE_SAVE_PROMPT,
  STATE_DATA_SAVED
};

SystemState currentState = STATE_BOOT;

// Current session variables
String currentFarmerID = "";
String currentPhone = "";
SoilData currentReading;
int resultPage = 0;

// ==========================================
//  SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Farm Data Collector ===");
  Serial.println("Initializing...\n");

  // Initialize all hardware
  lcdInit();
  rtcInit();
  keypadInit();
  sensorInit();

  // Show boot screen
  lcdShowBoot();
  delay(2000);

  // Initialize SD card
  if (!sdInit()) {
    lcdShowSDError();
    Serial.println("CRITICAL: SD Card failed! Waiting for retry...");
    delay(3000);
    // Try once more
    if (!sdInit()) {
      lcdShowMessage("SD CARD FAIL!", "Insert & reset");
      while (true) {
        delay(1000);
      } // Halt
    }
  }

  Serial.println("SD Card initialized. Farmers: " + String(getFarmerCount()) +
                 ", Logs: " + String(getLogCount()));

  // Initialize GSM module and load SMS config from SD
  gsmInit();
  loadSmsConfig();

  // Show GSM connection status on LCD
  lcdShowGsmStatus(gsmIsReady());
  delay(1500);

  // Move to WiFi check state
  currentState = STATE_WIFI_CHECK;
}

// ==========================================
//  MAIN LOOP - STATE MACHINE
// ==========================================
void loop() {
  switch (currentState) {

  // ------------------------------------------
  //  WIFI CHECK - Try to connect
  // ------------------------------------------
  case STATE_WIFI_CHECK: {
    lcdShowWiFiConnecting();

    if (connectWiFi()) {
      // WiFi connected - always offer sync (for data, SMS settings, and time)
      lcdShowWiFiConnected();
      currentState = STATE_SYNC_PROMPT;
    } else {
      // No WiFi - skip to main menu
      lcdShowNoWiFi();
      delay(2000);
      currentState = STATE_MAIN_MENU;
    }
    break;
  }

  // ------------------------------------------
  //  SYNC PROMPT - Ask user to sync
  // ------------------------------------------
  case STATE_SYNC_PROMPT: {
    char key = waitForConfirmOrCancel();
    if (key == '*') {
      currentState = STATE_SYNCING;
    } else {
      currentState = STATE_MAIN_MENU;
    }
    break;
  }

  // ------------------------------------------
  //  SYNCING - Upload data to server
  // ------------------------------------------
  case STATE_SYNCING: {
    lcdShowSyncing();

    // Read file contents
    String farmersData = readFileContent(FARMERS_FILE);
    String datalogData = readFileContent(DATALOG_FILE);

    Serial.println("Syncing " + String(farmersData.length()) +
                   " bytes farmers + " + String(datalogData.length()) +
                   " bytes datalog");

    // Attempt sync
    bool success = syncToServer(farmersData, datalogData);

    if (success) {
      // Server confirmed - clear data logs (keep farmers!)
      clearDataLogs();
      lcdShowSyncSuccess();
      notifySyncComplete(true);
      delay(2500);
    } else {
      lcdShowSyncFail();
      notifySyncComplete(false);
      delay(2500);
    }

    currentState = STATE_MAIN_MENU;
    break;
  }

  // ------------------------------------------
  //  MAIN MENU - Enter Farmer ID
  // ------------------------------------------
  case STATE_MAIN_MENU: {
    currentFarmerID = "";
    currentPhone = "";
    resultPage = 0;

    // Show stats + hint to press A for sync
    lcd.clear();
    String gsmTag = gsmIsReady() ? "G" : "";
    lcdPrint(0, 0,
             "F:" + String(getFarmerCount()) + " L:" + String(getLogCount()) +
                 " " + gsmTag);
    lcdPrint(0, 1, "A:Sync  *:Start");

    // Wait for key: A=sync, anything else=enter farmer ID
    char menuKey = waitForAnyKey();
    if (menuKey == 'A') {
      lcdShowSyncMenu();
      char syncKey = waitForConfirmOrCancel();
      if (syncKey == '*') {
        currentState = STATE_WIFI_CHECK;
      } else {
        currentState = STATE_MAIN_MENU; // Back to this menu
      }
    } else {
      currentState = STATE_ENTER_ID;
    }
    break;
  }

  // ------------------------------------------
  //  ENTER FARMER ID
  // ------------------------------------------
  case STATE_ENTER_ID: {
    lcdShowEnterID();

    // Show next available ID as hint
    String nextID = getNextFarmerID();
    Serial.println("Next available ID: " + nextID);

    bool confirmed;
    currentFarmerID =
        collectFarmerID(confirmed, [](String input) { lcdShowIDInput(input); });

    if (confirmed && currentFarmerID.length() == FARMER_ID_LENGTH) {
      Serial.println("Entered Farmer ID: " + currentFarmerID);

      if (farmerExists(currentFarmerID)) {
        // Farmer found!
        currentPhone = getFarmerPhone(currentFarmerID);
        currentState = STATE_FARMER_FOUND;
      } else {
        // New farmer
        currentState = STATE_NEW_FARMER;
      }
    } else {
      // Cancelled or invalid - stay at main menu
      currentState = STATE_MAIN_MENU;
    }
    break;
  }

  // ------------------------------------------
  //  FARMER FOUND - Show data and options
  // ------------------------------------------
  case STATE_FARMER_FOUND: {
    lcdShowFarmerFound(currentFarmerID, currentPhone);
    delay(2000);

    lcdShowFarmerOptions();

    char key = waitForConfirmOrCancel();
    if (key == '*') {
      // Take new reading
      currentState = STATE_READING_SOIL;
    } else {
      // Back to menu
      currentState = STATE_MAIN_MENU;
    }
    break;
  }

  // ------------------------------------------
  //  NEW FARMER - Register with phone number
  // ------------------------------------------
  case STATE_NEW_FARMER: {
    lcdShowNewFarmer();

    bool confirmed;
    currentPhone = collectPhoneNumber(
        confirmed, [](String input) { lcdShowPhoneInput(input); });

    if (confirmed && currentPhone.length() > 0) {
      // Save new farmer
      String timestamp = getTimestamp();
      if (addFarmer(currentFarmerID, currentPhone, timestamp)) {
        lcdShowFarmerSaved(currentFarmerID);
        delay(2000);
        // Proceed to soil reading
        currentState = STATE_READING_SOIL;
      } else {
        lcdShowSDError();
        delay(2000);
        currentState = STATE_MAIN_MENU;
      }
    } else {
      // Cancelled
      currentState = STATE_MAIN_MENU;
    }
    break;
  }

  // ------------------------------------------
  //  READING SOIL - Take 5 measurements
  // ------------------------------------------
  case STATE_READING_SOIL: {
    Serial.println("Starting soil reading for farmer " + currentFarmerID);

    currentReading =
        takeAveragedReading(NUM_SAMPLES, [](int current, int total) {
          lcdShowReadingProgress(current, total);
        });

    if (currentReading.valid) {
      resultPage = 0;
      currentState = STATE_SHOW_RESULTS;
    } else {
      lcdShowSensorError();
      delay(3000);

      // Ask retry or back
      lcdShowMessage("*:Retry", "#:Back to Menu");
      char key = waitForConfirmOrCancel();
      if (key == '*') {
        currentState = STATE_READING_SOIL; // Retry
      } else {
        currentState = STATE_MAIN_MENU;
      }
    }
    break;
  }

  // ------------------------------------------
  //  SHOW RESULTS - Display averaged readings
  // ------------------------------------------
  case STATE_SHOW_RESULTS: {
    lcdShowResults(currentReading.humidity, currentReading.temperature,
                   currentReading.ec, currentReading.ph,
                   currentReading.nitrogen, currentReading.phosphorus,
                   currentReading.potassium, resultPage);

    // Wait for keypress to navigate pages or proceed
    char key = waitForAnyKey();

    if (key == 'A' || key == 'B') {
      // Next page
      resultPage = (resultPage + 1) % 2;
    } else if (key == '*' || key == '#') {
      // Go to save prompt
      currentState = STATE_SAVE_PROMPT;
    } else {
      // Toggle page on any other key
      resultPage = (resultPage + 1) % 2;
    }
    break;
  }

  // ------------------------------------------
  //  SAVE PROMPT - Save or retake
  // ------------------------------------------
  case STATE_SAVE_PROMPT: {
    lcdShowSavePrompt();

    char key = waitForConfirmOrCancel();
    if (key == '*') {
      // Save the reading
      String timestamp = getTimestamp();
      if (saveReading(currentFarmerID, timestamp, currentReading)) {
        currentState = STATE_DATA_SAVED;
      } else {
        lcdShowSDError();
        delay(2000);
        currentState = STATE_MAIN_MENU;
      }
    } else {
      // Retake - go back to reading
      currentState = STATE_READING_SOIL;
    }
    break;
  }

  // ------------------------------------------
  //  DATA SAVED - Confirmation + SMS
  // ------------------------------------------
  case STATE_DATA_SAVED: {
    lcdShowDataSaved();

    // Send SMS to farmer if enabled
    if (isSmsEnabled() && currentPhone.length() > 0) {
      lcdShowMessage("Sending SMS...", currentPhone.c_str());

      String timestamp = getTimestamp();
      String smsMsg = buildSmsMessage(
          smsTemplate, currentFarmerID, currentReading.humidity,
          currentReading.temperature, currentReading.ec, currentReading.ph,
          currentReading.nitrogen, currentReading.phosphorus,
          currentReading.potassium, timestamp);

      if (sendSMS(currentPhone, smsMsg)) {
        lcdShowMessage("SMS Sent!", "Press any key...");
      } else {
        lcdShowMessage("SMS Failed!", "Press any key...");
      }
    }

    waitForAnyKey();
    currentState = STATE_MAIN_MENU;
    break;
  }

  default:
    currentState = STATE_MAIN_MENU;
    break;
  }

  // Small delay to prevent CPU hogging
  delay(10);
}
