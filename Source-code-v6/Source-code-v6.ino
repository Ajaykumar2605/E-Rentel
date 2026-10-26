#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <PZEM004Tv40_R4.h>

// =====================================================
// SMART RENTAL POWER CONTROLLER - V1.0
// =====================================================

// =====================================================
// PIN CONFIGURATION
// =====================================================

#define TFT_CS       53
#define TFT_DC       49
#define TFT_RST      48

#define RENT_SWITCH  22
#define RELAY_PIN    24
#define BUZZER_PIN   26

#define TARIFF       12.0f


// =====================================================
// TFT
// =====================================================

Adafruit_ST7735 tft = Adafruit_ST7735(
  TFT_CS,
  TFT_DC,
  TFT_RST
);


// =====================================================
// PZEM
// Mega Serial2:
// RX2 = 17
// TX2 = 16
// =====================================================

PZEM004Tv40_R4 pzem(&Serial2);


// =====================================================
// STATES
// =====================================================

enum SystemState {

  STATE_WELCOME,
  STATE_READY,
  STATE_RENTING,
  STATE_SUMMARY,
  STATE_PAYMENT,
  STATE_WAIT,
  STATE_ERROR

};

SystemState state = STATE_WELCOME;


// =====================================================
// PZEM DATA
// =====================================================

float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;
float frequency = 0;
float pf = 0;


// =====================================================
// RENT SESSION
// =====================================================

float sessionStartEnergy = 0;
float sessionEndEnergy = 0;
float sessionEnergy = 0;
float sessionCost = 0;

unsigned long rentalStartTime = 0;
unsigned long rentalEndTime = 0;

unsigned long stateStartTime = 0;


// =====================================================
// TIMING
// =====================================================

const unsigned long WELCOME_TIME  = 5000UL;
const unsigned long SUMMARY_TIME  = 15000UL;
const unsigned long PAYMENT_TIME  = 15000UL;
const unsigned long WAIT_TIME     = 15000UL;
const unsigned long PZEM_INTERVAL = 1000UL;

unsigned long lastPzemRead = 0;


// =====================================================
// DISPLAY UPDATE
// =====================================================

unsigned long lastReadyDisplay = 0;
unsigned long lastRentalDisplay = 0;


// =====================================================
// WARNING BEEP
// =====================================================

unsigned long lastWarningBeep = 0;


// =====================================================
// COLORS
// =====================================================

#define BLACK   ST77XX_BLACK
#define WHITE   ST77XX_WHITE
#define RED     ST77XX_RED
#define GREEN   ST77XX_GREEN
#define BLUE    ST77XX_BLUE
#define YELLOW  ST77XX_YELLOW
#define CYAN    ST77XX_CYAN


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("SMART RENTAL");
  Serial.println("--------------------------------");
  Serial.println("VERSION: V1.0");


  // ===================================================
  // SWITCH
  // ===================================================

  pinMode(RENT_SWITCH, INPUT_PULLUP);


  // ===================================================
  // RELAY
  // ===================================================

  pinMode(RELAY_PIN, OUTPUT);

  // SAFETY:
  // relay OFF during startup
  digitalWrite(RELAY_PIN, HIGH);


  // ===================================================
  // BUZZER
  // ===================================================

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);


  // ===================================================
  // TFT
  // ===================================================

  tft.initR(INITR_BLACKTAB);

  // 128 x 160 portrait
  tft.setRotation(0);

  tft.fillScreen(BLACK);


  // ===================================================
  // PZEM
  // ===================================================

  Serial2.begin(9600);

  pzem.begin();


  // ===================================================
  // INITIAL STATE
  // ===================================================

  state = STATE_WELCOME;

  stateStartTime = millis();

  drawWelcome();

}


// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  unsigned long now = millis();


  // ---------------------------------------------------
  // PZEM UPDATE
  // ---------------------------------------------------

  updatePZEM();


  // ---------------------------------------------------
  // STATE MACHINE
  // ---------------------------------------------------

  switch (state) {

    case STATE_WELCOME:
      handleWelcome(now);
      break;

    case STATE_READY:
      handleReady(now);
      break;

    case STATE_RENTING:
      handleRenting(now);
      break;

    case STATE_SUMMARY:
      handleSummary(now);
      break;

    case STATE_PAYMENT:
      handlePayment(now);
      break;

    case STATE_WAIT:
      handleWait(now);
      break;

    case STATE_ERROR:
      handleError(now);
      break;
  }

}


// =====================================================
// PZEM UPDATE
// =====================================================

void updatePZEM() {

  if (millis() - lastPzemRead < PZEM_INTERVAL) {

    return;

  }

  lastPzemRead = millis();


  if (pzem.readAll()) {

    voltage   = pzem.getVoltage();
    current   = pzem.getCurrent();
    power     = pzem.getPower();
    energy    = pzem.getEnergy();
    frequency = pzem.getFrequency();
    pf        = pzem.getPowerFactor();


    Serial.print("V: ");
    Serial.print(voltage);

    Serial.print("  I: ");
    Serial.print(current);

    Serial.print("  P: ");
    Serial.print(power);

    Serial.print("  E: ");
    Serial.print(energy);

    Serial.print("  F: ");
    Serial.print(frequency);

    Serial.print("  PF: ");
    Serial.println(pf);

  }

  else {

    Serial.print("PZEM ERROR: ");
    Serial.println(pzem.getLastError());

  }

}


// =====================================================
// WELCOME SCREEN
// =====================================================

void drawWelcome() {

  tft.fillScreen(BLACK);


  // ---------------------------------------------------
  // TITLE
  // ---------------------------------------------------

  tft.setTextColor(CYAN);
  tft.setTextSize(2);

  tft.setCursor(20, 18);
  tft.println("SMART");

  tft.setCursor(20, 43);
  tft.println(" E-RENTAL");


  // ---------------------------------------------------
  // LINE
  // ---------------------------------------------------

  tft.drawLine(
    10,
    70,
    117,
    70,
    WHITE
  );


  // ---------------------------------------------------
  // SUBTITLE
  // ---------------------------------------------------

  tft.setTextColor(WHITE);
  tft.setTextSize(1);

  tft.setCursor(20, 82);
  tft.println("EV CHARGING");
  tft.setCursor(20, 96);
  tft.println("STATION");

  tft.setCursor(20, 108);
  tft.println("lOADING");


  // ---------------------------------------------------
  // ANIMATION
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(20, 120);

  tft.print("*");


  // ---------------------------------------------------
  // VERSION
  // ---------------------------------------------------

  tft.setTextColor(CYAN);

  tft.setCursor(90, 145);

  tft.print("V1.0");

}


// =====================================================
// WELCOME HANDLER
// =====================================================

void handleWelcome(unsigned long now) {

  static unsigned long lastAnimation = 0;
  static byte dots = 0;


  // ---------------------------------------------------
  // ANIMATED DOTS
  // ---------------------------------------------------

  if (now - lastAnimation >= 400UL) {

    lastAnimation = now;


    tft.fillRect(
      20,
      115,
      80,
      15,
      BLACK
    );


    tft.setCursor(20, 115);

    tft.setTextColor(YELLOW);
    tft.setTextSize(1);


    for (byte i = 0; i < dots; i++) {

      tft.print(".");

    }


    dots++;


    if (dots > 4) {

      dots = 0;

    }

  }


  // ---------------------------------------------------
  // WELCOME FINISHED
  // ---------------------------------------------------

  if (now - stateStartTime >= WELCOME_TIME) {

    // SAFETY:
    // relay OFF
    digitalWrite(RELAY_PIN, HIGH);


    state = STATE_READY;

    stateStartTime = now;

    lastReadyDisplay = 0;

    drawReady();

  }

}


// =====================================================
// READY SCREEN
// =====================================================

void drawReady() {

  tft.fillScreen(BLACK);


  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.setTextColor(GREEN);

  tft.setTextSize(2);

  tft.setCursor(28, 5);

  tft.println("READY");


  tft.drawLine(
    0,
    28,
    127,
    28,
    WHITE
  );


  // ---------------------------------------------------
  // VOLTAGE
  // ---------------------------------------------------

  tft.setTextSize(1);

  tft.setTextColor(WHITE);

  tft.setCursor(5, 38);

  tft.print("Voltage : ");

  tft.print(voltage, 1);

  tft.println(" V");


  // ---------------------------------------------------
  // CURRENT
  // ---------------------------------------------------

  tft.setCursor(5, 53);

  tft.print("Current : ");

  tft.print(current, 2);

  tft.println(" A");


  // ---------------------------------------------------
  // POWER
  // ---------------------------------------------------

  tft.setCursor(5, 68);

  tft.print("Power   : ");

  tft.print(power, 1);

  tft.println(" W");


  // ---------------------------------------------------
  // FREQUENCY
  // ---------------------------------------------------

  tft.setCursor(5, 83);

  tft.print("Freq    : ");

  tft.print(frequency, 1);

  tft.println(" Hz");


  // ---------------------------------------------------
  // POWER FACTOR
  // ---------------------------------------------------

  tft.setCursor(5, 98);

  tft.print("PF      : ");

  tft.print(pf, 2);


  // ---------------------------------------------------
  // SEPARATOR
  // ---------------------------------------------------

  tft.drawLine(
    0,
    113,
    127,
    113,
    WHITE
  );


  // ---------------------------------------------------
  // INSTRUCTION
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(5, 122);

  tft.println("TURN SWITCH ON");


  tft.setCursor(5, 137);

  tft.println("TO START RENTAL");

}


// =====================================================
// READY HANDLER
// =====================================================

void handleReady(unsigned long now) {

  // ===================================================
  // RELAY MUST BE OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  // ---------------------------------------------------
  // DISPLAY UPDATE
  // ---------------------------------------------------

  if (now - lastReadyDisplay >= 1000UL) {

    lastReadyDisplay = now;

    drawReady();

  }


  // ---------------------------------------------------
  // SWITCH ON
  // ---------------------------------------------------

  if (digitalRead(RENT_SWITCH) == LOW) {

    delay(80);


    if (digitalRead(RENT_SWITCH) == LOW) {

      startRental();

    }

  }

}


// =====================================================
// START RENTAL
// =====================================================

void startRental() {

  // ---------------------------------------------------
  // CAPTURE START ENERGY
  // ---------------------------------------------------

  sessionStartEnergy = energy;

  sessionEndEnergy = sessionStartEnergy;

  sessionEnergy = 0;

  sessionCost = 0;


  rentalStartTime = millis();


  // ===================================================
  // RELAY ON
  // LOW = ON
  // ===================================================

  digitalWrite(RELAY_PIN, LOW);


  beep(1);


  // ---------------------------------------------------
  // CHANGE STATE
  // ---------------------------------------------------

  state = STATE_RENTING;

  stateStartTime = millis();

  lastRentalDisplay = 0;


  drawRenting();

}


// =====================================================
// RENTING SCREEN
// =====================================================

void drawRenting() {

  tft.fillScreen(BLACK);


  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.setTextColor(GREEN);

  tft.setTextSize(2);

  tft.setCursor(17, 5);

  tft.println("RENTING");


  tft.drawLine(
    0,
    28,
    127,
    28,
    WHITE
  );


  // ---------------------------------------------------
  // VOLTAGE
  // ---------------------------------------------------

  tft.setTextSize(1);

  tft.setTextColor(WHITE);

  tft.setCursor(5, 37);

  tft.print("Voltage : ");

  tft.print(voltage, 1);

  tft.println(" V");


  // ---------------------------------------------------
  // CURRENT
  // ---------------------------------------------------

  tft.setCursor(5, 52);

  tft.print("Current : ");

  tft.print(current, 2);

  tft.println(" A");


  // ---------------------------------------------------
  // POWER
  // ---------------------------------------------------

  tft.setCursor(5, 67);

  tft.print("Power   : ");

  tft.print(power, 1);

  tft.println(" W");


  // ---------------------------------------------------
  // ENERGY
  // ---------------------------------------------------

  tft.setCursor(5, 82);

  tft.print("Energy  : ");

  tft.print(sessionEnergy, 3);

  tft.println(" kWh");


  // ---------------------------------------------------
  // COST
  // ---------------------------------------------------

  tft.setCursor(5, 97);

  tft.print("Cost    : Rs ");

  tft.print(sessionCost, 2);


  // ---------------------------------------------------
  // FREQUENCY + PF
  // ---------------------------------------------------

  tft.setCursor(5, 112);

  tft.print("F:");

  tft.print(frequency, 1);

  tft.print("Hz PF:");

  tft.print(pf, 2);


  // ---------------------------------------------------
  // INSTRUCTION
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(5, 137);

  tft.println("SWITCH OFF TO STOP");

}


// =====================================================
// RENTING HANDLER
// =====================================================

void handleRenting(unsigned long now) {

  // ===================================================
  // RELAY MUST STAY ON
  // LOW = ON
  // ===================================================

  digitalWrite(RELAY_PIN, LOW);


  // ---------------------------------------------------
  // CALCULATE SESSION ENERGY
  // ---------------------------------------------------

  if (energy >= sessionStartEnergy) {

    sessionEnergy =
      energy - sessionStartEnergy;

  }

  else {

    sessionEnergy = 0;

  }


  // ---------------------------------------------------
  // CALCULATE COST
  // ---------------------------------------------------

  sessionCost =
    sessionEnergy * TARIFF;


  // ---------------------------------------------------
  // UPDATE DISPLAY
  // ---------------------------------------------------

  if (now - lastRentalDisplay >= 1000UL) {

    lastRentalDisplay = now;

    drawRenting();

  }


  // ---------------------------------------------------
  // SWITCH OFF
  // ---------------------------------------------------

  if (digitalRead(RENT_SWITCH) == HIGH) {

    delay(80);


    if (digitalRead(RENT_SWITCH) == HIGH) {

      finishRental();

    }

  }

}


// =====================================================
// FINISH RENTAL
// =====================================================

void finishRental() {

  // ===================================================
  // IMMEDIATELY DISCONNECT LOAD
  // HIGH = RELAY OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  rentalEndTime = millis();


  // ---------------------------------------------------
  // FINAL ENERGY
  // ---------------------------------------------------

  sessionEndEnergy = energy;


  if (sessionEndEnergy >= sessionStartEnergy) {

    sessionEnergy =
      sessionEndEnergy - sessionStartEnergy;

  }

  else {

    sessionEnergy = 0;

  }


  // ---------------------------------------------------
  // FINAL COST
  // ---------------------------------------------------

  sessionCost =
    sessionEnergy * TARIFF;


  beep(2);


  // ---------------------------------------------------
  // SUMMARY
  // ---------------------------------------------------

  state = STATE_SUMMARY;

  stateStartTime = millis();

  drawSummary();

}


// =====================================================
// SUMMARY SCREEN
// =====================================================

void drawSummary() {

  // ===================================================
  // RELAY MUST BE OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  tft.fillScreen(BLACK);


  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.setTextColor(CYAN);

  tft.setTextSize(2);

  tft.setCursor(17, 5);

  tft.println("SUMMARY");


  tft.drawLine(
    0,
    28,
    127,
    28,
    WHITE
  );


  // ---------------------------------------------------
  // ENERGY
  // ---------------------------------------------------

  tft.setTextSize(1);

  tft.setTextColor(WHITE);

  tft.setCursor(5, 40);

  tft.println("ENERGY USED");


  tft.setCursor(5, 54);

  tft.print(sessionEnergy, 4);

  tft.println(" kWh");


  // ---------------------------------------------------
  // RATE
  // ---------------------------------------------------

  tft.setCursor(5, 72);

  tft.print("RATE : Rs ");

  tft.print(TARIFF, 2);

  tft.println("/unit");


  // ---------------------------------------------------
  // TOTAL
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(5, 90);

  tft.print("TOTAL: Rs ");

  tft.print(sessionCost, 2);


  // ---------------------------------------------------
  // SEPARATOR
  // ---------------------------------------------------

  tft.drawLine(
    0,
    107,
    127,
    107,
    WHITE
  );


  // ---------------------------------------------------
  // PAYMENT MESSAGE
  // ---------------------------------------------------

  tft.setTextColor(CYAN);

  tft.setCursor(20, 115);

  tft.println("PAYMENT IN");


  drawSummaryCountdown();

}


// =====================================================
// SUMMARY COUNTDOWN
// =====================================================

void drawSummaryCountdown() {

  unsigned long elapsed =
    millis() - stateStartTime;


  int remaining =
    15 - (elapsed / 1000UL);


  if (remaining < 0) {

    remaining = 0;

  }


  tft.fillRect(
    0,
    130,
    127,
    30,
    BLACK
  );


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

void handleSummary(unsigned long now) {

  // ===================================================
  // RELAY OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  // ---------------------------------------------------
  // COUNTDOWN
  // ---------------------------------------------------

  drawSummaryCountdown();


  // ---------------------------------------------------
  // SWITCH ON DURING SUMMARY
  // ---------------------------------------------------

  if (digitalRead(RENT_SWITCH) == LOW) {

    beep(1);


    // -------------------------------------------------
    // DO NOT START NEW RENTAL
    // -------------------------------------------------

    state = STATE_WAIT;

    stateStartTime = now;

    drawWait();

    return;

  }


  // ---------------------------------------------------
  // SUMMARY FINISHED
  // ---------------------------------------------------

  if (now - stateStartTime >= SUMMARY_TIME) {

    state = STATE_PAYMENT;

    stateStartTime = now;

    lastWarningBeep = 0;

    drawPayment();

  }

}


// =====================================================
// PAYMENT SCREEN
// =====================================================

void drawPayment() {

  // ===================================================
  // RELAY MUST BE OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  tft.fillScreen(BLACK);


  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.setTextColor(GREEN);

  tft.setTextSize(2);

  tft.setCursor(20, 3);

  tft.println("PAYMENT");


  // ---------------------------------------------------
  // AMOUNT
  // ---------------------------------------------------

  tft.setTextSize(1);

  tft.setTextColor(YELLOW);

  tft.setCursor(38, 25);

  tft.print("Rs ");

  tft.println(sessionCost, 2);


  // ---------------------------------------------------
  // QR
  // ---------------------------------------------------

  drawDummyQR(32, 42);


  // ---------------------------------------------------
  // MESSAGE
  // ---------------------------------------------------

  tft.setTextColor(WHITE);

  tft.setCursor(34, 110);

  tft.println("SCAN TO PAY");


  tft.setTextColor(RED);

  tft.setCursor(28, 123);

  tft.println("QR UNDER WORK");


  drawPaymentCountdown();

}


// =====================================================
// PAYMENT COUNTDOWN
// =====================================================

void drawPaymentCountdown() {

  unsigned long elapsed =
    millis() - stateStartTime;


  int remaining =
    15 - (elapsed / 1000UL);


  if (remaining < 0) {

    remaining = 0;

  }


  tft.fillRect(
    0,
    141,
    127,
    19,
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

void handlePayment(unsigned long now) {

  // ===================================================
  // RELAY OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  // ---------------------------------------------------
  // COUNTDOWN
  // ---------------------------------------------------

  drawPaymentCountdown();


  // ---------------------------------------------------
  // SWITCH ON DURING PAYMENT
  // ---------------------------------------------------

  if (digitalRead(RENT_SWITCH) == LOW) {

    if (now - lastWarningBeep >= 2000UL) {

      lastWarningBeep = now;

      beep(1);


      // -----------------------------------------------
      // WARNING MESSAGE
      // -----------------------------------------------

      tft.fillRect(
        0,
        15,
        127,
        10,
        BLACK
      );


      tft.setTextColor(RED);

      tft.setTextSize(1);

      tft.setCursor(18, 15);

      tft.print("PLEASE WAIT...");

    }

  }


  // ---------------------------------------------------
  // PAYMENT FINISHED
  // ---------------------------------------------------

  if (now - stateStartTime >= PAYMENT_TIME) {

    // -----------------------------------------------
    // SWITCH OFF -> READY
    // -----------------------------------------------

    if (digitalRead(RENT_SWITCH) == HIGH) {

      digitalWrite(RELAY_PIN, HIGH);

      state = STATE_READY;

      stateStartTime = now;

      lastReadyDisplay = 0;

      drawReady();

      return;

    }


    // -----------------------------------------------
    // SWITCH STILL ON -> WAIT
    // -----------------------------------------------

    state = STATE_WAIT;

    stateStartTime = now;

    drawWait();

  }

}


// =====================================================
// WAIT SCREEN
// =====================================================

void drawWait() {

  // ===================================================
  // RELAY OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  tft.fillScreen(BLACK);


  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.setTextColor(RED);

  tft.setTextSize(2);

  tft.setCursor(40, 15);

  tft.println("WAIT");


  tft.drawLine(
    0,
    40,
    127,
    40,
    WHITE
  );


  // ---------------------------------------------------
  // MESSAGE
  // ---------------------------------------------------

  tft.setTextSize(1);

  tft.setTextColor(WHITE);

  tft.setCursor(12, 55);

  tft.println("Previous rental");


  tft.setCursor(12, 70);

  tft.println("is still processing.");


  // ---------------------------------------------------
  // PLEASE WAIT
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(12, 88);

  tft.println("Please wait...");


  // ---------------------------------------------------
  // LOCK
  // ---------------------------------------------------

  tft.setTextColor(CYAN);

  tft.setCursor(12, 110);

  tft.println("RENTAL LOCK ACTIVE");


  // ---------------------------------------------------
  // SWITCH
  // ---------------------------------------------------

  tft.setTextColor(WHITE);

  tft.setCursor(25, 128);

  tft.println("Switch OFF");


  drawWaitCountdown();

}


// =====================================================
// WAIT COUNTDOWN
// =====================================================

void drawWaitCountdown() {

  unsigned long elapsed =
    millis() - stateStartTime;


  int remaining =
    15 - (elapsed / 1000UL);


  if (remaining < 0) {

    remaining = 0;

  }


  tft.fillRect(
    35,
    140,
    70,
    20,
    BLACK
  );


  tft.setTextColor(WHITE);

  tft.setTextSize(2);

  tft.setCursor(45, 140);


  if (remaining < 10) {

    tft.print("0");

  }


  tft.print(remaining);

  tft.print("s");

}


// =====================================================
// WAIT HANDLER
// =====================================================

void handleWait(unsigned long now) {

  // ===================================================
  // RELAY OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  // ---------------------------------------------------
  // COUNTDOWN
  // ---------------------------------------------------

  drawWaitCountdown();


  // ---------------------------------------------------
  // BEEP WHILE SWITCH IS ON
  // ---------------------------------------------------

  if (
    digitalRead(RENT_SWITCH) == LOW &&
    now - lastWarningBeep >= 2000UL
  ) {

    lastWarningBeep = now;

    beep(1);

  }


  // ---------------------------------------------------
  // WAIT FINISHED
  // ---------------------------------------------------

  if (now - stateStartTime >= WAIT_TIME) {

    // -------------------------------------------------
    // SWITCH OFF
    // -------------------------------------------------

    if (digitalRead(RENT_SWITCH) == HIGH) {

      digitalWrite(RELAY_PIN, HIGH);

      state = STATE_READY;

      stateStartTime = now;

      lastReadyDisplay = 0;

      drawReady();

    }


    // -------------------------------------------------
    // SWITCH STILL ON
    // -------------------------------------------------

    else {

      // Restart another 15 second lock

      stateStartTime = now;

      drawWait();

    }

  }

}


// =====================================================
// ERROR SCREEN
// =====================================================

void showError(const char *message) {

  // ===================================================
  // VERY IMPORTANT:
  // V1 RELAY LOGIC
  // HIGH = OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  tft.fillScreen(BLACK);


  tft.setTextColor(RED);

  tft.setTextSize(2);

  tft.setCursor(28, 15);

  tft.println("ERROR");


  tft.setTextSize(1);

  tft.setCursor(10, 55);

  tft.setTextColor(WHITE);

  tft.println(message);


  tft.setCursor(10, 75);

  tft.println("Relay OFF");


  beep(3);


  state = STATE_ERROR;

  stateStartTime = millis();

}


// =====================================================
// ERROR HANDLER
// =====================================================

void handleError(unsigned long now) {

  // ===================================================
  // SAFETY:
  // RELAY MUST BE OFF
  // ===================================================

  digitalWrite(RELAY_PIN, HIGH);


  if (now - stateStartTime >= 5000UL) {

    if (digitalRead(RENT_SWITCH) == HIGH) {

      state = STATE_READY;

      stateStartTime = now;

      lastReadyDisplay = 0;

      drawReady();

    }

  }

}


// =====================================================
// BUZZER
// =====================================================

void beep(byte count) {

  for (byte i = 0; i < count; i++) {

    digitalWrite(
      BUZZER_PIN,
      HIGH
    );

    delay(100);


    digitalWrite(
      BUZZER_PIN,
      LOW
    );

    delay(100);

  }

}


// =====================================================
// DUMMY QR
// =====================================================

void drawDummyQR(int x, int y) {

  const int module = 3;

  const int size = 21;


  // ---------------------------------------------------
  // WHITE BACKGROUND
  // ---------------------------------------------------

  tft.fillRect(
    x,
    y,
    size * module,
    size * module,
    WHITE
  );


  // ---------------------------------------------------
  // QR PATTERN
  // ---------------------------------------------------

  for (int row = 0; row < size; row++) {

    for (int col = 0; col < size; col++) {

      bool blackPixel = false;


      // ------------------------------------------------
      // FINDER PATTERNS
      // ------------------------------------------------

      if (
        (row < 7 && col < 7) ||
        (row < 7 && col >= size - 7) ||
        (row >= size - 7 && col < 7)
      ) {

        int fx;
        int fy;


        // X coordinate

        if (col < 7) {

          fx = col;

        }

        else {

          fx = col - (size - 7);

        }


        // Y coordinate

        if (row < 7) {

          fy = row;

        }

        else {

          fy = row - (size - 7);

        }


        if (
          fx == 0 || fx == 6 ||
          fy == 0 || fy == 6 ||
          (
            fx >= 2 &&
            fx <= 4 &&
            fy >= 2 &&
            fy <= 4
          )
        ) {

          blackPixel = true;

        }

      }


      // ------------------------------------------------
      // PSEUDO QR DATA
      // ------------------------------------------------

      else {

        blackPixel =
          (
            (
              row * 13 +
              col * 7 +
              row * col
            ) % 5
          ) < 2;

      }


      // ------------------------------------------------
      // DRAW MODULE
      // ------------------------------------------------

      if (blackPixel) {

        tft.fillRect(
          x + col * module,
          y + row * module,
          module,
          module,
          BLACK
        );

      }

    }

  }

}