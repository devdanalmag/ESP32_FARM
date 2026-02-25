#ifndef GSM_MANAGER_H
#define GSM_MANAGER_H

#include "config.h"
#include <HardwareSerial.h>
#include <SD.h>

// Use Serial1 for GSM (Serial2 is used by RS485 soil sensor)
HardwareSerial gsmSerial(1);

bool gsmReady = false;
bool gsmNetworkReady = false;

// SMS config loaded from SD card
bool smsEnabled = false;
String smsTemplate = "";

// ==========================================
//  GSM INITIALIZATION
// ==========================================

// Send an AT command and wait for expected response
String sendATCommand(const char *cmd, unsigned long timeoutMs = 2000) {
  // Flush any leftover data
  while (gsmSerial.available()) gsmSerial.read();

  gsmSerial.println(cmd);

  unsigned long start = millis();
  String response = "";

  while ((millis() - start) < timeoutMs) {
    while (gsmSerial.available()) {
      char c = gsmSerial.read();
      response += c;
    }
    // Early exit if we got a complete response
    if (response.indexOf("OK") != -1 || response.indexOf("ERROR") != -1 ||
        response.indexOf(">") != -1) {
      delay(50); // Grab any trailing chars
      while (gsmSerial.available()) response += (char)gsmSerial.read();
      break;
    }
    delay(10);
  }

  response.trim();
  Serial.println("GSM> " + String(cmd) + " => " + response);
  return response;
}

// Check if SIM800L is registered on the cellular network
// Returns true if registered (home or roaming)
bool checkNetworkRegistration() {
  String resp = sendATCommand("AT+CREG?", 3000);
  // +CREG: 0,1 = registered home, +CREG: 0,5 = registered roaming
  if (resp.indexOf(",1") != -1 || resp.indexOf(",5") != -1) {
    gsmNetworkReady = true;
    return true;
  }
  gsmNetworkReady = false;
  return false;
}

// Initialize the SIM800L GSM module
void gsmInit() {
  Serial.println("GSM: Initializing SIM800L on Serial1...");
  gsmSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
  delay(3000); // SIM800L needs time to boot after power on

  // Test communication with AT
  String resp = sendATCommand("AT");
  if (resp.indexOf("OK") == -1) {
    Serial.println("GSM: SIM800L not responding. Retrying...");
    delay(2000);
    resp = sendATCommand("AT");
  }

  if (resp.indexOf("OK") == -1) {
    gsmReady = false;
    Serial.println("GSM: SIM800L not found! SMS disabled.");
    return;
  }

  Serial.println("GSM: SIM800L connected!");

  // Disable echo (cleaner responses)
  sendATCommand("ATE0");

  // Set text mode for SMS
  sendATCommand("AT+CMGF=1");

  // Set character set to GSM default
  sendATCommand("AT+CSCS=\"GSM\"");

  // Check SIM status
  String simResp = sendATCommand("AT+CPIN?");
  if (simResp.indexOf("READY") == -1) {
    Serial.println("GSM: SIM card NOT ready! Check SIM card.");
    Serial.println("GSM: Response was: " + simResp);
    gsmReady = false;
    return;
  }
  Serial.println("GSM: SIM card ready");

  // Wait for network registration (up to 30 seconds)
  Serial.println("GSM: Waiting for network registration...");
  bool registered = false;
  for (int i = 0; i < 15; i++) {
    if (checkNetworkRegistration()) {
      registered = true;
      break;
    }
    Serial.print(".");
    delay(2000);
  }
  Serial.println();

  if (registered) {
    Serial.println("GSM: Network registered!");
  } else {
    Serial.println("GSM: WARNING - Not registered on network! SMS may fail.");
  }

  // Check signal strength (0-31, 99=unknown; 10+ is usable)
  String sigResp = sendATCommand("AT+CSQ");
  Serial.println("GSM: Signal: " + sigResp);

  gsmReady = true;
  Serial.println("GSM: Initialization complete");
}

// Check if GSM module is ready
bool gsmIsReady() { return gsmReady; }

// ==========================================
//  SMS SENDING
// ==========================================

// Format phone number for SIM800L
// Converts local format (09XXXXXXXXX) to international (+639XXXXXXXXX)
String formatPhoneNumber(String phone) {
  phone.trim();
  // Already international format
  if (phone.startsWith("+")) {
    return phone;
  }
  // Local format starting with 0 — convert using country code from config
  if (phone.startsWith("0") && phone.length() >= 10) {
    return String(SMS_COUNTRY_CODE) + phone.substring(1);
  }
  // Already without leading 0, just add country code
  return String(SMS_COUNTRY_CODE) + phone;
}

// Send an SMS to the specified phone number
bool sendSMS(String phoneNumber, String message) {
  if (!gsmReady) {
    Serial.println("GSM: Cannot send SMS - module not ready");
    return false;
  }

  // Re-check network before sending
  if (!checkNetworkRegistration()) {
    Serial.println("GSM: ERROR - Not registered on network!");
    return false;
  }

  // Format the phone number to international format
  String formattedPhone = formatPhoneNumber(phoneNumber);
  Serial.println("GSM: Sending SMS to " + formattedPhone + " (was: " + phoneNumber + ")");
  Serial.println("GSM: Message (" + String(message.length()) + " chars): " + message);

  // Ensure text mode
  sendATCommand("AT+CMGF=1");
  delay(100);

  // Send AT+CMGS command and wait for '>' prompt
  String cmd = "AT+CMGS=\"" + formattedPhone + "\"";
  
  // Flush serial
  while (gsmSerial.available()) gsmSerial.read();
  
  gsmSerial.println(cmd);

  // Wait for '>' prompt (up to 5 seconds)
  unsigned long start = millis();
  String prompt = "";
  bool gotPrompt = false;
  while ((millis() - start) < 5000) {
    while (gsmSerial.available()) {
      char c = gsmSerial.read();
      prompt += c;
    }
    if (prompt.indexOf(">") != -1) {
      gotPrompt = true;
      break;
    }
    if (prompt.indexOf("ERROR") != -1) {
      break;
    }
    delay(50);
  }

  if (!gotPrompt) {
    Serial.println("GSM: ERROR - Never got '>' prompt! Response: " + prompt);
    Serial.println("GSM: This usually means invalid phone number or SIM issue");
    // Send ESC to cancel
    gsmSerial.write(0x1B);
    delay(500);
    return false;
  }

  Serial.println("GSM: Got '>' prompt, sending message body...");

  // Send message body
  gsmSerial.print(message);
  delay(100);

  // Send Ctrl+Z (0x1A) to finalize and send
  gsmSerial.write(0x1A);

  // Wait for response (SMS sending can take up to 60 seconds on some networks)
  start = millis();
  String response = "";
  while ((millis() - start) < 30000) {
    while (gsmSerial.available()) {
      char c = gsmSerial.read();
      response += c;
    }
    if (response.indexOf("+CMGS:") != -1) {
      break; // Network accepted the message
    }
    if (response.indexOf("ERROR") != -1) {
      break;
    }
    delay(100);
  }

  Serial.println("GSM: Raw response: " + response);

  if (response.indexOf("+CMGS:") != -1) {
    Serial.println("GSM: SMS accepted by network!");
    return true;
  } else if (response.indexOf("ERROR") != -1) {
    Serial.println("GSM: SMS REJECTED by network. Check: phone number, SIM credit, signal.");
    return false;
  } else {
    Serial.println("GSM: SMS send TIMEOUT - no response from network");
    return false;
  }
}

// ==========================================
//  MESSAGE TEMPLATE
// ==========================================

// Build the SMS message by replacing placeholders with actual values
String buildSmsMessage(String tmpl, String farmerID, float humidity,
                       float temperature, float ec, float ph, float nitrogen,
                       float phosphorus, float potassium, String timestamp) {
  String msg = tmpl;

  msg.replace("{farmer_id}", farmerID);
  msg.replace("{humidity}", String(humidity, 1));
  msg.replace("{temperature}", String(temperature, 1));
  msg.replace("{ec}", String((int)ec));
  msg.replace("{ph}", String(ph, 1));
  msg.replace("{nitrogen}", String((int)nitrogen));
  msg.replace("{phosphorus}", String((int)phosphorus));
  msg.replace("{potassium}", String((int)potassium));
  msg.replace("{timestamp}", timestamp);

  // Replace literal \n with actual newline for SMS
  msg.replace("\\n", "\n");

  return msg;
}

// ==========================================
//  SMS CONFIG (from SD card)
// ==========================================

// Load SMS configuration from SD card
void loadSmsConfig() {
  if (!SD.exists(SMS_CONFIG_FILE)) {
    Serial.println("GSM: No SMS config file found. SMS disabled.");
    smsEnabled = false;
    smsTemplate = "";
    return;
  }

  File f = SD.open(SMS_CONFIG_FILE, FILE_READ);
  if (!f) {
    Serial.println("GSM: Could not open SMS config file");
    smsEnabled = false;
    return;
  }

  // First line: enabled (0 or 1)
  String enabledStr = f.readStringUntil('\n');
  enabledStr.trim();
  smsEnabled = (enabledStr == "1");

  // Rest of file: template
  smsTemplate = "";
  while (f.available()) {
    smsTemplate += (char)f.read();
  }
  smsTemplate.trim();

  f.close();

  Serial.println("GSM: SMS Config loaded - Enabled: " +
                 String(smsEnabled ? "YES" : "NO"));
  Serial.println("GSM: Template: " + smsTemplate);
}

// Save SMS configuration to SD card
bool saveSmsConfig(bool enabled, String tmpl) {
  // Remove old file
  if (SD.exists(SMS_CONFIG_FILE)) {
    SD.remove(SMS_CONFIG_FILE);
  }

  File f = SD.open(SMS_CONFIG_FILE, FILE_WRITE);
  if (!f) {
    Serial.println("GSM: Could not save SMS config");
    return false;
  }

  f.println(enabled ? "1" : "0");
  f.print(tmpl);
  f.close();

  // Update in-memory values
  smsEnabled = enabled;
  smsTemplate = tmpl;

  Serial.println("GSM: SMS config saved to SD");
  return true;
}

// Check if SMS sending is enabled
bool isSmsEnabled() { return smsEnabled && gsmReady; }

#endif // GSM_MANAGER_H
