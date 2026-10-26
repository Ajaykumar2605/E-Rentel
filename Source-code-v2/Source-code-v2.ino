#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <PZEM004Tv40_R4.h>

// =====================================================
// PIN CONFIGURATION
// =====================================================

#define TFT_CS       53
#define TFT_DC       49
#define TFT_RST      48

#define RENT_SWITCH  22
#define RELAY_PIN    24
#define BUZZER_PIN   26

// =====================================================
// TARIFF
// =====================================================

#define TARIFF 12.0f

// =====================================================
// RELAY LOGIC
// Your relay:
// LOW  = ON
// HIGH = OFF
// =====================================================

#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

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
// Mega:
// TX2 = 16
// RX2 = 17
// =====================================================

PZEM004Tv40_R4 pzem(&Serial2);

// =====================================================
// DISPLAY SIZE
// PORTRAIT
// 1.8" TFT
// =====================================================

#define SCREEN_W 128
#define SCREEN_H 160

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
// PZEM VALUES
// =====================================================

float voltage   = 0.0;
float current   = 0.0;
float power     = 0.0;
float energy    = 0.0;
float frequency = 0.0;
float pf        = 0.0;

// =====================================================
// RENT SESSION
// =====================================================

float sessionStartEnergy = 0.0;
float sessionEndEnergy   = 0.0;
float sessionEnergy      = 0.0;
float sessionCost        = 0.0;

unsigned long rentalStartTime = 0;
unsigned long rentalEndTime   = 0;

// =====================================================
// TIMING
// =====================================================

unsigned long stateStartTime = 0;
unsigned long lastPzemRead   = 0;
unsigned long lastDisplay    = 0;
unsigned long lastCountdown  = 0;

// =====================================================
// TIME SETTINGS
// =====================================================

const unsigned long WELCOME_TIME = 5000;

const unsigned long SUMMARY_TIME = 15000;

const unsigned long PAYMENT_TIME = 15000;

const unsigned long WAIT_TIME = 15000;

const unsigned long PZEM_INTERVAL = 1000;

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
  Serial.println("SMART RENTAL POWER CONTROLLER");
  Serial.println("--------------------------------");

  // ---------------------------------------------------
  // RENT SWITCH
  // ---------------------------------------------------

  pinMode(RENT_SWITCH, INPUT_PULLUP);

  // ---------------------------------------------------
  // RELAY
  // ---------------------------------------------------

  pinMode(RELAY_PIN, OUTPUT);

  // SAFETY:
  // Relay OFF when Arduino starts
  digitalWrite(RELAY_PIN, RELAY_OFF);

  // ---------------------------------------------------
  // BUZZER
  // ---------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  // ---------------------------------------------------
  // TFT
  // ---------------------------------------------------

  tft.initR(INITR_BLACKTAB);

  // IMPORTANT:
  // Portrait 128 x 160
  tft.setRotation(0);

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // PZEM
  // ---------------------------------------------------

  pzem.begin();

  // ---------------------------------------------------
  // START
  // ---------------------------------------------------

  state = STATE_WELCOME;

  stateStartTime = millis();

  drawWelcome();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  unsigned long now = millis();

  updatePZEM();

  switch (state) {

    case STATE_WELCOME:
      handleWelcome(now);
      break;

    case STATE_READY:
      handleReady();
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

    Serial.print("Voltage: ");
    Serial.print(voltage, 1);

    Serial.print(" V | Current: ");
    Serial.print(current, 2);

    Serial.print(" A | Power: ");
    Serial.print(power, 1);

    Serial.print(" W | Energy: ");
    Serial.print(energy, 3);

    Serial.print(" kWh | Frequency: ");
    Serial.print(frequency, 1);

    Serial.print(" Hz | PF: ");
    Serial.println(pf, 2);

  } else {

    Serial.print("PZEM ERROR: ");

    Serial.println(
      pzem.getLastError()
    );
  }
}

// =====================================================
// WELCOME SCREEN
// =====================================================

void drawWelcome() {

  tft.fillScreen(BLACK);

  // TITLE
  tft.setTextColor(CYAN);
  tft.setTextSize(2);

  tft.setCursor(20, 20);
  tft.println("SMART");

  tft.setCursor(20, 45);
  tft.println("RENTAL");

  // SUBTITLE
  tft.setTextColor(WHITE);
  tft.setTextSize(1);

  tft.setCursor(23, 75);
  tft.println("POWER CONTROLLER");

  tft.setCursor(25, 100);
  tft.println("INITIALIZING");

  // Loading dots
  tft.setCursor(55, 120);
  tft.println(".");
}

// =====================================================
// WELCOME HANDLER
// =====================================================

void handleWelcome(unsigned long now) {

  static unsigned long lastAnimation = 0;

  static byte dots = 0;

  if (now - lastAnimation >= 400) {

    lastAnimation = now;

    tft.fillRect(
      50,
      118,
      35,
      12,
      BLACK
    );

    tft.setCursor(50, 120);

    tft.setTextColor(YELLOW);

    for (byte i = 0; i < dots; i++) {
      tft.print(".");
    }

    dots++;

    if (dots > 4) {
      dots = 0;
    }
  }

  if (now - stateStartTime >= WELCOME_TIME) {

    state = STATE_READY;

    stateStartTime = now;

    drawReady();
  }
}

// =====================================================
// READY SCREEN
// 128 x 160
// =====================================================

void drawReady() {

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.setTextSize(2);

  tft.setTextColor(GREEN);

  tft.setCursor(35, 5);

  tft.println("READY");

  // separator
  tft.drawFastHLine(
    0,
    28,
    SCREEN_W,
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
  // BOTTOM LINE
  // ---------------------------------------------------

  tft.drawFastHLine(
    0,
    113,
    SCREEN_W,
    WHITE
  );

  tft.setTextColor(YELLOW);

  tft.setCursor(7, 122);

  tft.println("TURN SWITCH ON");

  tft.setCursor(25, 137);

  tft.println("TO RENT");
}

// =====================================================
// READY HANDLER
// =====================================================

void handleReady() {

  // Keep relay OFF
  digitalWrite(
    RELAY_PIN,
    RELAY_OFF
  );

  if (millis() - lastDisplay >= 1000) {

    lastDisplay = millis();

    drawReady();
  }

  // Switch ON
  if (digitalRead(RENT_SWITCH) == LOW) {

    startRental();
  }
}

// =====================================================
// START RENTAL
// =====================================================

void startRental() {

  // Record starting energy
  sessionStartEnergy = energy;

  rentalStartTime = millis();

  // RELAY ON
  digitalWrite(
    RELAY_PIN,
    RELAY_ON
  );

  beep(1);

  state = STATE_RENTING;

  stateStartTime = millis();

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

  tft.setTextSize(2);

  tft.setTextColor(GREEN);

  tft.setCursor(25, 5);

  tft.println("RENTING");

  tft.drawFastHLine(
    0,
    28,
    SCREEN_W,
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

  float usedEnergy =
    energy - sessionStartEnergy;

  if (usedEnergy < 0) {
    usedEnergy = 0;
  }

  tft.setCursor(5, 82);

  tft.print("Energy  : ");

  tft.print(usedEnergy, 3);

  tft.println(" kWh");

  // ---------------------------------------------------
  // COST
  // ---------------------------------------------------

  float currentCost =
    usedEnergy * TARIFF;

  tft.setCursor(5, 97);

  tft.print("Cost    : Rs ");

  tft.print(currentCost, 2);

  // ---------------------------------------------------
  // FREQUENCY / PF
  // ---------------------------------------------------

  tft.setCursor(5, 112);

  tft.print("Freq: ");

  tft.print(frequency, 1);

  tft.print(" Hz");

  tft.setCursor(75, 112);

  tft.print("PF:");

  tft.print(pf, 2);

  // ---------------------------------------------------
  // INSTRUCTION
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(10, 137);

  tft.println("SWITCH OFF TO STOP");
}

// =====================================================
// RENTING HANDLER
// =====================================================

void handleRenting(unsigned long now) {

  // Relay stays ON
  digitalWrite(
    RELAY_PIN,
    RELAY_ON
  );

  if (now - lastDisplay >= 1000) {

    lastDisplay = now;

    drawRenting();
  }

  // Switch OFF
  if (digitalRead(RENT_SWITCH) == HIGH) {

    finishRental();
  }
}

// =====================================================
// FINISH RENTAL
// =====================================================

void finishRental() {

  // Immediately turn load OFF
  digitalWrite(
    RELAY_PIN,
    RELAY_OFF
  );

  rentalEndTime = millis();

  // Final energy
  sessionEndEnergy = energy;

  sessionEnergy =
    sessionEndEnergy -
    sessionStartEnergy;

  if (sessionEnergy < 0) {
    sessionEnergy = 0;
  }

  // Calculate cost
  sessionCost =
    sessionEnergy * TARIFF;

  beep(2);

  state = STATE_SUMMARY;

  stateStartTime = millis();

  lastCountdown = millis();

  drawSummary();
}

// =====================================================
// SUMMARY SCREEN
// =====================================================

void drawSummary() {

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.setTextSize(2);

  tft.setTextColor(CYAN);

  tft.setCursor(28, 5);

  tft.println("SUMMARY");

  tft.drawFastHLine(
    0,
    28,
    SCREEN_W,
    WHITE
  );

  // ---------------------------------------------------
  // ENERGY
  // ---------------------------------------------------

  tft.setTextSize(1);

  tft.setTextColor(WHITE);

  tft.setCursor(5, 40);

  tft.println("ENERGY USED:");

  tft.setTextColor(CYAN);

  tft.setCursor(5, 54);

  tft.print(sessionEnergy, 4);

  tft.println(" kWh");

  // ---------------------------------------------------
  // TARIFF
  // ---------------------------------------------------

  tft.setTextColor(WHITE);

  tft.setCursor(5, 72);

  tft.print("RATE: Rs ");

  tft.print(TARIFF, 2);

  tft.println(" / UNIT");

  // ---------------------------------------------------
  // TOTAL
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(5, 90);

  tft.print("TOTAL: Rs ");

  tft.print(sessionCost, 2);

  // ---------------------------------------------------
  // COUNTDOWN
  // ---------------------------------------------------

  tft.drawFastHLine(
    0,
    107,
    SCREEN_W,
    WHITE
  );

  tft.setTextColor(WHITE);

  tft.setCursor(28, 115);

  tft.println("PAYMENT IN");

  drawSummaryCountdown(15);
}

// =====================================================
// SUMMARY COUNTDOWN
// =====================================================

void drawSummaryCountdown(int seconds) {

  // Clear countdown area
  tft.fillRect(
    43,
    130,
    50,
    20,
    BLACK
  );

  tft.setTextSize(2);

  tft.setTextColor(YELLOW);

  if (seconds < 10) {

    tft.setCursor(58, 130);

  } else {

    tft.setCursor(53, 130);
  }

  tft.print(seconds);

  tft.setTextSize(1);
}

// =====================================================
// SUMMARY HANDLER
// =====================================================

void handleSummary(unsigned long now) {

  // ---------------------------------------------------
  // User tries to start again
  // ---------------------------------------------------

  if (digitalRead(RENT_SWITCH) == LOW) {

    beep(1);

    state = STATE_WAIT;

    stateStartTime = millis();

    drawWait();

    return;
  }

  // ---------------------------------------------------
  // Countdown
  // ---------------------------------------------------

  if (now - lastCountdown >= 1000) {

    lastCountdown = now;

    unsigned long elapsed =
      now - stateStartTime;

    int remaining =
      15 - (elapsed / 1000);

    if (remaining < 0) {
      remaining = 0;
    }

    drawSummaryCountdown(
      remaining
    );
  }

  // ---------------------------------------------------
  // 15 SEC COMPLETE
  // ---------------------------------------------------

  if (now - stateStartTime >= SUMMARY_TIME) {

    state = STATE_PAYMENT;

    stateStartTime = millis();

    lastCountdown = millis();

    drawPayment();
  }
}

// =====================================================
// PAYMENT SCREEN
// =====================================================

void drawPayment() {

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.setTextSize(2);

  tft.setTextColor(GREEN);

  tft.setCursor(28, 3);

  tft.println("PAYMENT");

  tft.setTextSize(1);

  tft.setTextColor(WHITE);

  // ---------------------------------------------------
  // AMOUNT
  // ---------------------------------------------------

  tft.setCursor(20, 25);

  tft.print("AMOUNT: Rs ");

  tft.print(sessionCost, 2);

  // ---------------------------------------------------
  // QR
  // ---------------------------------------------------

  drawDummyQR(32, 42);

  // ---------------------------------------------------
  // MESSAGE
  // ---------------------------------------------------

  tft.setTextColor(YELLOW);

  tft.setCursor(28, 110);

  tft.println("SCAN TO PAY");

  tft.setTextColor(WHITE);

  tft.setCursor(20, 123);

  tft.println("QR UNDER WORK");

  // Countdown
  drawPaymentCountdown(15);
}

// =====================================================
// PAYMENT COUNTDOWN
// =====================================================

void drawPaymentCountdown(int seconds) {

  tft.fillRect(
    0,
    140,
    SCREEN_W,
    18,
    BLACK
  );

  tft.setTextSize(1);

  tft.setTextColor(YELLOW);

  tft.setCursor(37, 145);

  tft.print("READY IN ");

  tft.print(seconds);

  tft.print(" SEC");
}

// =====================================================
// PAYMENT HANDLER
// =====================================================

void handlePayment(unsigned long now) {

  // Relay MUST remain OFF
  digitalWrite(
    RELAY_PIN,
    RELAY_OFF
  );

  // ---------------------------------------------------
  // Switch ON during payment
  // ---------------------------------------------------

  static unsigned long lastWarning = 0;

  if (digitalRead(RENT_SWITCH) == LOW) {

    if (now - lastWarning >= 2000) {

      lastWarning = now;

      beep(1);

      drawWait();
    }
  }

  // ---------------------------------------------------
  // Countdown
  // ---------------------------------------------------

  if (now - lastCountdown >= 1000) {

    lastCountdown = now;

    unsigned long elapsed =
      now - stateStartTime;

    int remaining =
      15 - (elapsed / 1000);

    if (remaining < 0) {
      remaining = 0;
    }

    drawPaymentCountdown(
      remaining
    );
  }

  // ---------------------------------------------------
  // PAYMENT COMPLETE
  // ---------------------------------------------------

  if (now - stateStartTime >= PAYMENT_TIME) {

    digitalWrite(
      RELAY_PIN,
      RELAY_OFF
    );

    state = STATE_READY;

    stateStartTime = now;

    drawReady();
  }
}

// =====================================================
// WAIT SCREEN
// =====================================================

void drawWait() {

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // HEADER
  // ---------------------------------------------------

  tft.setTextSize(2);

  tft.setTextColor(YELLOW);

  tft.setCursor(43, 15);

  tft.println("WAIT");

  // ---------------------------------------------------
  // MESSAGE
  // ---------------------------------------------------

  tft.setTextSize(1);

  tft.setTextColor(WHITE);

  tft.setCursor(17, 50);

  tft.println("Previous rental");

  tft.setCursor(20, 65);

  tft.println("is processing.");

  tft.setCursor(25, 82);

  tft.println("Please wait...");

  // ---------------------------------------------------
  // WARNING
  // ---------------------------------------------------

  tft.setTextColor(RED);

  tft.setCursor(13, 110);

  tft.println("RENTAL LOCK ACTIVE");

  tft.setTextColor(WHITE);

  tft.setCursor(20, 128);

  tft.println("Switch OFF");
}

// =====================================================
// WAIT HANDLER
// =====================================================

void handleWait(unsigned long now) {

  // Relay OFF
  digitalWrite(
    RELAY_PIN,
    RELAY_OFF
  );

  // ---------------------------------------------------
  // Wait until timer completes
  // ---------------------------------------------------

  if (now - stateStartTime >= WAIT_TIME) {

    // IMPORTANT:
    // Do not start automatically if switch
    // is still ON.

    if (digitalRead(RENT_SWITCH) == HIGH) {

      state = STATE_READY;

      stateStartTime = now;

      drawReady();
    }

    else {

      // Switch is still ON.
      // Keep waiting until user turns it OFF.

      tft.setTextColor(RED);

      tft.setCursor(12, 145);

      tft.println("TURN SWITCH OFF");
    }
  }
}

// =====================================================
// ERROR SCREEN
// =====================================================

void showError(
  const char *message
) {

  // Relay OFF
  digitalWrite(
    RELAY_PIN,
    RELAY_OFF
  );

  tft.fillScreen(BLACK);

  // ---------------------------------------------------
  // ERROR
  // ---------------------------------------------------

  tft.setTextSize(2);

  tft.setTextColor(RED);

  tft.setCursor(35, 15);

  tft.println("ERROR");

  // ---------------------------------------------------
  // MESSAGE
  // ---------------------------------------------------

  tft.setTextSize(1);

  tft.setTextColor(WHITE);

  tft.setCursor(5, 55);

  tft.println(message);

  // ---------------------------------------------------
  // BUZZER
  // ---------------------------------------------------

  beep(3);

  state = STATE_ERROR;

  stateStartTime = millis();
}

// =====================================================
// ERROR HANDLER
// =====================================================

void handleError(unsigned long now) {

  digitalWrite(
    RELAY_PIN,
    RELAY_OFF
  );

  if (now - stateStartTime >= 5000) {

    state = STATE_READY;

    drawReady();
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

void drawDummyQR(
  int x,
  int y
) {

  const int module = 3;

  const int size = 21;

  // 63 x 63 QR
  // Fits easily inside 128 x 160

  tft.fillRect(
    x,
    y,
    size * module,
    size * module,
    WHITE
  );

  for (
    int row = 0;
    row < size;
    row++
  ) {

    for (
      int col = 0;
      col < size;
      col++
    ) {

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

        if (col < 7) {

          fx = col;

        } else {

          fx =
            col - (size - 7);
        }

        if (row < 7) {

          fy = row;

        } else {

          fy =
            row - (size - 7);
        }

        if (
          fx == 0 ||
          fx == 6 ||
          fy == 0 ||
          fy == 6 ||
          (
            fx >= 2 &&
            fx <= 4 &&
            fy >= 2 &&
            fy <= 4
          )
        ) {

          blackPixel = true;
        }

      } else {

        // Pseudo QR pattern
        blackPixel =
          (
            (
              row * 13 +
              col * 7 +
              row * col
            ) % 5
          ) < 2;
      }

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