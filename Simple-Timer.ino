// ============================================================
//  Timer Module — TM1637 4-Digit 7-Segment
//  Board  : Arduino Nano (ATmega328P)
//  
//  Pins:
//  - Display CLK : D12
//  - Display DIO : D11
//  - Relay 1 (IN1): D10 (Main Timer Relay)
//  - Relay 2 (IN2): D9  (Completion Relay)
//  - Start Button : D8  (Loads 10 min & Starts / Resumes)
//  - Stop Button  : D7  (Stops if running / Resets if stopped)
//
//  Required library:
//    "TM1637Display" by Avishay Orpaz
// ============================================================

#include <TM1637Display.h>

// ----- Pin definitions -----
#define CLK_PIN       12
#define DIO_PIN       11
#define RELAY1_PIN    2
#define RELAY2_PIN     9
#define BTN_START_PIN  7
#define BTN_STOP_PIN   8

// ----- Relay States -----
// IMPORTANT: Most Arduino relay modules are Active-LOW. 
// If your relays turn ON when they should be OFF, swap these two values (LOW/HIGH).
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ----- Constants -----
#define TIMER_START_SEC (10 * 60) // 10 minutes (600 seconds)
#define RELAY2_DURATION_SEC 15    // 15 seconds
#define RELAY_COOLDOWN_MS 2000    // 2 seconds lockout to prevent rapid cycling

// Create display object
TM1637Display display(CLK_PIN, DIO_PIN);

// ----- State Machine -----
enum State {
  STATE_IDLE,
  STATE_RUNNING,
  STATE_PAUSED,
  STATE_RELAY2_ACTIVE
};

State currentState = STATE_IDLE;

unsigned long previousMillis = 0;
unsigned long lastStateChangeMillis = 0; // Tracks when the relay state last changed
int timeRemaining = 0; // in seconds

bool lastBtnStart = HIGH;
bool lastBtnStop = HIGH;

// Function prototype
void displayTime(int totalSeconds);

// ============================================================
void setup() {
  // Initialize Relays
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, RELAY_OFF);
  digitalWrite(RELAY2_PIN, RELAY_OFF);

  // Initialize Buttons (assumes buttons connect Pin to GND)
  pinMode(BTN_START_PIN, INPUT_PULLUP);
  pinMode(BTN_STOP_PIN, INPUT_PULLUP);

  // Initialize Display
  display.setBrightness(5); // 0 (dimmest) to 7 (brightest)
  displayTime(0);
}

// ============================================================
void loop() {
  unsigned long currentMillis = millis();

  // ----- Button Reading & Debouncing with Cooldown -----
  bool startPressed = false;
  bool stopPressed = false;

  bool currentBtnStart = digitalRead(BTN_START_PIN);
  if (lastBtnStart == HIGH && currentBtnStart == LOW) {
    // Only accept press if 2 seconds have passed since the last state change
    if (currentMillis - lastStateChangeMillis >= RELAY_COOLDOWN_MS) {
      startPressed = true;
    }
    delay(50); // Quick debounce
  }
  lastBtnStart = currentBtnStart;

  bool currentBtnStop = digitalRead(BTN_STOP_PIN);
  if (lastBtnStop == HIGH && currentBtnStop == LOW) {
    // Only accept press if 2 seconds have passed since the last state change
    if (currentMillis - lastStateChangeMillis >= RELAY_COOLDOWN_MS) {
      stopPressed = true;
    }
    delay(50); // Quick debounce
  }
  lastBtnStop = currentBtnStop;

  // ----- State Machine Logic -----
  switch (currentState) {
    case STATE_IDLE:
      if (startPressed) {
        timeRemaining = TIMER_START_SEC; 
        currentState = STATE_RUNNING;
        digitalWrite(RELAY1_PIN, RELAY_ON); 
        previousMillis = currentMillis;
        lastStateChangeMillis = currentMillis; // Log state change
      }
      break;

    case STATE_RUNNING:
      if (stopPressed) {
        currentState = STATE_PAUSED;
        digitalWrite(RELAY1_PIN, RELAY_OFF); 
        lastStateChangeMillis = currentMillis; // Log state change
      } else if (currentMillis - previousMillis >= 1000) {
        previousMillis += 1000;
        timeRemaining--;
        
        if (timeRemaining <= 0) {
          digitalWrite(RELAY1_PIN, RELAY_OFF); 
          
          timeRemaining = RELAY2_DURATION_SEC;
          currentState = STATE_RELAY2_ACTIVE;
          digitalWrite(RELAY2_PIN, RELAY_ON); 
          lastStateChangeMillis = currentMillis; // Log state change
        }
      }
      break;

    case STATE_PAUSED:
      if (startPressed) {
        currentState = STATE_RUNNING;
        digitalWrite(RELAY1_PIN, RELAY_ON);
        previousMillis = currentMillis;
        lastStateChangeMillis = currentMillis; // Log state change
      } else if (stopPressed) {
        timeRemaining = 0;
        currentState = STATE_IDLE;
        digitalWrite(RELAY1_PIN, RELAY_OFF);
        digitalWrite(RELAY2_PIN, RELAY_OFF);
        lastStateChangeMillis = currentMillis; // Log state change
      }
      break;

    case STATE_RELAY2_ACTIVE:
      if (stopPressed) {
        timeRemaining = 0;
        currentState = STATE_IDLE;
        digitalWrite(RELAY2_PIN, RELAY_OFF);
        lastStateChangeMillis = currentMillis; // Log state change
      } else if (currentMillis - previousMillis >= 1000) {
        previousMillis += 1000;
        timeRemaining--;
        
        if (timeRemaining <= 0) {
          digitalWrite(RELAY2_PIN, RELAY_OFF);
          currentState = STATE_IDLE;
          lastStateChangeMillis = currentMillis; // Log state change
        }
      }
      break;
  }

  // ----- Update Display -----
  displayTime(timeRemaining);
}

// ============================================================
// Helper function to format and display the time
// ============================================================
void displayTime(int totalSeconds) {
  int displayValue = 0;
  
  if (currentState == STATE_RELAY2_ACTIVE) {
    // Show just seconds during the 15s Relay 2 countdown
    displayValue = totalSeconds;
  } else {
    // Format as MM:SS
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    displayValue = (minutes * 100) + seconds;
  }
  
  // Determine colon state (0b01000000 is the colon segment on TM1637)
  uint8_t dots = 0;
  
  if (currentState == STATE_RUNNING) {
    // Blink colon while running (toggles every 500ms)
    if ((millis() / 500) % 2 == 0) {
      dots = 0b01000000;
    }
  } else {
    // Solid colon when idle or paused
    dots = 0b01000000;
  }

  // Update display
  display.showNumberDecEx(displayValue, dots, true); // true = keep leading zeros
}