#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "config.h"
#include <HardwareSerial.h>


// Soil data structure
struct SoilData {
  float humidity;    // %RH
  float temperature; // °C
  float ec;          // µS/cm (conductivity)
  float ph;          // pH value
  float nitrogen;    // mg/kg
  float phosphorus;  // mg/kg
  float potassium;   // mg/kg
  bool valid;        // true if reading was successful
};

// Modbus RTU request frame to read 7 registers starting at 0x0000
// Slave=0x01, Function=0x03 (Read Holding Registers), Start=0x0000,
// Count=0x0007 CRC is pre-calculated for this exact frame
const byte soilRequest[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};
const int RESPONSE_LENGTH =
    19; // 1(addr) + 1(func) + 1(byteCount) + 14(data) + 2(CRC)

// Use HardwareSerial (Serial2) on ESP32
HardwareSerial rs485Serial(2);

void sensorInit() {
  // Configure direction control pins
  pinMode(RS485_DE_PIN, OUTPUT);
  pinMode(RS485_RE_PIN, OUTPUT);

  // Start in receive mode
  digitalWrite(RS485_DE_PIN, LOW);
  digitalWrite(RS485_RE_PIN, LOW);

  // Initialize Serial2 with custom pins
  rs485Serial.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  delay(100);
}

// Set RS485 to transmit mode
void rs485Transmit() {
  digitalWrite(RS485_DE_PIN, HIGH);
  digitalWrite(RS485_RE_PIN, HIGH);
  delay(5);
}

// Set RS485 to receive mode
void rs485Receive() {
  digitalWrite(RS485_DE_PIN, LOW);
  digitalWrite(RS485_RE_PIN, LOW);
  delay(5);
}

// Read a single soil measurement from the sensor
SoilData readSoilSensor() {
  SoilData data;
  data.valid = false;

  byte response[RESPONSE_LENGTH];

  // Clear any old data in the buffer
  while (rs485Serial.available()) {
    rs485Serial.read();
  }

  // Switch to transmit mode and send request
  rs485Transmit();
  rs485Serial.write(soilRequest, sizeof(soilRequest));
  rs485Serial.flush(); // Wait for transmission to complete

  // Switch to receive mode
  rs485Receive();

  // Wait for response with timeout
  unsigned long startTime = millis();
  while (rs485Serial.available() < RESPONSE_LENGTH &&
         (millis() - startTime) < SENSOR_TIMEOUT_MS) {
    delay(1);
  }

  if (rs485Serial.available() >= RESPONSE_LENGTH) {
    // Read the response
    byte index = 0;
    while (rs485Serial.available() && index < RESPONSE_LENGTH) {
      response[index] = rs485Serial.read();
      index++;
    }

    // Verify response header: address=0x01, function=0x03, byteCount=0x0E (14)
    if (response[0] == SENSOR_ADDR && response[1] == 0x03 &&
        response[2] == 0x0E) {
      // Parse the 7 register values (each 2 bytes, big-endian)
      int rawHumidity = (response[3] << 8) | response[4];
      int rawTemperature = (response[5] << 8) | response[6];
      int rawEC = (response[7] << 8) | response[8];
      int rawPH = (response[9] << 8) | response[10];
      int rawNitrogen = (response[11] << 8) | response[12];
      int rawPhosphorus = (response[13] << 8) | response[14];
      int rawPotassium = (response[15] << 8) | response[16];

      // Convert to real values (divide by 10)
      data.humidity = rawHumidity / 10.0;
      data.temperature = rawTemperature / 10.0;
      data.ec = rawEC; // EC might not need /10, depends on sensor model
      data.ph = rawPH / 10.0;
      data.nitrogen = rawNitrogen; // mg/kg, usually integer
      data.phosphorus = rawPhosphorus;
      data.potassium = rawPotassium;

      // Handle negative temperature (two's complement)
      if (rawTemperature > 0x7FFF) {
        data.temperature = -(0x10000 - rawTemperature) / 10.0;
      }

      data.valid = true;

      // Debug output
      Serial.println("--- Soil Sensor Reading ---");
      Serial.println("Humidity: " + String(data.humidity) + " %RH");
      Serial.println("Temperature: " + String(data.temperature) + " °C");
      Serial.println("EC: " + String(data.ec) + " µS/cm");
      Serial.println("pH: " + String(data.ph));
      Serial.println("Nitrogen: " + String(data.nitrogen) + " mg/kg");
      Serial.println("Phosphorus: " + String(data.phosphorus) + " mg/kg");
      Serial.println("Potassium: " + String(data.potassium) + " mg/kg");
    } else {
      Serial.println("Sensor: Invalid response header");
      Serial.print("Got: ");
      for (int i = 0; i < index; i++) {
        Serial.print(response[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  } else {
    Serial.println("Sensor: Timeout or incomplete frame (" +
                   String(rs485Serial.available()) + " bytes received)");
  }

  return data;
}

// Take multiple samples and return the averaged result
SoilData takeAveragedReading(int numSamples,
                             void (*progressCallback)(int, int)) {
  SoilData averaged;
  averaged.humidity = 0;
  averaged.temperature = 0;
  averaged.ec = 0;
  averaged.ph = 0;
  averaged.nitrogen = 0;
  averaged.phosphorus = 0;
  averaged.potassium = 0;
  averaged.valid = false;

  int validCount = 0;

  for (int i = 0; i < numSamples; i++) {
    if (progressCallback) {
      progressCallback(i + 1, numSamples);
    }

    SoilData sample = readSoilSensor();

    if (sample.valid) {
      averaged.humidity += sample.humidity;
      averaged.temperature += sample.temperature;
      averaged.ec += sample.ec;
      averaged.ph += sample.ph;
      averaged.nitrogen += sample.nitrogen;
      averaged.phosphorus += sample.phosphorus;
      averaged.potassium += sample.potassium;
      validCount++;
    }

    if (i < numSamples - 1) {
      delay(SENSOR_READ_DELAY);
    }
  }

  if (validCount > 0) {
    averaged.humidity /= validCount;
    averaged.temperature /= validCount;
    averaged.ec /= validCount;
    averaged.ph /= validCount;
    averaged.nitrogen /= validCount;
    averaged.phosphorus /= validCount;
    averaged.potassium /= validCount;
    averaged.valid = true;

    Serial.println("=== Averaged Result (" + String(validCount) + "/" +
                   String(numSamples) + " valid samples) ===");
  } else {
    Serial.println("ERROR: No valid sensor readings obtained!");
  }

  return averaged;
}

#endif // SENSOR_MANAGER_H
