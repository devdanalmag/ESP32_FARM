#ifndef KEYPAD_MANAGER_H
#define KEYPAD_MANAGER_H

#include "config.h"
#include <Keypad.h>


// Create the keypad object
char keys[KEYPAD_ROWS][KEYPAD_COLS];
Keypad keypad = Keypad(makeKeymap(KEYPAD_KEYS), (byte *)ROW_PINS,
                       (byte *)COL_PINS, KEYPAD_ROWS, KEYPAD_COLS);

void keypadInit() {
  keypad.setDebounceTime(50);
  keypad.setHoldTime(1000);
}

// Get a single keypress (blocking with timeout)
// Returns '\0' if timeout
char getKey(unsigned long timeoutMs = 0) {
  unsigned long start = millis();
  while (true) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      return key;
    }
    if (timeoutMs > 0 && (millis() - start) > timeoutMs) {
      return '\0'; // timeout
    }
    delay(10);
  }
}

// Get a single keypress (non-blocking)
// Returns '\0' if no key pressed
char getKeyNonBlocking() {
  char key = keypad.getKey();
  return (key != NO_KEY) ? key : '\0';
}

// Wait for either * or # key
// Returns '*' or '#'
char waitForConfirmOrCancel() {
  while (true) {
    char key = keypad.getKey();
    if (key == '*' || key == '#') {
      return key;
    }
    delay(10);
  }
}

// Collect a numeric string of specified length
// * = confirm/save, # = cancel/back, A/B/C/D = backspace
// Returns the collected string (may be shorter than maxLen if confirmed early)
// Sets 'confirmed' to true if * was pressed, false if # was pressed
String collectNumericInput(int maxLen, bool &confirmed,
                           void (*displayCallback)(String)) {
  String input = "";
  confirmed = false;

  while (true) {
    char key = keypad.getKey();
    if (key == NO_KEY) {
      delay(10);
      continue;
    }

    if (key >= '0' && key <= '9') {
      // Numeric input
      if (input.length() < maxLen) {
        input += key;
        if (displayCallback)
          displayCallback(input);
      }
    } else if (key == '*') {
      // Confirm
      if (input.length() > 0) {
        confirmed = true;
        return input;
      }
    } else if (key == '#') {
      // Cancel
      confirmed = false;
      return input;
    } else if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
      // Backspace - remove last character
      if (input.length() > 0) {
        input.remove(input.length() - 1);
        if (displayCallback)
          displayCallback(input);
      }
    }
  }
}

// Collect a farmer ID (exactly 4 digits, zero-padded)
String collectFarmerID(bool &confirmed, void (*displayCallback)(String)) {
  String id = collectNumericInput(FARMER_ID_LENGTH, confirmed, displayCallback);

  // Zero-pad to 4 digits if confirmed
  if (confirmed) {
    while (id.length() < FARMER_ID_LENGTH) {
      id = "0" + id;
    }
  }
  return id;
}

// Collect phone number (up to 15 digits)
String collectPhoneNumber(bool &confirmed, void (*displayCallback)(String)) {
  return collectNumericInput(15, confirmed, displayCallback);
}

// Wait for any key press
char waitForAnyKey() {
  while (true) {
    char key = keypad.getKey();
    if (key != NO_KEY) {
      return key;
    }
    delay(10);
  }
}

#endif // KEYPAD_MANAGER_H
