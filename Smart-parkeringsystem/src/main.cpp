/*
 * SMART PARKING SYSTEM - PROFESSIONELL VERSION (SAMMANSLAGEN)
 * Hårdvara: ESP32 DevKitC V4, 2x RFID, 2x Servo, 2x LCD, 4x HC-SR04, 1x Buzzer
 * Databas: Redis via Wi-Fi (med LittleFS Offline-stöd)
 */

#include <Arduino.h>
#include "esp_task_wdt.h"
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Redis.h>

// =======================================================
// 1. PIN-DEFINITIONER (Standardiserade)
// =======================================================
#define BUZZER_PIN 2

#define RST_IN_PIN 27
#define RST_OUT_PIN 27 // Tillagd för den andra RFID-läsaren
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

// =======================================================
// 2. KLASSER & OBJEKT (Sensorklass för Khalids kod)
// =======================================================
class UltrasoundSensor {
private:
    uint8_t trigPin;
    uint8_t echoPin;

public:
    static constexpr float kInvalidDistanceCm = -1.0f;

    UltrasoundSensor(uint8_t trig, uint8_t echo) : trigPin(trig), echoPin(echo) {}

    void begin() {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
    }

    float readDistanceCm(uint8_t samples = 1) {
        float totalDistance = 0;
        int validSamples = 0;

        for (uint8_t i = 0; i < samples; i++) {
            digitalWrite(trigPin, LOW);
            delayMicroseconds(2);
            digitalWrite(trigPin, HIGH);
            delayMicroseconds(10);
            digitalWrite(trigPin, LOW);

            long duration = pulseIn(echoPin, HIGH, 30000);
            if (duration > 0) {
                totalDistance += (duration * 0.034f / 2.0f);
                validSamples++;
            }
            delay(5); // Kort paus mellan mätningar
        }

        if (validSamples == 0) return kInvalidDistanceCm;
        return totalDistance / validSamples;
    }
};

// =======================================================
// 3. INSTÄLLNINGAR, VARIABLER & OBJEKT
// =======================================================
// Yousifs inställningar (WiFi & Redis)
const char* ssid = "DITT_WIFI_NAMN";
const char* password = "DITT_WIFI_LOSENORD";
const char* redis_host = "192.168.1.100";
const int redis_port = 6379;
const char* queueFile = "/queue.txt";
WiFiClient redisClient;

// Mohameds inställningar (Skärmar & Platser)
LiquidCrystal_I2C lcdIn(0x27, 16, 2);
int totalSpaces = 10;
int occupiedSpaces = 4;

// Stars inställningar (RFID)
#define DEBOUNCE_MS 1500
MFRC522 readerIn(SS_IN_PIN, RST_IN_PIN);
MFRC522 readerOut(SS_OUT_PIN, RST_OUT_PIN);
String lastUidIn, lastUidOut;
uint32_t lastSeenIn = 0, lastSeenOut = 0;

// Khalids inställningar (Sensorer)
constexpr unsigned long kPrintIntervalMs = 300UL;
constexpr float kCarPresentThresholdCm = 15.0f;
constexpr uint8_t kSamplesPerReading = 3;
UltrasoundSensor inBeforeSensor(TRIG_IN_BEFORE, ECHO_IN_BEFORE);
UltrasoundSensor inAfterSensor(TRIG_IN_AFTER, ECHO_IN_AFTER);
UltrasoundSensor outBeforeSensor(TRIG_OUT_BEFORE, ECHO_OUT_BEFORE);
UltrasoundSensor outAfterSensor(TRIG_OUT_AFTER, ECHO_OUT_AFTER);
unsigned long lastPrintAtMs = 0;
bool carAtEntry = false;
bool gateInOpen = false;
bool carWasUnderInAfter = false;
unsigned long gateOpenedAtMs = 0;

// Atoshs inställningar (Servon)
Servo gateIn;
Servo gateOut;

// =======================================================
// 4. HJÄLPFUNKTIONER
// =======================================================

// --- Skärmar ---
void refreshDisplays() {
  int available = totalSpaces - occupiedSpaces;
  lcdIn.clear();
  if (available <= 0) {
    lcdIn.setCursor(0, 0); lcdIn.print("Parkeringen");
    lcdIn.setCursor(0, 1); lcdIn.print("ar full!");
  } else {
    lcdIn.setCursor(0, 0); lcdIn.print("Welcome!");
    lcdIn.setCursor(0, 1); lcdIn.print("Spaces: "); lcdIn.print(available);
  }

}

// --- WiFi & Databas ---
void connectWiFi() {
  Serial.print("Ansluter till Wi-Fi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500); Serial.print("."); attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println(" OFFLINE! Fortsätter utan internet.");
  }
}

// --- Ljud & Buzzer (Från Desmond) ---
// Pleasant single beep (accepted / entry)
void beepOnce(int durationMs = 120) {
  tone(BUZZER_PIN, 1800, durationMs);
  delay(durationMs + 10);
}

// Two rising beeps (success / gate open)
void beepTwice() {
  tone(BUZZER_PIN, 1200, 100); delay(150);
  tone(BUZZER_PIN, 1800, 100); delay(150);
}

// --- Grindar (Från Atosh) ---
void servoInit() {
    gateIn.attach(SERVO_IN_PIN, 500, 2500);
    gateOut.attach(SERVO_OUT_PIN, 500, 2500);
    gateIn.writeMicroseconds(500);
    gateOut.writeMicroseconds(500);
}
void openEntryGate() {
    for (int us = 500; us <= 1500; us += 15) {
        gateIn.writeMicroseconds(us);
        delay(15);
    }
    Serial.println("Entry gate opened");
}
void closeEntryGate() {
    gateIn.attach(SERVO_IN_PIN, 500, 2500);
    for (int us = 1500; us >= 500; us -= 15) {
        gateIn.writeMicroseconds(us);
        delay(15);
    }
    gateIn.writeMicroseconds(500);
    Serial.println("Entry gate closed");
}
void openExitGate() { gateOut.write(90); Serial.println("Exit gate opened"); beepOnce(250); }
void closeExitGate() { gateOut.write(0); Serial.println("Exit gate closed"); }

// --- RFID (Från Star) ---
String uidToHex(MFRC522::Uid &uid) {
    String s;
    for (byte i = 0; i < uid.size; i++) {
        if (uid.uidByte[i] < 0x10) s += '0';
        s += String(uid.uidByte[i], HEX);
    }
    s.toUpperCase(); return s;
}

bool pollReader(MFRC522 &mfrc, String &lastUid, uint32_t &lastSeen, String &outUid) {
    if (!mfrc.PICC_IsNewCardPresent() || !mfrc.PICC_ReadCardSerial()) return false;
    String uid = uidToHex(mfrc.uid);
    mfrc.PICC_HaltA(); mfrc.PCD_StopCrypto1();

    uint32_t now = millis();
    if (uid == lastUid && (now - lastSeen) < DEBOUNCE_MS) {
        lastSeen = now; return false;
    }
    lastUid = uid; lastSeen = now; outUid = uid;
    return true;
}

// --- Sensorer (Från Khalid) ---
void printSensorState(const char* name, UltrasoundSensor& sensor) {
    float distance = sensor.readDistanceCm(kSamplesPerReading);
    bool present = (distance != UltrasoundSensor::kInvalidDistanceCm && distance <= kCarPresentThresholdCm);
    Serial.printf("%s: ", name);
    if (distance == UltrasoundSensor::kInvalidDistanceCm) Serial.print("no reading");
    else Serial.printf("%.1f cm", distance);
    Serial.printf(" | car=%s\n", present ? "yes" : "no");
}

// =======================================================
// 5. HUVUDPROGRAM (SETUP & LOOP)
// =======================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n[BOOT] Smart Parking System");

    // Initiera I2C och SPI
    Wire.begin(); 
    SPI.begin(); // Använder standard SPI-pins

    // Initiera Skärmar
    lcdIn.init(); lcdIn.backlight();
    refreshDisplays();

    // Initiera Servon och Buzzer
    servoInit();
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // Servo boot test: open then close
    // Initiera RFID
    readerIn.PCD_Init();
    readerOut.PCD_Init();
    Serial.println("[RFID] ready");

    // Initiera Sensorer
    inBeforeSensor.begin();
    inAfterSensor.begin();
    outBeforeSensor.begin();
    outAfterSensor.begin();
    Serial.println("[ULTRASOUND] ready");

    // Initiera Nätverk (LittleFS och WiFi)
    if (!LittleFS.begin(true)) {
      Serial.println("Kritiskt fel: Kunde inte starta LittleFS!");
    }
    connectWiFi();
}

void loop() {
    String uid;
    unsigned long now = millis();

    // 1. Kontrollera inkommande bilar (RFID)
if (pollReader(readerIn, lastUidIn, lastSeenIn, uid)) {
    Serial.printf("[CARD IN]  UID=%s\n", uid.c_str());

    if (occupiedSpaces >= totalSpaces) {
        Serial.println("Parkeringen är full!");
        beepOnce(500);            // lång pip = nekad
    } else {
        beepTwice();
        esp_task_wdt_reset();
        delay(2000);
        openEntryGate();
        gateInOpen = true;
        gateOpenedAtMs = millis();
        carWasUnderInAfter = false;
        occupiedSpaces++;
        carAtEntry = false;
        refreshDisplays();
    }
    }

    // 2. Kontrollera utgående bilar (RFID)
    if (pollReader(readerOut, lastUidOut, lastSeenOut, uid)) {
        Serial.printf("[CARD OUT] UID=%s\n", uid.c_str());
        beepTwice();
        openExitGate();

        if (occupiedSpaces > 0) {
            occupiedSpaces--;
        }
        Serial.printf("[EXIT] occupied=%d free=%d\n", occupiedSpaces, totalSpaces - occupiedSpaces);
        refreshDisplays();
        // Här kan du lägga till logik för att stänga grinden när bilen passerat (med hjälp av outAfterSensor)
    }

    // 3. Kontrollera sensor vid ingången och uppdatera LCD
    if (now - lastPrintAtMs >= kPrintIntervalMs) {
        lastPrintAtMs = now;

        float distInBefore = inBeforeSensor.readDistanceCm(kSamplesPerReading);
        bool carDetected = (distInBefore != UltrasoundSensor::kInvalidDistanceCm && distInBefore <= kCarPresentThresholdCm);

        if (carDetected && !carAtEntry) {
            carAtEntry = true;
            lcdIn.clear();
            if (occupiedSpaces >= totalSpaces) {
                lcdIn.setCursor(0, 0); lcdIn.print("Parkeringen");
                lcdIn.setCursor(0, 1); lcdIn.print("ar full!");
            } else {
                lcdIn.setCursor(0, 0); lcdIn.print("Welcome!");
                lcdIn.setCursor(0, 1); lcdIn.print("Scan your card");
            }
        } else if (!carDetected && carAtEntry) {
            carAtEntry = false;
            refreshDisplays();
        }

        if (gateInOpen && (millis() - gateOpenedAtMs > 3000)) {
            float distAfter = inAfterSensor.readDistanceCm(kSamplesPerReading);
            bool carUnderSensor = (distAfter != UltrasoundSensor::kInvalidDistanceCm && distAfter <= kCarPresentThresholdCm);
            Serial.printf("[AFTER] dist=%.1f car=%s wasUnder=%s\n", distAfter, carUnderSensor?"yes":"no", carWasUnderInAfter?"yes":"no");
            if (carUnderSensor) carWasUnderInAfter = true;
            if (carWasUnderInAfter && !carUnderSensor) {
                closeEntryGate();
                gateInOpen = false;
                carWasUnderInAfter = false;
            }
        }
    }
    


        // 3. Debug-utskrift av sensorer (körs bara ibland för att inte spamma)
    if (now - lastPrintAtMs >= kPrintIntervalMs) {
        lastPrintAtMs = now;
        /* Avkommentera nedan för att se live-data från ultraljudssensorerna i konsolen:*/
        printSensorState("IN_BEFORE", inBeforeSensor);
        printSensorState("IN_AFTER", inAfterSensor);
        printSensorState("OUT_BEFORE", outBeforeSensor);
        printSensorState("OUT_AFTER", outAfterSensor);
        Serial.println("---");
        
    }
}