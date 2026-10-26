#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <PZEM004Tv40_R4.h>

// =====================================================
// PIN CONFIGURATION - ARDUINO MEGA 2560
// =====================================================

// ---------- TFT ST7735 1.8" 128x160 ----------
#define TFT_CS   53
#define TFT_DC   49
#define TFT_RST  48

// ---------- Rental Switch ----------
#define RENT_SWITCH 22

// ---------- Relay ----------
#define RELAY_PIN 24

// ---------- Buzzer ----------
#define BUZZER_PIN 26

// ---------- PZEM-004T V4 ----------
PZEM004Tv40_R4 pzem(&Serial2);

// =====================================================
// RELAY CONFIGURATION
// =====================================================
//
// LOW  = RELAY ON
// HIGH = RELAY OFF
//
// =====================================================

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// =====================================================
// SETTINGS
// =====================================================

#define TARIFF 12.0f

#define SUMMARY_TIME 15
#define PAYMENT_TIME 15
#define WAIT_TIME    15

// =====================================================
// TFT
// =====================================================

Adafruit_ST7735 tft(
  TFT_CS,
  TFT_DC,
  TFT_RST
);

#define SCREEN_W 128
#define SCREEN_H 160

// =====================================================
// COLORS
// =====================================================

#define BLACK   ST77XX_BLACK
#define WHITE   ST77XX_WHITE
#define RED     ST77XX_RED
#define GREEN   ST77XX_GREEN
#define BLUE    ST77XX_BLUE
#define CYAN    ST77XX_CYAN
#define YELLOW  ST77XX_YELLOW
#define MAGENTA ST77XX_MAGENTA

// =====================================================
// STATE MACHINE
// =====================================================

enum SystemState {
  WELCOME,
  READY,
  RENTING,
  SUMMARY,
  PAYMENT,
  WAIT_STATE,
  ERROR_STATE
};

SystemState state = WELCOME;

// =====================================================
// PZEM VALUES
// =====================================================

float voltage   = 0.0;
float current   = 0.0;
float power     = 0.0;
float energy    = 0.0;
float frequency = 0.0;
float pf        = 0.0;

// =====================================================
// RENTAL VALUES
// =====================================================

float startEnergy   = 0.0;
float sessionEnergy = 0.0;
float sessionCost   = 0.0;

// =====================================================
// TIMERS
// =====================================================

unsigned long stateStartTime = 0;
unsigned long lastReadTime   = 0;
unsigned long lastBeepTime   = 0;

// =====================================================
// RELAY FUNCTIONS
// =====================================================

// =====================================================
// RELAY ON
// LOW = ON
// =====================================================

void relayON() {
  digitalWrite(RELAY_PIN, LOW);
}

// =====================================================
// RELAY OFF
// HIGH = OFF
// =====================================================

void relayOFF() {
  digitalWrite(RELAY_PIN, HIGH);
}

// =====================================================
// BUZZER
// =====================================================

void beep() {

  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
}

void doubleBeep() {

  beep();
  delay(100);
  beep();
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // ---------------------------------------------------
  // PZEM
  // ---------------------------------------------------

  Serial2.begin(9600);

  // ---------------------------------------------------
  // RENTAL SWITCH
  // ---------------------------------------------------

  pinMode(RENT_SWITCH, INPUT_PULLUP);

  // ---------------------------------------------------
  // RELAY
  // ---------------------------------------------------

  pinMode(RELAY_PIN, OUTPUT);

  // IMPORTANT:
  // RELAY OFF = HIGH
  digitalWrite(RELAY_PIN, HIGH);

  // ---------------------------------------------------
  // BUZZER
  // ---------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // ---------------------------------------------------
  // TFT
  // ---------------------------------------------------

  tft.initR(INITR_BLACKTAB);

  // 128 x 160 portrait
  tft.setRotation(0);

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // PZEM INITIALIZATION
  // ---------------------------------------------------

  pzem.begin();

  // ---------------------------------------------------
  // WELCOME
  // ---------------------------------------------------

  showWelcome();

  stateStartTime = millis();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  switch (state) {

    case WELCOME:
      handleWelcome();
      break;

    case READY:
      handleReady();
      break;

    case RENTING:
      handleRenting();
      break;

    case SUMMARY:
      handleSummary();
      break;

    case PAYMENT:
      handlePayment();
      break;

    case WAIT_STATE:
      handleWait();
      break;

    case ERROR_STATE:
      handleError();
      break;
  }
}

// =====================================================
// WELCOME SCREEN
// =====================================================

void showWelcome() {

  tft.fillScreen(BLACK);

  tft.setTextColor(CYAN);
  tft.setTextSize(2);

  tft.setCursor(20, 20);
  tft.print("SMART");

  tft.setCursor(15, 45);
  tft.print("RENTAL");

  tft.setTextColor(WHITE);
  tft.setTextSize(1);

  tft.setCursor(20, 75);
  tft.print("POWER CONTROLLER");

  tft.setCursor(30, 100);
  tft.print("INITIALIZING");

  tft.setCursor(45, 120);
  tft.print("...");
}

// =====================================================
// WELCOME HANDLER
// =====================================================

void handleWelcome() {

  unsigned long elapsed = millis() - stateStartTime;

  if (elapsed >= 5000UL) {

    // Relay OFF before entering READY
    relayOFF();

    state = READY;
    stateStartTime = millis();

    showReady();
  }
}

// =====================================================
// READ PZEM
// =====================================================
//
// Compatible with PZEM004Tv40_R4 style:
// readAll()
// getVoltage()
// getCurrent()
// getPower()
// getEnergy()
// getFrequency()
// getPowerFactor()
// getLastError()
//
// =====================================================

bool readPZEM() {

  // Read all PZEM values
  pzem.readAll();

  float v = pzem.getVoltage();
  float c = pzem.getCurrent();
  float p = pzem.getPower();
  float e = pzem.getEnergy();
  float f = pzem.getFrequency();
  float factor = pzem.getPowerFactor();

  // Check important values
  if (isnan(v) || isnan(c) || isnan(p) || isnan(e)) {

    Serial.println("PZEM READ ERROR");

    return false;
  }

  voltage   = v;
  current   = c;
  power     = p;
  energy    = e;
  frequency = f;
  pf        = factor;

  return true;
}

// =====================================================
// READY SCREEN
// =====================================================

void showReady() {

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // TITLE
  // ---------------------------------------------------

  tft.setTextColor(CYAN);
  tft.setTextSize(2);

  tft.setCursor(28, 5);
  tft.print("READY");

  tft.drawLine(0, 28, 127, 28, WHITE);

  // ---------------------------------------------------
  // VALUES
  // ---------------------------------------------------

  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  tft.setCursor(5, 38);
  tft.print("Voltage : ");
  tft.print(voltage, 1);
  tft.print(" V");

  tft.setCursor(5, 53);
  tft.print("Current : ");
  tft.print(current, 2);
  tft.print(" A");

  tft.setCursor(5, 68);
  tft.print("Power   : ");
  tft.print(power, 1);
  tft.print(" W");

  tft.setCursor(5, 83);
  tft.print("Freq    : ");
  tft.print(frequency, 1);
  tft.print(" Hz");

  tft.setCursor(5, 98);
  tft.print("PF      : ");
  tft.print(pf, 2);

  tft.drawLine(0, 113, 127, 113, WHITE);

  // ---------------------------------------------------
  // INSTRUCTION
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(5, 122);
  tft.print("TURN SWITCH ON");

  tft.setCursor(5, 137);
  tft.print("TO START RENTAL");
}

// =====================================================
// READY HANDLER
// =====================================================

void handleReady() {

  // Relay must remain OFF
  relayOFF();

  // Read PZEM every 1 second
  if (millis() - lastReadTime >= 1000UL) {

    lastReadTime = millis();

    if (!readPZEM()) {

      showErrorMessage("PZEM ERROR");

      state = ERROR_STATE;
      stateStartTime = millis();

      return;
    }

    showReady();
  }

  // ---------------------------------------------------
  // SWITCH ON
  // ---------------------------------------------------

  if (digitalRead(RENT_SWITCH) == LOW) {

    delay(80);

    if (digitalRead(RENT_SWITCH) == LOW) {

      // Current PZEM energy becomes starting point
      startEnergy = energy;

      sessionEnergy = 0.0;
      sessionCost = 0.0;

      // =================================================
      // RELAY ON
      // LOW = ON
      // =================================================

      relayON();

      beep();

      state = RENTING;
      stateStartTime = millis();

      // Force immediate PZEM update
      lastReadTime = 0;

      showRenting();

      delay(300);
    }
  }
}

// =====================================================
// RENTING SCREEN
// =====================================================

void showRenting() {

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // TITLE
  // ---------------------------------------------------

  tft.setTextColor(GREEN);
  tft.setTextSize(2);

  tft.setCursor(17, 5);
  tft.print("RENTING");

  tft.drawLine(0, 28, 127, 28, WHITE);

  // ---------------------------------------------------
  // VALUES
  // ---------------------------------------------------

  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  tft.setCursor(5, 37);
  tft.print("Voltage : ");
  tft.print(voltage, 1);
  tft.print(" V");

  tft.setCursor(5, 52);
  tft.print("Current : ");
  tft.print(current, 2);
  tft.print(" A");

  tft.setCursor(5, 67);
  tft.print("Power   : ");
  tft.print(power, 1);
  tft.print(" W");

  tft.setCursor(5, 82);
  tft.print("Energy  : ");
  tft.print(sessionEnergy, 3);
  tft.print(" kWh");

  tft.setCursor(5, 97);
  tft.print("Cost    : Rs ");
  tft.print(sessionCost, 2);

  tft.setCursor(5, 112);
  tft.print("F:");
  tft.print(frequency, 1);

  tft.print("Hz PF:");
  tft.print(pf, 2);

  // ---------------------------------------------------
  // STOP MESSAGE
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(5, 137);
  tft.print("SWITCH OFF TO STOP");
}

// =====================================================
// RENTING HANDLER
// =====================================================

void handleRenting() {

  // ===================================================
  // RELAY MUST STAY ON
  // LOW = ON
  // ===================================================

  relayON();

  // ---------------------------------------------------
  // READ PZEM
  // ---------------------------------------------------

  if (millis() - lastReadTime >= 1000UL) {

    lastReadTime = millis();

    if (!readPZEM()) {

      // Safety: relay OFF
      relayOFF();

      showErrorMessage("PZEM ERROR");

      state = ERROR_STATE;
      stateStartTime = millis();

      return;
    }

    // -------------------------------------------------
    // SESSION ENERGY
    // -------------------------------------------------

    if (energy >= startEnergy) {

      sessionEnergy = energy - startEnergy;
    }
    else {

      sessionEnergy = 0.0;
    }

    // -------------------------------------------------
    // COST
    // -------------------------------------------------

    sessionCost = sessionEnergy * TARIFF;

    showRenting();
  }

  // ---------------------------------------------------
  // SWITCH OFF
  // ---------------------------------------------------

  if (digitalRead(RENT_SWITCH) == HIGH) {

    delay(80);

    if (digitalRead(RENT_SWITCH) == HIGH) {

      // =================================================
      // RELAY OFF
      // HIGH = OFF
      // =================================================

      relayOFF();

      // Final energy calculation
      if (energy >= startEnergy) {

        sessionEnergy = energy - startEnergy;
      }
      else {

        sessionEnergy = 0.0;
      }

      sessionCost = sessionEnergy * TARIFF;

      beep();

      state = SUMMARY;
      stateStartTime = millis();

      showSummary();
    }
  }
}

// =====================================================
// SUMMARY SCREEN
// =====================================================

void showSummary() {

  // ===================================================
  // RELAY OFF
  // HIGH = OFF
  // ===================================================

  relayOFF();

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // TITLE
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);
  tft.setTextSize(2);

  tft.setCursor(17, 5);
  tft.print("SUMMARY");

  tft.drawLine(0, 28, 127, 28, WHITE);

  // ---------------------------------------------------
  // ENERGY
  // ---------------------------------------------------

  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  tft.setCursor(5, 40);
  tft.print("Energy Used");

  tft.setCursor(5, 54);
  tft.print(sessionEnergy, 3);
  tft.print(" kWh");

  // ---------------------------------------------------
  // RATE
  // ---------------------------------------------------

  tft.setCursor(5, 72);
  tft.print("Rate : Rs ");
  tft.print(TARIFF, 2);
  tft.print("/unit");

  // ---------------------------------------------------
  // TOTAL
  // ---------------------------------------------------

  tft.setTextColor(GREEN);
  tft.setTextSize(2);

  tft.setCursor(5, 90);
  tft.print("Rs ");
  tft.print(sessionCost, 2);

  tft.drawLine(0, 107, 127, 107, WHITE);

  // ---------------------------------------------------
  // PAYMENT
  // ---------------------------------------------------

  tft.setTextColor(CYAN);
  tft.setTextSize(1);

  tft.setCursor(22, 115);
  tft.print("PAYMENT IN");

  drawSummaryCountdown();
}

// =====================================================
// SUMMARY COUNTDOWN
// =====================================================

void drawSummaryCountdown() {

  unsigned long elapsed = millis() - stateStartTime;

  int remaining =
    SUMMARY_TIME - (elapsed / 1000UL);

  if (remaining < 0) {
    remaining = 0;
  }

  tft.fillRect(0, 130, 127, 159, BLACK);

  tft.setTextColor(WHITE);
  tft.setTextSize(2);

  tft.setCursor(45, 130);

  if (remaining < 10) {
    tft.print("0");
  }

  tft.print(remaining);
  tft.print("s");
}

// =====================================================
// SUMMARY HANDLER
// =====================================================

void handleSummary() {

  // Relay OFF
  relayOFF();

  unsigned long elapsed =
    millis() - stateStartTime;

  drawSummaryCountdown();

  // ---------------------------------------------------
  // SWITCH TURNED ON DURING SUMMARY
  // ---------------------------------------------------

  if (digitalRead(RENT_SWITCH) == LOW) {

    beep();

    state = WAIT_STATE;
    stateStartTime = millis();

    showWait();

    return;
  }

  // ---------------------------------------------------
  // SUMMARY FINISHED
  // ---------------------------------------------------

  if (elapsed >= SUMMARY_TIME * 1000UL) {

    state = PAYMENT;
    stateStartTime = millis();

    showPayment();
  }
}

// =====================================================
// DUMMY QR CODE
// =====================================================

void drawDummyQR() {

  const int moduleSize = 3;
  const int qrSize = 21;

  const int qrPixelSize =
    qrSize * moduleSize;

  const int startX =
    (SCREEN_W - qrPixelSize) / 2;

  const int startY = 42;

  // ---------------------------------------------------
  // WHITE QR BACKGROUND
  // ---------------------------------------------------

  tft.fillRect(
    startX - 2,
    startY - 2,
    qrPixelSize + 4,
    qrPixelSize + 4,
    WHITE
  );

  // ---------------------------------------------------
  // QR PATTERN
  // ---------------------------------------------------

  for (int y = 0; y < qrSize; y++) {

    for (int x = 0; x < qrSize; x++) {

      bool black = false;
      bool finder = false;

      // ------------------------------------------------
      // TOP LEFT FINDER
      // ------------------------------------------------

      if (x < 7 && y < 7) {

        finder = true;

        if (
          x == 0 || x == 6 ||
          y == 0 || y == 6 ||
          (x >= 2 && x <= 4 &&
           y >= 2 && y <= 4)
        ) {

          black = true;
        }
      }

      // ------------------------------------------------
      // TOP RIGHT FINDER
      // ------------------------------------------------

      else if (x >= 14 && y < 7) {

        finder = true;

        int xx = x - 14;

        if (
          xx == 0 || xx == 6 ||
          y == 0 || y == 6 ||
          (xx >= 2 && xx <= 4 &&
           y >= 2 && y <= 4)
        ) {

          black = true;
        }
      }

      // ------------------------------------------------
      // BOTTOM LEFT FINDER
      // ------------------------------------------------

      else if (x < 7 && y >= 14) {

        finder = true;

        int yy = y - 14;

        if (
          x == 0 || x == 6 ||
          yy == 0 || yy == 6 ||
          (x >= 2 && x <= 4 &&
           yy >= 2 && yy <= 4)
        ) {

          black = true;
        }
      }

      // ------------------------------------------------
      // RANDOM MODULES
      // ------------------------------------------------

      if (!finder) {

        if (
          ((x * 3 + y * 5 + x * y) % 7) < 3
        ) {

          black = true;
        }
      }

      if (black) {

        tft.fillRect(
          startX + x * moduleSize,
          startY + y * moduleSize,
          moduleSize,
          moduleSize,
          BLACK
        );
      }
    }
  }
}

// =====================================================
// PAYMENT SCREEN
// =====================================================

void showPayment() {

  // ===================================================
  // RELAY OFF
  // ===================================================

  relayOFF();

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // TITLE
  // ---------------------------------------------------

  tft.setTextColor(CYAN);
  tft.setTextSize(2);

  tft.setCursor(20, 3);
  tft.print("PAYMENT");

  // ---------------------------------------------------
  // AMOUNT
  // ---------------------------------------------------

  tft.setTextSize(1);
  tft.setTextColor(YELLOW);

  tft.setCursor(38, 25);

  tft.print("Rs ");
  tft.print(sessionCost, 2);

  // ---------------------------------------------------
  // QR
  // ---------------------------------------------------

  drawDummyQR();

  // ---------------------------------------------------
  // QR TEXT
  // ---------------------------------------------------

  tft.setTextColor(WHITE);

  tft.setCursor(34, 110);
  tft.print("SCAN TO PAY");

  tft.setTextColor(RED);

  tft.setCursor(28, 123);
  tft.print("QR UNDER WORK");

  // ---------------------------------------------------
  // COUNTDOWN
  // ---------------------------------------------------

  drawPaymentCountdown();
}

// =====================================================
// PAYMENT COUNTDOWN
// =====================================================

void drawPaymentCountdown() {

  unsigned long elapsed =
    millis() - stateStartTime;

  int remaining =
    PAYMENT_TIME - (elapsed / 1000UL);

  if (remaining < 0) {
    remaining = 0;
  }

  tft.fillRect(
    0,
    141,
    127,
    159,
    BLACK
  );

  tft.setTextColor(WHITE);
  tft.setTextSize(2);

  tft.setCursor(45, 141);

  if (remaining < 10) {
    tft.print("0");
  }

  tft.print(remaining);
  tft.print("s");
}

// =====================================================
// PAYMENT HANDLER
// =====================================================

void handlePayment() {

  // Relay OFF
  relayOFF();

  unsigned long elapsed =
    millis() - stateStartTime;

  drawPaymentCountdown();

  // ---------------------------------------------------
  // SWITCH ON DURING PAYMENT
  // ---------------------------------------------------

  if (digitalRead(RENT_SWITCH) == LOW) {

    if (
      millis() - lastBeepTime >= 2000UL
    ) {

      lastBeepTime = millis();

      beep();
    }

    // Show warning at top
    tft.fillRect(
      0,
      15,
      127,
      22,
      BLACK
    );

    tft.setTextColor(RED);
    tft.setTextSize(1);

    tft.setCursor(15, 15);
    tft.print("PLEASE WAIT...");
  }

  // ---------------------------------------------------
  // PAYMENT FINISHED
  // ---------------------------------------------------

  if (elapsed >= PAYMENT_TIME * 1000UL) {

    // Switch OFF -> READY
    if (digitalRead(RENT_SWITCH) == HIGH) {

      state = READY;
      stateStartTime = millis();

      showReady();

      return;
    }

    // Switch still ON -> WAIT
    state = WAIT_STATE;
    stateStartTime = millis();

    showWait();
  }
}

// =====================================================
// WAIT SCREEN
// =====================================================

void showWait() {

  // Relay OFF
  relayOFF();

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // TITLE
  // ---------------------------------------------------

  tft.setTextColor(RED);
  tft.setTextSize(2);

  tft.setCursor(40, 15);
  tft.print("WAIT");

  // ---------------------------------------------------
  // MESSAGE
  // ---------------------------------------------------

  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  tft.setCursor(12, 50);
  tft.print("Previous rental");

  tft.setCursor(12, 65);
  tft.print("is still closing.");

  tft.setTextColor(YELLOW);

  tft.setCursor(12, 82);
  tft.print("Please wait...");

  tft.setTextColor(CYAN);

  tft.setCursor(12, 110);
  tft.print("RENTAL LOCK ACTIVE");

  tft.setTextColor(WHITE);

  tft.setCursor(25, 128);
  tft.print("Switch OFF");
}

// =====================================================
// WAIT HANDLER
// =====================================================

void handleWait() {

  // Relay OFF
  relayOFF();

  unsigned long elapsed =
    millis() - stateStartTime;

  int remaining =
    WAIT_TIME - (elapsed / 1000UL);

  if (remaining < 0) {
    remaining = 0;
  }

  // ---------------------------------------------------
  // COUNTDOWN
  // ---------------------------------------------------

  tft.fillRect(
    35,
    135,
    60,
    20,
    BLACK
  );

  tft.setTextColor(WHITE);
  tft.setTextSize(2);

  tft.setCursor(45, 135);

  if (remaining < 10) {
    tft.print("0");
  }

  tft.print(remaining);

  // ---------------------------------------------------
  // BEEP WHILE SWITCH IS ON
  // ---------------------------------------------------

  if (
    digitalRead(RENT_SWITCH) == LOW &&
    millis() - lastBeepTime >= 2000UL
  ) {

    lastBeepTime = millis();

    beep();
  }

  // ---------------------------------------------------
  // WAIT FINISHED
  // ---------------------------------------------------

  if (elapsed >= WAIT_TIME * 1000UL) {

    // Switch OFF
    if (digitalRead(RENT_SWITCH) == HIGH) {

      state = READY;
      stateStartTime = millis();

      showReady();
    }

    // Switch still ON
    else {

      stateStartTime = millis();

      showWait();
    }
  }
}

// =====================================================
// ERROR MESSAGE
// =====================================================

void showErrorMessage(
  const char *message
) {

  // Safety: relay OFF
  relayOFF();

  tft.fillScreen(BLACK);

  tft.setTextColor(RED);
  tft.setTextSize(2);

  tft.setCursor(28, 15);
  tft.print("ERROR");

  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  tft.setCursor(10, 55);
  tft.print(message);

  tft.setCursor(10, 75);
  tft.print("Check connection.");

  beep();
}

// =====================================================
// ERROR SCREEN
// =====================================================

void showError() {

  // Relay OFF
  relayOFF();

  tft.fillScreen(BLACK);

  tft.setTextColor(RED);
  tft.setTextSize(2);

  tft.setCursor(28, 15);
  tft.print("ERROR");

  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  tft.setCursor(10, 55);
  tft.print("System Error");

  tft.setCursor(10, 75);
  tft.print("Relay OFF");

  tft.setCursor(10, 95);
  tft.print("Check system.");

  beep();
}

// =====================================================
// ERROR HANDLER
// =====================================================

void handleError() {

  // Relay OFF
  relayOFF();

  unsigned long elapsed =
    millis() - stateStartTime;

  if (elapsed >= 5000UL) {

    if (digitalRead(RENT_SWITCH) == HIGH) {

      state = READY;

      stateStartTime = millis();

      showReady();
    }
  }
}