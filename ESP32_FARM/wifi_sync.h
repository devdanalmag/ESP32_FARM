#ifndef WIFI_SYNC_H
#define WIFI_SYNC_H

#include "config.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

bool wifiConnected = false;

// Attempt to connect to WiFi with timeout
bool connectWiFi() {
  Serial.println("WiFi: Connecting to " + String(WIFI_SSID) + "...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - startTime) < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("WiFi: Connected! IP: " + WiFi.localIP().toString());
    return true;
  } else {
    wifiConnected = false;
    Serial.println("WiFi: Connection failed");
    WiFi.disconnect();
    return false;
  }
}

// Check if WiFi is still connected
bool isWiFiConnected() {
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  return wifiConnected;
}

// Disconnect WiFi
void disconnectWiFi() {
  WiFi.disconnect();
  wifiConnected = false;
  Serial.println("WiFi: Disconnected");
}

// Sync data to server - upload farmers and data logs
// Returns true if server confirmed success
bool syncToServer(String farmersData, String datalogData) {
  if (!isWiFiConnected()) {
    Serial.println("Sync: No WiFi connection");
    return false;
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000); // 15 second timeout

  // Build JSON payload
  JsonDocument doc;
  doc["farmers_csv"] = farmersData;
  doc["datalog_csv"] = datalogData;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  Serial.println("Sync: Sending " + String(jsonPayload.length()) +
                 " bytes to server...");

  int httpCode = http.POST(jsonPayload);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Sync: Server responded with code " + String(httpCode));
    Serial.println("Sync: Response: " + response);

    if (httpCode == 200) {
      // Parse server response to check for success
      JsonDocument respDoc;
      DeserializationError error = deserializeJson(respDoc, response);

      if (!error) {
        bool success = respDoc["success"] | false;
        if (success) {
          Serial.println("Sync: Server confirmed success!");

          // Save SMS settings from server if available
          if (respDoc.containsKey("sms_settings")) {
            bool smsEn = respDoc["sms_settings"]["enabled"] | false;
            String smsTmpl = respDoc["sms_settings"]["template"] | "";
            if (smsTmpl.length() > 0) {
              saveSmsConfig(smsEn, smsTmpl);
              loadSmsConfig(); // Reload into memory
              Serial.println("Sync: SMS settings updated from server");
            }
          }

          // Update RTC from server time if available
          if (respDoc.containsKey("server_time")) {
            int yr = respDoc["server_time"]["year"] | 0;
            int mo = respDoc["server_time"]["month"] | 0;
            int dy = respDoc["server_time"]["day"] | 0;
            int hr = respDoc["server_time"]["hour"] | 0;
            int mn = respDoc["server_time"]["minute"] | 0;
            int sc = respDoc["server_time"]["second"] | 0;
            if (yr > 2020) {
              rtcSetTime(yr, mo, dy, hr, mn, sc);
              Serial.println("Sync: RTC updated from server time");
            }
          }

          http.end();
          return true;
        } else {
          String msg = respDoc["message"] | "Unknown error";
          Serial.println("Sync: Server reported error: " + msg);
        }
      } else {
        Serial.println("Sync: Could not parse server response");
      }
    }
  } else {
    Serial.println("Sync: HTTP error: " + http.errorToString(httpCode));
  }

  http.end();
  return false;
}

// Check if the dashboard has requested a sync
// Returns true if a pending sync request exists
bool checkSyncRequest() {
  if (!isWiFiConnected())
    return false;

  HTTPClient http;
  http.begin(SYNC_CHECK_URL);
  http.setTimeout(5000);

  int httpCode = http.GET();

  if (httpCode == 200) {
    String response = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (!error) {
      bool pending = doc["sync_pending"] | false;
      http.end();
      return pending;
    }
  }

  http.end();
  return false;
}

// Notify server that sync is complete
bool notifySyncComplete(bool success) {
  if (!isWiFiConnected())
    return false;

  HTTPClient http;
  String url = String(SYNC_CHECK_URL) +
               "?action=complete&status=" + (success ? "completed" : "failed");
  http.begin(url);
  http.setTimeout(5000);

  int httpCode = http.GET();
  http.end();

  return (httpCode == 200);
}

#endif // WIFI_SYNC_H
