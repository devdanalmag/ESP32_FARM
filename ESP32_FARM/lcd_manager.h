#ifndef LCD_MANAGER_H
#define LCD_MANAGER_H

#include "config.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

void lcdInit() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void lcdClear() { lcd.clear(); }

void lcdPrint(int col, int row, const char *text) {
  lcd.setCursor(col, row);
  lcd.print(text);
}

void lcdPrint(int col, int row, String text) {
  lcd.setCursor(col, row);
  lcd.print(text);
}

void lcdPrintCentered(int row, const char *text) {
  int len = strlen(text);
  int col = (LCD_COLS - len) / 2;
  if (col < 0)
    col = 0;
  lcd.setCursor(col, row);
  lcd.print(text);
}

void lcdShowBoot() {
  lcd.clear();
  lcdPrintCentered(0, "FARM DATA");
  lcdPrintCentered(1, "COLLECTOR v1.0");
}

void lcdShowWiFiConnecting() {
  lcd.clear();
  lcdPrint(0, 0, "Connecting WiFi");
  lcdPrint(0, 1, "Please wait...");
}

void lcdShowWiFiConnected() {
  lcd.clear();
  lcdPrint(0, 0, "WiFi Connected!");
  lcdPrint(0, 1, "Sync? *Yes #No");
}

void lcdShowNoWiFi() {
  lcd.clear();
  lcdPrint(0, 0, "No WiFi Found");
  lcdPrint(0, 1, "Skipping sync...");
}

void lcdShowSyncing() {
  lcd.clear();
  lcdPrint(0, 0, "Syncing data...");
  lcdPrint(0, 1, "Please wait");
}

void lcdShowSyncSuccess() {
  lcd.clear();
  lcdPrint(0, 0, "Sync Success!");
  lcdPrint(0, 1, "Logs cleared.");
}

void lcdShowSyncFail() {
  lcd.clear();
  lcdPrint(0, 0, "Sync Failed!");
  lcdPrint(0, 1, "Data kept safe.");
}

void lcdShowEnterID() {
  lcd.clear();
  lcdPrint(0, 0, "Enter Farmer ID:");
  lcdPrint(0, 1, "ID: ");
}

void lcdShowIDInput(String id) {
  lcdPrint(4, 1, id + "    "); // pad to clear old chars
}

void lcdShowFarmerFound(String farmerId, String phone) {
  lcd.clear();
  lcdPrint(0, 0, "ID:" + farmerId + " Found!");
  lcdPrint(0, 1, phone);
}

void lcdShowFarmerOptions() {
  lcd.clear();
  lcdPrint(0, 0, "*:New Reading");
  lcdPrint(0, 1, "#:Back to Menu");
}

void lcdShowNewFarmer() {
  lcd.clear();
  lcdPrint(0, 0, "New! Enter Phone");
  lcdPrint(0, 1, "");
}

void lcdShowPhoneInput(String phone) {
  lcdPrint(0, 1, phone + "     "); // pad to clear old chars
}

void lcdShowFarmerSaved(String id) {
  lcd.clear();
  lcdPrint(0, 0, "Farmer Saved!");
  lcdPrint(0, 1, "ID: " + id);
}

void lcdShowReadingProgress(int current, int total) {
  lcd.clear();
  lcdPrint(0, 0, "Reading soil...");
  lcdPrint(0, 1, "Sample " + String(current) + "/" + String(total));
}

void lcdShowSensorError() {
  lcd.clear();
  lcdPrint(0, 0, "Sensor Error!");
  lcdPrint(0, 1, "Check wiring");
}

// Show soil reading results - pages through parameters
void lcdShowResults(float humidity, float temperature, float ec, float ph,
                    float nitrogen, float phosphorus, float potassium,
                    int page) {
  lcd.clear();
  switch (page) {
  case 0:
    lcdPrint(0, 0, "H:" + String(humidity, 1) + "%");
    lcdPrint(9, 0, "T:" + String(temperature, 1) + "C");
    lcdPrint(0, 1, "pH:" + String(ph, 1));
    lcdPrint(9, 1, "EC:" + String((int)ec));
    break;
  case 1:
    lcdPrint(0, 0, "N:" + String((int)nitrogen));
    lcdPrint(8, 0, "P:" + String((int)phosphorus));
    lcdPrint(0, 1, "K:" + String((int)potassium));
    lcdPrint(8, 1, "*Sav #Re");
    break;
  }
}

void lcdShowSavePrompt() {
  lcd.clear();
  lcdPrint(0, 0, "Save reading?");
  lcdPrint(0, 1, "*:Save  #:Retake");
}

void lcdShowDataSaved() {
  lcd.clear();
  lcdPrint(0, 0, "Data Saved!");
  lcdPrint(0, 1, "Press any key...");
}

void lcdShowSDError() {
  lcd.clear();
  lcdPrint(0, 0, "SD Card Error!");
  lcdPrint(0, 1, "Check SD card");
}

void lcdShowMessage(const char *line1, const char *line2) {
  lcd.clear();
  lcdPrint(0, 0, line1);
  lcdPrint(0, 1, line2);
}

void lcdShowGsmStatus(bool ready) {
  lcd.clear();
  if (ready) {
    lcdPrint(0, 0, "GSM: Connected");
    lcdPrint(0, 1, "SIM800L OK");
  } else {
    lcdPrint(0, 0, "GSM: Not Found!");
    lcdPrint(0, 1, "SMS disabled");
  }
}

void lcdShowSyncMenu() {
  lcd.clear();
  lcdPrint(0, 0, "WiFi Sync Menu");
  lcdPrint(0, 1, "*:Sync  #:Back");
}

#endif // LCD_MANAGER_H
