#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "config.h"
#include "sensor_manager.h"
#include <SD.h>
#include <SPI.h>

bool sdInitialized = false;

bool sdInit() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card: Mount failed!");
    sdInitialized = false;
    return false;
  }
  Serial.println("SD Card: Mounted successfully");
  sdInitialized = true;

  // Create files with headers if they don't exist
  if (!SD.exists(FARMERS_FILE)) {
    File f = SD.open(FARMERS_FILE, FILE_WRITE);
    if (f) {
      f.println("farmer_id,phone_number,created_at");
      f.close();
      Serial.println("Created " + String(FARMERS_FILE));
    }
  }

  if (!SD.exists(DATALOG_FILE)) {
    File f = SD.open(DATALOG_FILE, FILE_WRITE);
    if (f) {
      f.println("farmer_id,timestamp,humidity,temperature,ec,ph,nitrogen,"
                "phosphorus,potassium");
      f.close();
      Serial.println("Created " + String(DATALOG_FILE));
    }
  }

  return true;
}

// ==========================================
//  FARMER OPERATIONS
// ==========================================

// Check if a farmer ID exists in farmers.csv
bool farmerExists(String farmerId) {
  if (!sdInitialized)
    return false;

  File f = SD.open(FARMERS_FILE, FILE_READ);
  if (!f)
    return false;

  // Skip header line
  f.readStringUntil('\n');

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
      continue;

    // Extract farmer_id (first field before comma)
    int commaIdx = line.indexOf(',');
    if (commaIdx > 0) {
      String id = line.substring(0, commaIdx);
      if (id == farmerId) {
        f.close();
        return true;
      }
    }
  }
  f.close();
  return false;
}

// Get farmer phone number by ID
String getFarmerPhone(String farmerId) {
  if (!sdInitialized)
    return "";

  File f = SD.open(FARMERS_FILE, FILE_READ);
  if (!f)
    return "";

  // Skip header
  f.readStringUntil('\n');

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
      continue;

    int comma1 = line.indexOf(',');
    if (comma1 > 0) {
      String id = line.substring(0, comma1);
      if (id == farmerId) {
        int comma2 = line.indexOf(',', comma1 + 1);
        String phone = (comma2 > 0) ? line.substring(comma1 + 1, comma2)
                                    : line.substring(comma1 + 1);
        f.close();
        return phone;
      }
    }
  }
  f.close();
  return "";
}

// Get the next available farmer ID (auto-increment)
String getNextFarmerID() {
  if (!sdInitialized)
    return "0001";

  int maxID = 0;

  File f = SD.open(FARMERS_FILE, FILE_READ);
  if (!f)
    return "0001";

  // Skip header
  f.readStringUntil('\n');

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
      continue;

    int commaIdx = line.indexOf(',');
    if (commaIdx > 0) {
      String idStr = line.substring(0, commaIdx);
      int id = idStr.toInt();
      if (id > maxID)
        maxID = id;
    }
  }
  f.close();

  // Format next ID with zero padding
  int nextID = maxID + 1;
  char idBuf[5];
  snprintf(idBuf, sizeof(idBuf), "%04d", nextID);
  return String(idBuf);
}

// Get total number of registered farmers
int getFarmerCount() {
  if (!sdInitialized)
    return 0;

  int count = 0;
  File f = SD.open(FARMERS_FILE, FILE_READ);
  if (!f)
    return 0;

  // Skip header
  f.readStringUntil('\n');

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0)
      count++;
  }
  f.close();
  return count;
}

// Add a new farmer to farmers.csv
bool addFarmer(String farmerId, String phoneNumber, String timestamp) {
  if (!sdInitialized)
    return false;

  File f = SD.open(FARMERS_FILE, FILE_APPEND);
  if (!f) {
    Serial.println("SD: Could not open farmers file for writing");
    return false;
  }

  String line = farmerId + "," + phoneNumber + "," + timestamp;
  f.println(line);
  f.close();

  Serial.println("SD: Farmer saved - " + line);
  return true;
}

// ==========================================
//  DATA LOG OPERATIONS
// ==========================================

// Save a soil reading to datalog.csv
bool saveReading(String farmerId, String timestamp, SoilData data) {
  if (!sdInitialized)
    return false;

  File f = SD.open(DATALOG_FILE, FILE_APPEND);
  if (!f) {
    Serial.println("SD: Could not open datalog file for writing");
    return false;
  }

  String line = farmerId + "," + timestamp + "," + String(data.humidity, 1) +
                "," + String(data.temperature, 1) + "," + String(data.ec, 0) +
                "," + String(data.ph, 1) + "," + String(data.nitrogen, 0) +
                "," + String(data.phosphorus, 0) + "," +
                String(data.potassium, 0);
  f.println(line);
  f.close();

  Serial.println("SD: Reading saved - " + line);
  return true;
}

// Get number of log entries
int getLogCount() {
  if (!sdInitialized)
    return 0;

  int count = 0;
  File f = SD.open(DATALOG_FILE, FILE_READ);
  if (!f)
    return 0;

  // Skip header
  f.readStringUntil('\n');

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0)
      count++;
  }
  f.close();
  return count;
}

// Read entire file content as a string (for sync upload)
String readFileContent(const char *path) {
  if (!sdInitialized)
    return "";

  File f = SD.open(path, FILE_READ);
  if (!f)
    return "";

  String content = "";
  while (f.available()) {
    content += (char)f.read();
  }
  f.close();
  return content;
}

// Clear the data log file (keep header only)
bool clearDataLogs() {
  if (!sdInitialized)
    return false;

  // Remove and recreate with header
  SD.remove(DATALOG_FILE);

  File f = SD.open(DATALOG_FILE, FILE_WRITE);
  if (!f)
    return false;

  f.println("farmer_id,timestamp,humidity,temperature,ec,ph,nitrogen,"
            "phosphorus,potassium");
  f.close();

  Serial.println("SD: Data logs cleared");
  return true;
}

#endif // SD_MANAGER_H
