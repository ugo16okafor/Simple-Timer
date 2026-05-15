# 1 "C:\\Users\\HP\\Documents\\GitHub\\Simple-Timer\\Simple-Timer.ino"
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

# 18 "C:\\Users\\HP\\Documents\\GitHub\\Simple-Timer\\Simple-Timer.ino" 2

// ----- Pin definitions -----







// ----- Relay States -----
// IMPORTANT: Most Arduino relay modules are Active-LOW. 
// If your relays turn ON when they should be OFF, swap these two values (LOW/HIGH).



// ----- Constants -----



// Create display object
TM1637Display display(12, 11);

// ----- State Machine -----
enum State {
  STATE_IDLE,
  STATE_RUNNING,
  STATE_PAUSED,
  STATE_RELAY2_ACTIVE
};

State currentState = STATE_IDLE;

unsigned long previousMillis = 0;
int timeRemaining = 0; // in seconds

bool lastBtnStart = 0x1;
bool lastBtnStop = 0x1;

// Function prototype
void displayTime(int totalSeconds);

// ============================================================
void setup() {
  // Initialize Relays
  pinMode(2, 0x1);
  pinMode(9, 0x1);
  digitalWrite(2, 0x1);
  digitalWrite(9, 0x1);

  // Initialize Buttons (assumes buttons connect Pin to GND)
  pinMode(8, 0x2);
  pinMode(7, 0x2);

  // Initialize Display
  display.setBrightness(5); // 0 (dimmest) to 7 (brightest)
  displayTime(0);
}

// ============================================================
void loop() {
  unsigned long currentMillis = millis();

  // ----- Button Reading & Debouncing -----
  bool startPressed = false;
  bool stopPressed = false;

  bool currentBtnStart = digitalRead(8);
  if (lastBtnStart == 0x1 && currentBtnStart == 0x0) {
    startPressed = true;
    delay(50); // Quick debounce
  }
  lastBtnStart = currentBtnStart;

  bool currentBtnStop = digitalRead(7);
  if (lastBtnStop == 0x1 && currentBtnStop == 0x0) {
    stopPressed = true;
    delay(50); // Quick debounce
  }
  lastBtnStop = currentBtnStop;

  // ----- State Machine Logic -----
  switch (currentState) {
    case STATE_IDLE:
      if (startPressed) {
        timeRemaining = (10 * 60) /* 10 minutes (600 seconds)*/; // Load 10 minutes
        currentState = STATE_RUNNING;
        digitalWrite(2, 0x0); // Turn ON Relay 1
        previousMillis = currentMillis;
      }
      break;

    case STATE_RUNNING:
      if (stopPressed) {
        // Stop (Pause) the timer
        currentState = STATE_PAUSED;
        digitalWrite(2, 0x1); // Turn OFF Relay 1
      } else if (currentMillis - previousMillis >= 1000) {
        previousMillis += 1000;
        timeRemaining--;

        if (timeRemaining <= 0) {
          // Timer finished!
          digitalWrite(2, 0x1); // Turn OFF Relay 1

          // Start Relay 2 sequence
          timeRemaining = 15 /* 15 seconds*/;
          currentState = STATE_RELAY2_ACTIVE;
          digitalWrite(9, 0x0); // Turn ON Relay 2
        }
      }
      break;

    case STATE_PAUSED:
      if (startPressed) {
        // Resume running
        currentState = STATE_RUNNING;
        digitalWrite(2, 0x0);
        previousMillis = currentMillis;
      } else if (stopPressed) {
        // Reset the timer when STOP is pressed a second time
        timeRemaining = 0;
        currentState = STATE_IDLE;
        digitalWrite(2, 0x1);
        digitalWrite(9, 0x1);
      }
      break;

    case STATE_RELAY2_ACTIVE:
      if (stopPressed) {
        // Allow manual abort of the 15-second relay 2 period
        timeRemaining = 0;
        currentState = STATE_IDLE;
        digitalWrite(9, 0x1);
      } else if (currentMillis - previousMillis >= 1000) {
        previousMillis += 1000;
        timeRemaining--;

        if (timeRemaining <= 0) {
          // Relay 2 finished
          digitalWrite(9, 0x1);
          currentState = STATE_IDLE;
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
