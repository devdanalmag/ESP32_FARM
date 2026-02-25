#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include "config.h"
#include <RTClib.h>
#include <Wire.h>

RTC_DS3231 rtc;
bool rtcAvailable = false;

// Initialize the DS3231 RTC module (shares I2C bus with LCD)
void rtcInit() {
  if (!rtc.begin()) {
    Serial.println("RTC: DS3231 not found! Using millis() fallback.");
    rtcAvailable = false;
    return;
  }

  rtcAvailable = true;
  Serial.println("RTC: DS3231 initialized");

  // If the RTC lost power or has never been set, set it to compile time
  if (rtc.lostPower()) {
    Serial.println("RTC: Lost power, setting to compile time...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Print current time
  DateTime now = rtc.now();
  char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", now.year(),
           now.month(), now.day(), now.hour(), now.minute(), now.second());
  Serial.println("RTC: Current time: " + String(buf));
}

// Check if the RTC module is available and working
bool rtcIsValid() { return rtcAvailable; }

// Get a formatted timestamp string from the RTC
// Format: "YYYY-MM-DD HH:MM:SS"
// Falls back to millis()-based counter if RTC is not available
String getTimestamp() {
  if (rtcAvailable) {
    DateTime now = rtc.now();
    char buf[25];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", now.year(),
             now.month(), now.day(), now.hour(), now.minute(), now.second());
    return String(buf);
  }

  // Fallback: millis()-based counter (no real date/time)
  unsigned long sec = millis() / 1000;
  unsigned long mn = sec / 60;
  unsigned long hr = mn / 60;

  char buf[20];
  snprintf(buf, sizeof(buf), "T+%02lu:%02lu:%02lu", hr, mn % 60, sec % 60);
  return String(buf);
}

// Manually set the RTC time
void rtcSetTime(int year, int month, int day, int hour, int minute,
                int second) {
  if (rtcAvailable) {
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    Serial.println("RTC: Time set successfully");
  } else {
    Serial.println("RTC: Cannot set time - module not available");
  }
}

#endif // RTC_MANAGER_H
