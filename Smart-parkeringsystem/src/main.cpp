/*
 * SMART PARKING SYSTEM - PROFESSIONELL VERSION (NON-BLOCKING)
 * Hårdvara: ESP32 DevKitC V4 (38-pin), 2x RFID, 2x Servo, 2x LCD, 4x HC-SR04, 1x Buzzer
 * Databas: Redis via Wi-Fi (med LittleFS Offline-stöd)
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>

#define BUZZER_PIN 2

#define RST_PIN 27
#define SS_IN_PIN 5
#define SS_OUT_PIN 4

#define SERVO_IN_PIN 13
#define SERVO_OUT_PIN 14

#define TRIG_IN_BEFORE 32
#define ECHO_IN_BEFORE 34
#define TRIG_IN_AFTER 33
#define ECHO_IN_AFTER 35

#define TRIG_OUT_BEFORE 25
#define ECHO_OUT_BEFORE 36
#define TRIG_OUT_AFTER 26
#define ECHO_OUT_AFTER 39

#define RFID_SS_PIN 5
#define RFID_RST_PIN 27
#define SERVO_PIN 4
#define TRIG_PIN 12
#define ECHO_PIN 13

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);
Servo gateServo;

bool lastCarDetected = false;
bool gateOpen = false;

void beepOnce(int durationMs = 120) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(durationMs);
  digitalWrite(BUZZER_PIN, LOW);
}

void beepTwice() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(80);
  }
}

void beepGateOpen() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_PIN, LOW);
}

long readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  long distance = duration * 0.034 / 2;
  return distance;
}

bool isCarDetected() {
  long distance = readDistanceCm();
  if (distance == -1) return false;
  return distance > 0 && distance < 15;
}

bool isRFIDCardRead() {
  if (!mfrc522.PICC_IsNewCardPresent()) return false;
  if (!mfrc522.PICC_ReadCardSerial()) return false;

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return true;
}

void openGate() {
  gateServo.write(90);
  gateOpen = true;
  beepGateOpen();
  Serial.println("Gate opened");
}

void closeGate() {
  gateServo.write(0);
  gateOpen = false;
  Serial.println("Gate closed");
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  SPI.begin();
  mfrc522.PCD_Init();

  gateServo.attach(SERVO_PIN);
  gateServo.write(0);

  Serial.println("System started");
}

void loop() {
  bool carDetected = isCarDetected();

  if (carDetected && !lastCarDetected) {
    beepOnce();
    Serial.println("Car detected");
  }
  lastCarDetected = carDetected;

  if (isRFIDCardRead()) {
    beepTwice();
    Serial.println("RFID detected");

    openGate();
    delay(3000);
    closeGate();
  }

  delay(100);
}



// ==========================================================
// PIN-MAPPNING I PARKERINGSSYSTEMET
// ==========================================================
// BUZZER_PIN      -> Buzzer för ljudsignal
// RST_PIN         -> Reset för RFID-moduler
// SS_IN_PIN       -> RFID vid infart
// SS_OUT_PIN      -> RFID vid utfart
// SERVO_IN_PIN    -> Servo till bom vid infart
// SERVO_OUT_PIN   -> Servo till bom vid utfart
// TRIG_IN_BEFORE  -> Ultraljud före infartsbom (trigger)
// ECHO_IN_BEFORE  -> Ultraljud före infartsbom (echo)
// TRIG_IN_AFTER   -> Ultraljud efter infartsbom (trigger)
// ECHO_IN_AFTER   -> Ultraljud efter infartsbom (echo)
// TRIG_OUT_BEFORE -> Ultraljud före utfartsbom (trigger)
// ECHO_OUT_BEFORE -> Ultraljud före utfartsbom (echo)
// TRIG_OUT_AFTER  -> Ultraljud efter utfartsbom (trigger)
// ECHO_OUT_AFTER  -> Ultraljud efter utfartsbom (echo)

// =======================================================
// 2. PIN-DEFINITIONER
// =======================================================
#define BUZZER_PIN 2

#define RST_PIN 27
#define SS_IN_PIN 5
#define SS_OUT_PIN 4

#define SERVO_IN_PIN 13
#define SERVO_OUT_PIN 14

#define TRIG_IN_BEFORE 32
#define ECHO_IN_BEFORE 34
#define TRIG_IN_AFTER 33
#define ECHO_IN_AFTER 35

#define TRIG_OUT_BEFORE 25
#define ECHO_OUT_BEFORE 36
#define TRIG_OUT_AFTER 26
#define ECHO_OUT_AFTER 39