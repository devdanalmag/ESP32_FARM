#ifndef GSM_MANAGER_H
#define GSM_MANAGER_H

#include "config.h"
#include <HardwareSerial.h>
#include <SD.h>

// Use Serial1 for GSM (Serial2 is used by RS485 soil sensor)
HardwareSerial gsmSerial(1);

bool gsmReady = false;

// SMS config loaded from SD card
bool smsEnabled = false;
String smsTemplate = "";

// ==========================================
//  GSM INITIALIZATION
// ==========================================

// Send an AT command and wait for expected response
String sendATCommand(const char *cmd, unsigned long timeoutMs = 2000) {
  gsmSerial.println(cmd);

  unsigned long start = millis();
  String response = "";

  while ((millis() - start) < timeoutMs) {
    while (gsmSerial.available()) {
      char c = gsmSerial.read();
      response += c;
    }
    delay(10);
  }

  response.trim();
  Serial.println("GSM CMD: " + String(cmd) + " -> " + response);
  return response;
}

// Initialize the SIM800L GSM module
void gsmInit() {
  Serial.println("GSM: Initializing SIM800L on Serial1...");
  gsmSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
  delay(1000);

  // Test communication with AT
  String resp = sendATCommand("AT");
  if (resp.indexOf("OK") == -1) {
    Serial.println("GSM: SIM800L not responding. Retrying...");
    delay(2000);
    resp = sendATCommand("AT");
  }

  if (resp.indexOf("OK") != -1) {
    gsmReady = true;
    Serial.println("GSM: SIM800L connected!");

    // Set text mode for SMS
    sendATCommand("AT+CMGF=1");

    // Set character set to GSM default
    sendATCommand("AT+CSCS=\"GSM\"");

    // Check SIM status
    String simResp = sendATCommand("AT+CPIN?");
    if (simResp.indexOf("READY") != -1) {
      Serial.println("GSM: SIM card ready");
    } else {
      Serial.println("GSM: SIM card issue - " + simResp);
    }

    // Check network registration
    String netResp = sendATCommand("AT+CREG?", 3000);
    Serial.println("GSM: Network status: " + netResp);

    // Check signal strength
    String sigResp = sendATCommand("AT+CSQ");
    Serial.println("GSM: Signal: " + sigResp);

  } else {
    gsmReady = false;
    Serial.println("GSM: SIM800L not found! SMS disabled.");
  }
}

// Check if GSM module is ready
bool gsmIsReady() { return gsmReady; }

// ==========================================
//  SMS SENDING
// ==========================================

// Send an SMS to the specified phone number
bool sendSMS(String phoneNumber, String message) {
  if (!gsmReady) {
    Serial.println("GSM: Cannot send SMS - module not ready");
    return false;
  }

  Serial.println("GSM: Sending SMS to " + phoneNumber);
  Serial.println("GSM: Message: " + message);

  // Set SMS text mode
  sendATCommand("AT+CMGF=1");

  // Set recipient
  String cmd = "AT+CMGS=\"" + phoneNumber + "\"";
  gsmSerial.println(cmd);
  delay(500);

  // Send message body
  gsmSerial.print(message);
  delay(100);

  // Send Ctrl+Z (0x1A) to finalize
  gsmSerial.write(0x1A);

  // Wait for response (SMS sending can take a few seconds)
  unsigned long start = millis();
  String response = "";
  while ((millis() - start) < 10000) {
    while (gsmSerial.available()) {
      char c = gsmSerial.read();
      response += c;
    }
    if (response.indexOf("+CMGS:") != -1) {
      break; // Success
    }
    if (response.indexOf("ERROR") != -1) {
      break; // Failed
    }
    delay(100);
  }

  Serial.println("GSM: Send result: " + response);

  if (response.indexOf("+CMGS:") != -1) {
    Serial.println("GSM: SMS sent successfully!");
    return true;
  } else {
    Serial.println("GSM: SMS failed to send");
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
