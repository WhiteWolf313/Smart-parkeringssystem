#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Redis.h>

// =======================================================
// WiFi CONFIGURATION
// =======================================================
const char* ssid = "Vodafone-EBDF";
const char* password = "TLL86QcUHeeD6EcQ";
const char* redisHost = "192.168.0.75";  // Pi Zero 2W
const int redisPort = 6379;

// =======================================================
// REDIS CLIENT
// =======================================================
WiFiClient redisClient;
Redis redis(redisClient);

// =======================================================
// PIN DEFINITIONS (your existing pins)
// =======================================================
#define SS_ENTRY_PIN 5
#define SS_EXIT_PIN 4
#define RST_PIN 27
#define SERVO_ENTRY_PIN 13
#define SERVO_EXIT_PIN 14
#define LCD_ENTRY_ADDR 0x27
#define LCD_EXIT_ADDR 0x26
#define BUZZER_PIN 2
#define TRIG_ENTRY_BEFORE 32
#define ECHO_ENTRY_BEFORE 34
#define TRIG_ENTRY_AFTER 33
#define ECHO_ENTRY_AFTER 35
#define TRIG_EXIT_BEFORE 25
#define ECHO_EXIT_BEFORE 36
#define TRIG_EXIT_AFTER 16
#define ECHO_EXIT_AFTER 39

// =======================================================
// GLOBAL OBJECTS
// =======================================================
MFRC522 rfidEntry(SS_ENTRY_PIN, RST_PIN);
MFRC522 rfidExit(SS_EXIT_PIN, RST_PIN);
Servo servoEntry;
Servo servoExit;
LiquidCrystal_I2C lcdEntry(LCD_ENTRY_ADDR, 16, 2);
LiquidCrystal_I2C lcdExit(LCD_EXIT_ADDR, 16, 2);

int totalSpaces = 10;
int occupiedSpaces = 4;

// =======================================================
// REDIS FUNCTIONS  (electric-sheep-co/Redis v2.6.x API)
// =======================================================

bool connectToRedis() {
  Serial.println("[REDIS] Connecting to Redis server...");
  if (!redisClient.connect(redisHost, redisPort)) {
    Serial.println("[REDIS] TCP connection to Redis failed!");
    return false;
  }
  Serial.println("[REDIS] Connected successfully!");
  return true;
}

// incr is not in this library version — simulate with get+set
void redisIncr(const char* key) {
  String val = redis.get(key);
  int n = (val.length() > 0) ? val.toInt() : 0;
  redis.set(key, String(n + 1).c_str());
}

// Returns 1 if card is active, -1 if not found
int checkCardStatus(String cardUID) {
  String key = "card:" + cardUID;
  String status = redis.hget(key.c_str(), "status");
  if (status.length() > 0) {
    return status.toInt();
  }
  return -1;
}

String getCardOwner(String cardUID) {
  String key = "card:" + cardUID;
  String owner = redis.hget(key.c_str(), "owner");
  if (owner.length() > 0) {
    return owner;
  }
  return "Unknown";
}

void logCardEntry(String cardUID) {
  String timestamp = String(millis());
  String owner = getCardOwner(cardUID);
  String logEntry = cardUID + "|" + owner + "|" + timestamp;

  redis.lpush("parking:entry_log", logEntry.c_str());

  redis.hset("parking:last_entry", "card",  cardUID.c_str());
  redis.hset("parking:last_entry", "owner", owner.c_str());
  redis.hset("parking:last_entry", "time",  timestamp.c_str());

  // incr works on a plain key; use card:<uid>:entry_count
  String countKey = "card:" + cardUID + ":entry_count";
  redisIncr(countKey.c_str());

  Serial.printf("[REDIS] Entry logged: %s (%s)\n", cardUID.c_str(), owner.c_str());
}

void logCardExit(String cardUID) {
  String timestamp = String(millis());
  String owner = getCardOwner(cardUID);
  String logEntry = cardUID + "|" + owner + "|" + timestamp;

  redis.lpush("parking:exit_log", logEntry.c_str());

  redis.hset("parking:last_exit", "card",  cardUID.c_str());
  redis.hset("parking:last_exit", "owner", owner.c_str());
  redis.hset("parking:last_exit", "time",  timestamp.c_str());

  String countKey = "card:" + cardUID + ":exit_count";
  redisIncr(countKey.c_str());

  Serial.printf("[REDIS] Exit logged: %s (%s)\n", cardUID.c_str(), owner.c_str());
}

void updateParkingStatus() {
  int availableSpaces = totalSpaces - occupiedSpaces;

  redis.hset("parking:status", "occupied",    String(occupiedSpaces).c_str());
  redis.hset("parking:status", "available",   String(availableSpaces).c_str());
  redis.hset("parking:status", "total",       String(totalSpaces).c_str());
  redis.hset("parking:status", "last_update", String(millis()).c_str());
  redis.hset("parking:status", "esp32_ip",    WiFi.localIP().toString().c_str());

  Serial.printf("[REDIS] Status updated: %d/%d occupied\n", occupiedSpaces, totalSpaces);
}

void saveOccupiedToRedis() {
  redis.set("parking:occupied_persistent", String(occupiedSpaces).c_str());
}

void restoreFromRedis() {
  String saved = redis.get("parking:occupied_persistent");
  if (saved.length() > 0) {
    int savedOccupied = saved.toInt();
    if (savedOccupied >= 0 && savedOccupied <= totalSpaces) {
      occupiedSpaces = savedOccupied;
      Serial.printf("[REDIS] Restored occupied count: %d\n", occupiedSpaces);
    }
  }
}

// =======================================================
// GET RFID UID
// =======================================================
String getRFIDUID(MFRC522 &rfid) {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) {
      uid += ":";
    }
  }
  uid.toUpperCase();
  return uid;
}

// =======================================================
// WIFI CONNECTION
// =======================================================
void connectToWiFi() {
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Failed to connect!");
  }
}

//from me to keep the connection between esp32 and pi

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("[WiFi] Reconnecting...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Reconnected!");
  } else {
    Serial.println("\n[WiFi] Failed to reconnect!");
  }
}


bool ensureRedis() {
  if (redisClient.connected()) return true;

  Serial.println("[REDIS] Reconnecting...");

  redisClient.stop();  // 🔥 important: clear broken connection

  if (!redisClient.connect(redisHost, redisPort)) {
    Serial.println("[REDIS] Reconnect failed!");
    return false;
  }

  Serial.println("[REDIS] Reconnected!");
  return true;
}
//************************************ */




// =======================================================
// ULTRASONIC AND GATE FUNCTIONS (your existing code)
// =======================================================
long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 30000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

bool isCarPresent(int trig, int echo) {
  long distance = getDistance(trig, echo);
  return (distance < 15);
}

void beepSuccess() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

void beepError() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}

void beepGateOpen() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
}

void openEntryGate() {
  Serial.println("[ENTRY GATE] Opening...");
  beepGateOpen();
  servoEntry.write(90);
}

void closeEntryGate() {
  Serial.println("[ENTRY GATE] Closing...");
  servoEntry.write(0);
}

void openExitGate() {
  Serial.println("[EXIT GATE] Opening...");
  beepGateOpen();
  servoExit.write(90);
}

void closeExitGate() {
  Serial.println("[EXIT GATE] Closing...");
  servoExit.write(0);
}

void updateEntryLCD() {
  int availableSpaces = totalSpaces - occupiedSpaces;
  lcdEntry.clear();
  lcdEntry.setCursor(0, 0);
  lcdEntry.print("Welcome!");
  lcdEntry.setCursor(0, 1);
  lcdEntry.print("Free: ");
  lcdEntry.print(availableSpaces);
  lcdEntry.print("/");
  lcdEntry.print(totalSpaces);
}

void updateExitLCD() {
  int availableSpaces = totalSpaces - occupiedSpaces;
  lcdExit.clear();
  lcdExit.setCursor(0, 0);
  lcdExit.print("Exit Ready");
  lcdExit.setCursor(0, 1);
  lcdExit.print("Free: ");
  lcdExit.print(availableSpaces);
  lcdExit.print("/");
  lcdExit.print(totalSpaces);
}

// =======================================================
// SETUP
// =======================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("PARKING SYSTEM - REDIS V2 API");
  Serial.println("========================================\n");
  
  // Connect to WiFi
  connectToWiFi();
  
  // Connect to Redis and restore state
  if (WiFi.status() == WL_CONNECTED) {
    if (connectToRedis()) {
      restoreFromRedis();
      updateParkingStatus();
      saveOccupiedToRedis();
    }
  }
  
  // Initialize peripherals
  SPI.begin();
  rfidEntry.PCD_Init();
  rfidExit.PCD_Init();
  servoEntry.attach(SERVO_ENTRY_PIN);
  servoExit.attach(SERVO_EXIT_PIN);
  servoEntry.write(0);
  servoExit.write(0);
  lcdEntry.init();
  lcdEntry.backlight();
  lcdExit.init();
  lcdExit.backlight();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  pinMode(TRIG_ENTRY_BEFORE, OUTPUT); pinMode(ECHO_ENTRY_BEFORE, INPUT);
  pinMode(TRIG_ENTRY_AFTER, OUTPUT);  pinMode(ECHO_ENTRY_AFTER, INPUT);
  pinMode(TRIG_EXIT_BEFORE, OUTPUT);  pinMode(ECHO_EXIT_BEFORE, INPUT);
  pinMode(TRIG_EXIT_AFTER, OUTPUT);   pinMode(ECHO_EXIT_AFTER, INPUT);
  
  updateEntryLCD();
  updateExitLCD();
  
  Serial.println("\n[SYSTEM] READY!");
  Serial.println("========================================\n");
}

// =======================================================
// MAIN LOOP
// =======================================================
void loop() {
  ensureWiFi();

  static unsigned long lastRedisUpdate = 0;

  if (millis() - lastRedisUpdate > 10000) {

    if (ensureRedis()) {
      updateParkingStatus();
      lastRedisUpdate = millis();
    } else {
      Serial.println("[REDIS] Skipping update, not connected.");
    }
  }




  // ========== CHECK FOR CAR AT ENTRY (BEFORE SENSOR) ==========
  bool carAtEntryBefore = isCarPresent(TRIG_ENTRY_BEFORE, ECHO_ENTRY_BEFORE);
  
  if (carAtEntryBefore) {
    // Show scan card message on entry LCD
    lcdEntry.clear();
    lcdEntry.setCursor(0, 0);
    lcdEntry.print("Welcome!");
    lcdEntry.setCursor(0, 1);
    lcdEntry.print("Scan Your Card");
    // NO BEEP HERE - Only RFID triggers beep
  } else {
    // Show normal entry display
    updateEntryLCD();
  }





  
  // ========== ENTRY RFID CHECK ==========
  if (rfidEntry.PICC_IsNewCardPresent() && rfidEntry.PICC_ReadCardSerial()) {
    String cardUID = getRFIDUID(rfidEntry);
    Serial.println("\n>>> [ENTRY] Card detected: " + cardUID);
    
    int cardStatus = checkCardStatus(cardUID);
    
    if (cardStatus == 1 && occupiedSpaces < totalSpaces) {
      String owner = getCardOwner(cardUID);
      
      lcdEntry.clear();
      lcdEntry.setCursor(0, 0);
      lcdEntry.print("Welcome " + owner);
      lcdEntry.setCursor(0, 1);
      lcdEntry.print("Gate Opening...");
      
      beepSuccess();
      logCardEntry(cardUID);
      openEntryGate();
      
      delay(3000);
      
      occupiedSpaces++;
      saveOccupiedToRedis();
      updateParkingStatus();
      
      closeEntryGate();
      updateEntryLCD();
      updateExitLCD();
      
      lcdEntry.clear();
      lcdEntry.setCursor(0, 0);
      lcdEntry.print("Welcome " + owner + "!");
      lcdEntry.setCursor(0, 1);
      lcdEntry.print("Drive Safely!");
      delay(2000);
      updateEntryLCD();
      
    } else {
      lcdEntry.clear();
      lcdEntry.setCursor(0, 0);
      lcdEntry.print("INVALID CARD");
      lcdEntry.setCursor(0, 1);
      lcdEntry.print("Access Denied!");
      beepError();
      delay(2000);
      updateEntryLCD();
    }
    
    rfidEntry.PICC_HaltA();
    delay(500);
  }
  
  // ========== EXIT RFID CHECK ==========
  if (rfidExit.PICC_IsNewCardPresent() && rfidExit.PICC_ReadCardSerial()) {
    String cardUID = getRFIDUID(rfidExit);
    Serial.println("\n>>> [EXIT] Card detected: " + cardUID);
    
    int cardStatus = checkCardStatus(cardUID);
    
    if (cardStatus == 1 && occupiedSpaces > 0) {
      String owner = getCardOwner(cardUID);
      
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("Goodbye " + owner);
      lcdExit.setCursor(0, 1);
      lcdExit.print("Gate Opening...");
      
      beepSuccess();
      logCardExit(cardUID);
      openExitGate();
      
      delay(3000);
      
      occupiedSpaces--;
      saveOccupiedToRedis();
      updateParkingStatus();
      
      closeExitGate();
      updateEntryLCD();
      updateExitLCD();
      
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("Goodbye " + owner + "!");
      lcdExit.setCursor(0, 1);
      lcdExit.print("Drive Safely!");
      delay(2000);
      updateExitLCD();
      
    } else {
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("INVALID CARD");
      lcdExit.setCursor(0, 1);
      lcdExit.print("Access Denied!");
      beepError();
      delay(2000);
      updateExitLCD();
    }
    
    rfidExit.PICC_HaltA();
    delay(500);
  }
  
  delay(100);
}
