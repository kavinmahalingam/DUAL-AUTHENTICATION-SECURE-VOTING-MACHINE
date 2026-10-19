
#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_Fingerprint.h>

// =====================================================
// ESP32 EVM MODULE 1
// RFID + TFT + R307 FINGERPRINT + CANCEL BUTTON
// =====================================================

// RC522 - HSPI
#define RFID_SCK   22
#define RFID_MISO  35
#define RFID_MOSI  21
#define RFID_SS     5
#define RFID_RST   27

// ILI9341 TFT - VSPI
#define TFT_SCK    18
#define TFT_MISO   19
#define TFT_MOSI   23
#define TFT_CS     14
#define TFT_DC     26
#define TFT_RST    25

// Outputs
#define GREEN_LED  32
#define RED_LED    33
#define BUZZER      4

// Cancel button: GPIO34 to GND when pressed.
// External pull-up resistor connects GPIO34 to 3.3V.
#define CANCEL_BUTTON 34

// R307 UART2
#define FINGER_RX   16
#define FINGER_TX   17
#define FINGER_BAUD 57600

SPIClass rfidSPI(HSPI);
SPIClass tftSPI(VSPI);

Adafruit_ILI9341 tft(&tftSPI, TFT_DC, TFT_CS, TFT_RST);
HardwareSerial fingerSerial(2);
Adafruit_Fingerprint finger(&fingerSerial);
Preferences prefs;

const uint16_t C_WHITE  = ILI9341_WHITE;
const uint16_t C_CYAN   = ILI9341_CYAN;
const uint16_t C_GREEN  = ILI9341_GREEN;
const uint16_t C_RED    = ILI9341_RED;
const uint16_t C_BLUE   = ILI9341_BLUE;
const uint16_t C_YELLOW = ILI9341_YELLOW;
const uint16_t C_BLACK  = ILI9341_BLACK;

bool rfidReady = false;
bool fingerprintReady = false;
bool busy = false;
bool cancelRequested = false;

// =====================================================
// RC522 REGISTERS
// =====================================================

#define CommandReg       0x01
#define ComIrqReg        0x04
#define ErrorReg         0x06
#define FIFODataReg      0x09
#define FIFOLevelReg     0x0A
#define BitFramingReg    0x0D
#define ModeReg          0x11
#define TxControlReg     0x14
#define TxASKReg         0x15
#define TModeReg         0x2A
#define TPrescalerReg    0x2B
#define TReloadRegH      0x2C
#define TReloadRegL      0x2D
#define VersionReg       0x37

#define PCD_IDLE         0x00
#define PCD_TRANSCEIVE   0x0C
#define PCD_SOFTRESET    0x0F
#define PICC_REQA        0x26
#define PICC_SEL_CL1     0x93

// =====================================================
// DISPLAY
// =====================================================

void drawCentered(const String &text, int y,
                  uint16_t color, uint8_t size = 2) {
  tft.setTextSize(size);
  tft.setTextColor(color, C_BLACK);

  int16_t x1, y1;
  uint16_t w, h;

  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  tft.setCursor((tft.width() - w) / 2, y);
  tft.print(text);
}

void drawHeader() {
  tft.fillScreen(C_BLACK);
  tft.drawRoundRect(5, 5, 230, 310, 12, C_CYAN);
  drawCentered("SECURE VOTING", 18, C_CYAN, 2);
  drawCentered("DUAL AUTHENTICATION", 42, C_WHITE, 1);
  tft.drawFastHLine(20, 62, 200, C_BLUE);
}

void drawCircleLogo(int pulse = 0) {
  int cx = 120;
  int cy = 119;
  int radius = 29 + pulse;

  tft.drawCircle(cx, cy, radius, C_CYAN);
  tft.drawCircle(cx, cy, radius - 5, C_BLUE);
  tft.drawCircle(cx, cy, 13, C_GREEN);
  tft.drawLine(cx - 7, cy, cx - 1, cy + 6, C_GREEN);
  tft.drawLine(cx - 1, cy + 6, cx + 9, cy - 7, C_GREEN);
}

void showWelcome() {
  drawHeader();
  drawCircleLogo();
  drawCentered("WELCOME VOTER", 170, C_WHITE, 2);
  drawCentered("Scan your RFID card", 205, C_YELLOW, 2);
  drawCentered("RFID + FINGERPRINT", 245, C_CYAN, 1);
}

void showVoterID(const String &voterID) {
  drawHeader();
  drawCircleLogo();
  drawCentered("RFID VERIFIED", 140, C_GREEN, 2);
  drawCentered("VOTER ID:", 180, C_WHITE, 2);
  drawCentered(voterID, 210, C_YELLOW, 2);
  drawCentered("Press CANCEL to stop", 260, C_CYAN, 1);
}

void showFingerprintPrompt() {
  drawHeader();
  drawCircleLogo();
  drawCentered("RFID VERIFIED", 140, C_GREEN, 2);
  drawCentered("PLACE YOUR FINGER", 185, C_YELLOW, 2);
  drawCentered("ON THE SENSOR", 215, C_WHITE, 2);
  drawCentered("Press CANCEL to stop", 260, C_CYAN, 1);
}

void showSuccess() {
  drawHeader();

  for (int r = 10; r <= 40; r += 5) {
    if (digitalRead(CANCEL_BUTTON) == LOW) {
      cancelRequested = true;
      return;
    }

    tft.drawCircle(120, 130, r, C_GREEN);
    delay(35);
  }

  tft.drawLine(99, 130, 114, 145, C_GREEN);
  tft.drawLine(114, 145, 145, 111, C_GREEN);

  drawCentered("AUTHENTICATED", 195, C_GREEN, 2);
  drawCentered("DEMO ACCESS GRANTED", 226, C_WHITE, 1);
}

void showFailure(const String &message) {
  drawHeader();
  tft.drawCircle(120, 125, 36, C_RED);
  tft.drawLine(103, 108, 137, 142, C_RED);
  tft.drawLine(137, 108, 103, 142, C_RED);
  drawCentered("VERIFICATION FAILED", 185, C_RED, 2);
  drawCentered(message, 220, C_YELLOW, 1);
}

void resetFeedback() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);
}

void cancelSession() {
  resetFeedback();

  tft.fillScreen(C_BLACK);
  drawCentered("SESSION CANCELLED", 110, C_YELLOW, 2);
  drawCentered("Scan next card", 150, C_WHITE, 1);

  Serial.println("AUTH:CANCELLED");

  // Wait for button release to prevent repeated cancellation.
  while (digitalRead(CANCEL_BUTTON) == LOW) {
    delay(10);
  }

  delay(700);
  cancelRequested = false;
  showWelcome();
}

// =====================================================
// CANCEL BUTTON
// =====================================================

bool checkCancel() {
  if (digitalRead(CANCEL_BUTTON) != LOW) {
    return false;
  }

  delay(25);

  if (digitalRead(CANCEL_BUTTON) != LOW) {
    return false;
  }

  cancelRequested = true;

  while (digitalRead(CANCEL_BUTTON) == LOW) {
    delay(10);
  }

  delay(30);
  return true;
}

bool waitWithCancel(unsigned long durationMs) {
  unsigned long start = millis();

  while (millis() - start < durationMs) {
    if (checkCancel()) {
      return false;
    }

    delay(10);
  }

  return true;
}

// =====================================================
// RC522 DRIVER
// =====================================================

void rfidWrite(uint8_t reg, uint8_t value) {
  rfidSPI.beginTransaction(
    SPISettings(4000000, MSBFIRST, SPI_MODE0)
  );

  digitalWrite(RFID_SS, LOW);
  rfidSPI.transfer((reg << 1) & 0x7E);
  rfidSPI.transfer(value);
  digitalWrite(RFID_SS, HIGH);

  rfidSPI.endTransaction();
}

uint8_t rfidRead(uint8_t reg) {
  rfidSPI.beginTransaction(
    SPISettings(4000000, MSBFIRST, SPI_MODE0)
  );

  digitalWrite(RFID_SS, LOW);
  rfidSPI.transfer(0x80 | ((reg << 1) & 0x7E));
  uint8_t value = rfidSPI.transfer(0x00);
  digitalWrite(RFID_SS, HIGH);

  rfidSPI.endTransaction();
  return value;
}

void rfidSetMask(uint8_t reg, uint8_t mask) {
  rfidWrite(reg, rfidRead(reg) | mask);
}

void rfidClearMask(uint8_t reg, uint8_t mask) {
  rfidWrite(reg, rfidRead(reg) & (uint8_t)(~mask));
}

void rfidAntennaOn() {
  if ((rfidRead(TxControlReg) & 0x03) != 0x03) {
    rfidSetMask(TxControlReg, 0x03);
  }
}

void rfidInit() {
  pinMode(RFID_SS, OUTPUT);
  pinMode(RFID_RST, OUTPUT);

  digitalWrite(RFID_SS, HIGH);
  digitalWrite(RFID_RST, HIGH);

  rfidSPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_SS);

  digitalWrite(RFID_RST, LOW);
  delay(5);
  digitalWrite(RFID_RST, HIGH);
  delay(50);

  rfidWrite(CommandReg, PCD_SOFTRESET);
  delay(50);

  rfidWrite(TModeReg, 0x8D);
  rfidWrite(TPrescalerReg, 0x3E);
  rfidWrite(TReloadRegH, 0x00);
  rfidWrite(TReloadRegL, 30);
  rfidWrite(TxASKReg, 0x40);
  rfidWrite(ModeReg, 0x3D);

  rfidAntennaOn();
  delay(20);

  uint8_t version = rfidRead(VersionReg);
  Serial.printf("RC522 version: 0x%02X\n", version);

  rfidReady = (version != 0x00 && version != 0xFF);
}

bool rfidTransceive(const uint8_t *sendData,
                    uint8_t sendLen,
                    uint8_t *backData,
                    uint8_t *backLen,
                    uint8_t txLastBits = 0) {
  rfidWrite(CommandReg, PCD_IDLE);
  rfidWrite(ComIrqReg, 0x7F);
  rfidWrite(FIFOLevelReg, 0x80);

  for (uint8_t i = 0; i < sendLen; i++) {
    rfidWrite(FIFODataReg, sendData[i]);
  }

  rfidWrite(BitFramingReg, txLastBits);
  rfidWrite(CommandReg, PCD_TRANSCEIVE);
  rfidSetMask(BitFramingReg, 0x80);

  unsigned long start = millis();
  bool completed = false;

  while (millis() - start < 40) {
    uint8_t irq = rfidRead(ComIrqReg);

    if (irq & 0x30) {
      completed = true;
      break;
    }

    if (irq & 0x01) {
      break;
    }

    delay(1);
  }

  rfidClearMask(BitFramingReg, 0x80);

  if (!completed || (rfidRead(ErrorReg) & 0x1B)) {
    return false;
  }

  uint8_t count = rfidRead(FIFOLevelReg);

  if (count > *backLen) {
    return false;
  }

  *backLen = count;

  for (uint8_t i = 0; i < count; i++) {
    backData[i] = rfidRead(FIFODataReg);
  }

  return true;
}

bool rfidRequestA() {
  uint8_t command = PICC_REQA;
  uint8_t response[2] = {0};
  uint8_t length = sizeof(response);

  rfidWrite(BitFramingReg, 0x07);

  bool ok = rfidTransceive(
    &command, 1, response, &length, 7
  );

  rfidWrite(BitFramingReg, 0x00);

  return ok && length == 2;
}

// Supports 4-byte UIDs at cascade level 1 only.
bool readCardUID(uint8_t uid[4]) {
  if (!rfidRequestA()) {
    return false;
  }

  uint8_t command[2] = {PICC_SEL_CL1, 0x20};
  uint8_t response[10] = {0};
  uint8_t length = sizeof(response);

  if (!rfidTransceive(
        command, 2, response, &length)) {
    return false;
  }

  if (length != 5) {
    return false;
  }

  uint8_t bcc = response[0] ^ response[1] ^
                response[2] ^ response[3];

  if (bcc != response[4]) {
    return false;
  }

  memcpy(uid, response, 4);
  return true;
}

String uidToString(const uint8_t uid[4]) {
  char buffer[9];

  snprintf(buffer, sizeof(buffer),
           "%02X%02X%02X%02X",
           uid[0], uid[1], uid[2], uid[3]);

  return String(buffer);
}

// =====================================================
// REGISTERED VOTER DATABASE
// =====================================================

int findCardSlot(const String &uid, String &voterID) {
  for (int slot = 1; slot <= 6; slot++) {
    String uidKey = "uid" + String(slot);
    String vidKey = "vid" + String(slot);

    String savedUID = prefs.getString(
      uidKey.c_str(), ""
    );

    savedUID.toUpperCase();

    if (savedUID.length() && savedUID == uid) {
      voterID = prefs.getString(
        vidKey.c_str(), ""
      );

      return slot;
    }
  }

  return 0;
}

// =====================================================
// FINGERPRINT AUTHENTICATION
// =====================================================

bool checkFingerprint(uint16_t expectedID) {
  unsigned long start = millis();

  while (millis() - start < 20000) {
    if (checkCancel()) {
      return false;
    }

    uint8_t result = finger.getImage();

    if (result == FINGERPRINT_NOFINGER) {
      delay(60);
      continue;
    }

    if (result != FINGERPRINT_OK) {
      delay(100);
      continue;
    }

    if (finger.image2Tz(1) != FINGERPRINT_OK) {
      return false;
    }

    if (checkCancel()) {
      return false;
    }

    if (finger.fingerFastSearch() != FINGERPRINT_OK) {
      return false;
    }

    Serial.printf(
      "Matched fingerprint template: %d\n",
      finger.fingerID
    );

    return finger.fingerID == expectedID;
  }

  Serial.println("Fingerprint timeout.");
  return false;
}

// =====================================================
// FEEDBACK
// =====================================================

void successFeedback() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, HIGH);

  delay(180);
  digitalWrite(BUZZER, LOW);
}

void failureFeedback() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);

  for (int i = 0; i < 2; i++) {
    if (checkCancel()) {
      return;
    }

    digitalWrite(BUZZER, HIGH);
    delay(120);
    digitalWrite(BUZZER, LOW);
    delay(120);
  }
}

// =====================================================
// AUTHENTICATION FLOW
// =====================================================

void authenticateCard(const String &uid) {
  if (busy) {
    return;
  }

  busy = true;
  cancelRequested = false;

  String voterID;
  int slot = findCardSlot(uid, voterID);

  if (slot == 0 || voterID.length() == 0) {
    Serial.println("AUTH:DENY - Card not registered");

    showFailure("CARD NOT REGISTERED");
    failureFeedback();

    if (!cancelRequested) {
      waitWithCancel(1800);
    }

    if (cancelRequested) {
      cancelSession();
    } else {
      resetFeedback();
      showWelcome();
    }

    busy = false;
    return;
  }

  Serial.printf("Registered card slot: %d\n", slot);
  Serial.printf("Voter ID: %s\n", voterID.c_str());

  // Show the voter ID immediately after a registered RFID scan.
  showVoterID(voterID);

  // Keep the ID on screen before asking for the fingerprint.
  if (!waitWithCancel(1800)) {
    cancelSession();
    busy = false;
    return;
  }

  if (!fingerprintReady) {
    showFailure("FINGER SENSOR ERROR");
    failureFeedback();

    if (!cancelRequested) {
      waitWithCancel(1800);
    }

    if (cancelRequested) {
      cancelSession();
    } else {
      resetFeedback();
      showWelcome();
    }

    busy = false;
    return;
  }

  showFingerprintPrompt();

  // The original mapping is slot 1 -> fingerprint 1,
  // slot 2 -> fingerprint 2, and so on.
  bool accepted = checkFingerprint((uint16_t)slot);

  if (cancelRequested) {
    cancelSession();
    busy = false;
    return;
  }

  if (accepted) {
    Serial.println("AUTH:ALLOW");

    showSuccess();

    if (cancelRequested) {
      cancelSession();
      busy = false;
      return;
    }

    successFeedback();
  } else {
    Serial.println("AUTH:DENY");

    showFailure("FINGERPRINT MISMATCH");
    failureFeedback();
  }

  if (!cancelRequested) {
    waitWithCancel(2500);
  }

  if (cancelRequested) {
    cancelSession();
  } else {
    resetFeedback();
    showWelcome();
  }

  busy = false;
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // GPIO34 has no internal pull-up or pull-down.
  pinMode(CANCEL_BUTTON, INPUT);

  resetFeedback();

  // Initialize TFT.
  tftSPI.begin(
    TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS
  );

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(C_BLACK);

  drawHeader();
  drawCentered("INITIALIZING...", 120, C_YELLOW, 2);

  // Read-only mode preserves existing voter records.
  prefs.begin("voterdb", true);

  // Initialize RFID reader.
  rfidInit();

  // Initialize R307 fingerprint sensor.
  fingerSerial.begin(
    FINGER_BAUD,
    SERIAL_8N1,
    FINGER_RX,
    FINGER_TX
  );

  finger.begin(FINGER_BAUD);
  fingerprintReady = finger.verifyPassword();

  Serial.println();
  Serial.println("ESP32 DUAL-AUTHENTICATION EVM");
  Serial.println("Voter database preserved.");

  Serial.printf(
    "RC522: %s\n",
    rfidReady ? "READY" : "NOT DETECTED"
  );

  Serial.printf(
    "R307: %s\n",
    fingerprintReady ? "CONNECTED" : "NOT DETECTED"
  );

  Serial.println("Cancel button: GPIO34");

  showWelcome();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  if (checkCancel()) {
    cancelSession();
    return;
  }

  if (!rfidReady) {
    drawHeader();
    drawCentered("RC522 NOT DETECTED", 120, C_RED, 2);
    drawCentered("CHECK HSPI WIRING", 155, C_YELLOW, 1);

    delay(1000);
    return;
  }

  if (busy) {
    return;
  }

  uint8_t uidBytes[4] = {0};

  if (readCardUID(uidBytes)) {
    String uid = uidToString(uidBytes);

    Serial.print("Scanned RFID UID: ");
    Serial.println(uid);

    authenticateCard(uid);

    // Allow the card to be removed.
    delay(500);
  }

  delay(30);
}
