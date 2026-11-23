/*===========================================================
                 LED MODE CONTROLLER (ESP32)
============================================================
Description: 
  This is a module for controller of 6 leds with 3 modes namely BLINK, RUN, ALTERNATE
  controlled by switches, python GUI, and the serial monitor. Displaying the current state
  in the GUI, serial monitor, and an LCD1602A I2C DISPLAY.

Programmer:
  Aragon, Edward Paul M.
  Regalado, Cid Lawrenc C.  

Date:
  November 18, 2026
-----------------------------------------------------------  */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LEDs in order: GREEN, YELLOW, RED | GREEN, YELLOW, RED
int LED[] = {32, 33, 25, 26, 27, 12};
const int NUM_LEDS = 6;
const int HALF = 3;                  // first half = LEDs 0-2, second half = LEDs 3-5

int SW1 = 18;
int SW2 = 19;

// How are the switches wired?
//   Switch sends 3.3V when ON:  SWITCH_ON = HIGH, SWITCH_PIN_MODE = INPUT  (current wiring)
//   Switch connects to GND:     SWITCH_ON = LOW,  SWITCH_PIN_MODE = INPUT
const int SWITCH_ON = HIGH;
const int SWITCH_PIN_MODE = INPUT;

#include "LED_Modes.h"               // blink(), alt(), run(), allOff(), resetPattern()


// ---------------------------------------------------------
// VARIABLES
// ---------------------------------------------------------

LiquidCrystal_I2C lcd(0x27, 16, 2);  

// Mode numbers
const int MODE_IDLE  = 0;
const int MODE_BLINK = 1;
const int MODE_RUN   = 2;
const int MODE_ALT   = 3;

const char* names[] = {"IDLE", "BLINK", "RUN", "ALT"};   // names[mode] = text for the LCD

int mode = MODE_IDLE;                // current mode
unsigned long lastStepTime = 0;      // when the last 0.5 s step happened

// Switch debounce
int lastSwitchMode = -1;             
int appliedSwitchMode = -1;          
unsigned long switchChangeTime = 0;  
const unsigned long SETTLE_MS = 50;  


// ---------------------------------------------------------
// SHOW / REPORT / CHANGE THE MODE
// ---------------------------------------------------------

void showLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LED MODE:");
  lcd.setCursor(0, 1);
  lcd.print(names[mode]);
}

// Tell the GUI the current state string matching Python's expectation
void report() {
  Serial.print("STATE:");
  if (mode == MODE_BLINK) Serial.println("BLINK");
  else if (mode == MODE_RUN)   Serial.println("RUN");
  else if (mode == MODE_ALT)   Serial.println("ALT");
  else                         Serial.println("IDLE");
}

void setMode(int newMode) {
  if (newMode != mode) {             // only restart if the mode really changed
    mode = newMode;
    resetPattern();                  // start the pattern again (LEDs off)
    lastStepTime = 0;                // first step happens right away
    showLCD();
  }
  report();
}


// ---------------------------------------------------------
// INPUTS
// ---------------------------------------------------------

// Messages from the Python GUI
void checkSerial() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '1') setMode(MODE_BLINK);
    if (c == '2') setMode(MODE_ALT);
    if (c == '3') setMode(MODE_RUN);
    if (c == '?') report();
  }
}

// Read the 2 switches and return the mode they ask for
int readSwitches() {
  bool sw1On = (digitalRead(SW1) == SWITCH_ON);
  bool sw2On = (digitalRead(SW2) == SWITCH_ON);

  if (sw1On && sw2On)   return MODE_RUN;     // 11
  if (sw1On && !sw2On)  return MODE_ALT;     // 10
  if (!sw1On && sw2On)  return MODE_BLINK;   // 01
  return MODE_IDLE;                          // 00
}

void checkSwitches() {
  int wanted = readSwitches();

  // Debounce: wait until the switches stop changing
  if (wanted != lastSwitchMode) {
    lastSwitchMode = wanted;
    switchChangeTime = millis();
  }
  if (millis() - switchChangeTime < SETTLE_MS) return;

  if (wanted == appliedSwitchMode) return;   
  appliedSwitchMode = wanted;

  setMode(wanted);
}


// ---------------------------------------------------------
// SETUP AND LOOP
// ---------------------------------------------------------

void setup() {
  Serial.begin(115200);              // Configured to 115200 baud

  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED[i], OUTPUT);
  }
  pinMode(SW1, SWITCH_PIN_MODE);
  pinMode(SW2, SWITCH_PIN_MODE);

  Wire.begin(21, 22);                // LCD: SDA = 21, SCL = 22
  lcd.init();
  lcd.backlight();
  showLCD();
  report();                          
}

void loop() {
  checkSerial();
  checkSwitches();

  // every 0.5 s: do one step of the current mode
  if (millis() - lastStepTime >= 500) {
    lastStepTime = millis();

    if (mode == MODE_BLINK) blink();
    if (mode == MODE_RUN)   run();
    if (mode == MODE_ALT)   alt();
  }
}