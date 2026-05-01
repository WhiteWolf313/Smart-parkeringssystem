#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =======================================================
// PIN DEFINITIONS
// =======================================================

// -------- RFID (Same RST pin as your working code) --------
#define SS_ENTRY_PIN 5
#define SS_EXIT_PIN 4
#define RST_PIN 27  // Same RST for both readers

// -------- Servo --------
#define SERVO_ENTRY_PIN 13
#define SERVO_EXIT_PIN 14

// -------- LCD --------
#define LCD_ENTRY_ADDR 0x27
#define LCD_EXIT_ADDR 0x26

// -------- Buzzer --------
#define BUZZER_PIN 2

// -------- Ultrasonic Sensors for Car Detection --------
// Entry sensors (before and after gate)
#define TRIG_ENTRY_BEFORE 32
#define ECHO_ENTRY_BEFORE 34
#define TRIG_ENTRY_AFTER 33
#define ECHO_ENTRY_AFTER 35

// Exit sensors (before and after gate)
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

// =======================================================
// PARKING SLOTS
// =======================================================
int totalSpaces = 10;
int occupiedSpaces = 4;

// =======================================================
// FUNCTION: Get distance from ultrasonic sensor
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

// =======================================================
// FUNCTION: Check if car is present (within 15cm)
// =======================================================
bool isCarPresent(int trig, int echo) {
  long distance = getDistance(trig, echo);
  return (distance < 15);
}

// =======================================================
// FUNCTION: Beep patterns (ONLY for RFID)
// =======================================================
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

// =======================================================
// FUNCTION: Open/Close Entry Gate
// =======================================================
void openEntryGate() {
  Serial.println("[ENTRY GATE] Opening...");
  beepGateOpen();
  servoEntry.write(90);
}

void closeEntryGate() {
  Serial.println("[ENTRY GATE] Closing...");
  servoEntry.write(0);
}

// =======================================================
// FUNCTION: Open/Close Exit Gate  
// =======================================================
void openExitGate() {
  Serial.println("[EXIT GATE] Opening...");
  beepGateOpen();
  servoExit.write(90);
}

void closeExitGate() {
  Serial.println("[EXIT GATE] Closing...");
  servoExit.write(0);
}

// =======================================================
// FUNCTION: Update LCD displays
// =======================================================
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
  Serial.println("PARKING SYSTEM - CORRECTED VERSION");
  Serial.println("========================================\n");
  
  // Initialize SPI
  SPI.begin();
  
  // Initialize RFID readers
  rfidEntry.PCD_Init();
  rfidExit.PCD_Init();
  Serial.println("[RFID] Both readers initialized");
  
  // Initialize servos
  servoEntry.attach(SERVO_ENTRY_PIN);
  servoExit.attach(SERVO_EXIT_PIN);
  servoEntry.write(0);
  servoExit.write(0);
  Serial.println("[SERVO] Servos initialized");
  
  // Initialize LCDs
  lcdEntry.init();
  lcdEntry.backlight();
  lcdExit.init();
  lcdExit.backlight();
  Serial.println("[LCD] Displays initialized");
  
  // Initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize ultrasonic sensors
  pinMode(TRIG_ENTRY_BEFORE, OUTPUT); pinMode(ECHO_ENTRY_BEFORE, INPUT);
  pinMode(TRIG_ENTRY_AFTER, OUTPUT);  pinMode(ECHO_ENTRY_AFTER, INPUT);
  pinMode(TRIG_EXIT_BEFORE, OUTPUT);  pinMode(ECHO_EXIT_BEFORE, INPUT);
  pinMode(TRIG_EXIT_AFTER, OUTPUT);   pinMode(ECHO_EXIT_AFTER, INPUT);
  Serial.println("[ULTRASONIC] Car detection sensors initialized");
  
  // Initial display
  updateEntryLCD();
  updateExitLCD();
  
  Serial.println("\n[SYSTEM] READY!");
  Serial.println("Entry RFID -> Opens ENTRY gate");
  Serial.println("Exit RFID -> Opens EXIT gate");
  Serial.println("========================================\n");
}

// =======================================================
// MAIN LOOP
// =======================================================
void loop() {
  // Update available spaces display
  int availableSpaces = totalSpaces - occupiedSpaces;
  
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
  
  // ========== CHECK FOR CAR AT EXIT (BEFORE SENSOR) ==========
  bool carAtExitBefore = isCarPresent(TRIG_EXIT_BEFORE, ECHO_EXIT_BEFORE);
  
  if (carAtExitBefore) {
    // Show scan card message on exit LCD
    lcdExit.clear();
    lcdExit.setCursor(0, 0);
    lcdExit.print("Goodbye!");
    lcdExit.setCursor(0, 1);
    lcdExit.print("Scan Your Card");
    // NO BEEP HERE - Only RFID triggers beep
  } else {
    // Show normal exit display (Exit Ready)
    updateExitLCD();
  }
  
  // ========== ENTRY RFID CARD DETECTED ==========
  if (rfidEntry.PICC_IsNewCardPresent() && rfidEntry.PICC_ReadCardSerial()) {
    Serial.println("\n>>> [ENTRY RFID] Card detected <<<");
    
    if (occupiedSpaces < totalSpaces) {
      // Success - Open entry gate
      lcdEntry.clear();
      lcdEntry.setCursor(0, 0);
      lcdEntry.print("Access Granted");
      lcdEntry.setCursor(0, 1);
      lcdEntry.print("Gate Opening...");
      
      beepSuccess();  // Success beep for valid card
      openEntryGate();
      
      // Wait for car to enter (using AFTER sensor)
      unsigned long gateOpenTime = millis();
      bool carEntered = false;
      
      lcdEntry.clear();
      lcdEntry.setCursor(0, 0);
      lcdEntry.print("Gate Open");
      lcdEntry.setCursor(0, 1);
      lcdEntry.print("Please Enter");
      
      while (millis() - gateOpenTime < 10000) {
        if (isCarPresent(TRIG_ENTRY_AFTER, ECHO_ENTRY_AFTER)) {
          carEntered = true;
          Serial.println("[ENTRY] Car detected at AFTER sensor");
          lcdEntry.clear();
          lcdEntry.setCursor(0, 0);
          lcdEntry.print("Car Detected");
          lcdEntry.setCursor(0, 1);
          lcdEntry.print("Please Proceed");
          break;
        }
        delay(50);
      }
      
      if (carEntered) {
        // Wait for car to clear the gate
        while (isCarPresent(TRIG_ENTRY_AFTER, ECHO_ENTRY_AFTER)) {
          delay(50);
        }
        occupiedSpaces++;
        Serial.printf("[ENTRY] Car entered. Occupied: %d/%d\n", occupiedSpaces, totalSpaces);
      }
      
      closeEntryGate();
      updateEntryLCD();
      updateExitLCD();
      
      // Show success message
      lcdEntry.clear();
      lcdEntry.setCursor(0, 0);
      lcdEntry.print("Welcome!");
      lcdEntry.setCursor(0, 1);
      lcdEntry.print("Drive Safely!");
      delay(2000);
      updateEntryLCD();
      
    } else {
      // Parking full
      lcdEntry.clear();
      lcdEntry.setCursor(0, 0);
      lcdEntry.print("PARKING FULL");
      lcdEntry.setCursor(0, 1);
      lcdEntry.print("Access Denied");
      
      beepError();  // Error beep for denied access
      delay(2000);
      updateEntryLCD();
    }
    
    rfidEntry.PICC_HaltA();
    delay(500);
  }
  
  // ========== EXIT RFID CARD DETECTED ==========
  if (rfidExit.PICC_IsNewCardPresent() && rfidExit.PICC_ReadCardSerial()) {
    Serial.println("\n>>> [EXIT RFID] Card detected <<<");
    
    if (occupiedSpaces > 0) {
      // Success - Open exit gate
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("Access Granted");
      lcdExit.setCursor(0, 1);
      lcdExit.print("Gate Opening...");
      
      beepSuccess();  // Success beep for valid card
      openExitGate();
      
      // Wait for car to exit (using AFTER sensor)
      unsigned long gateOpenTime = millis();
      bool carExited = false;
      
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("Gate Open");
      lcdExit.setCursor(0, 1);
      lcdExit.print("Please Exit");
      
      while (millis() - gateOpenTime < 10000) {
        if (isCarPresent(TRIG_EXIT_AFTER, ECHO_EXIT_AFTER)) {
          carExited = true;
          Serial.println("[EXIT] Car detected at AFTER sensor");
          lcdExit.clear();
          lcdExit.setCursor(0, 0);
          lcdExit.print("Car Detected");
          lcdExit.setCursor(0, 1);
          lcdExit.print("Please Proceed");
          break;
        }
        delay(50);
      }
      
      if (carExited) {
        // Wait for car to clear the gate
        while (isCarPresent(TRIG_EXIT_AFTER, ECHO_EXIT_AFTER)) {
          delay(50);
        }
        occupiedSpaces--;
        Serial.printf("[EXIT] Car exited. Occupied: %d/%d\n", occupiedSpaces, totalSpaces);
      }
      
      closeExitGate();
      updateEntryLCD();
      updateExitLCD();
      
      // Show goodbye message
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("Goodbye!");
      lcdExit.setCursor(0, 1);
      lcdExit.print("Drive Safely!");
      delay(2000);
      updateExitLCD();
      
    } else {
      // No cars to exit
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("No Cars");
      lcdExit.setCursor(0, 1);
      lcdExit.print("to Exit!");
      
      beepError();  // Error beep for invalid exit
      delay(2000);
      updateExitLCD();
    }
    
    rfidExit.PICC_HaltA();
    delay(500);
  }
  
  // Debug output every 3 seconds
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 3000) {
    lastDebug = millis();
    Serial.printf("\n[DEBUG] Occupied: %d/%d\n", occupiedSpaces, totalSpaces);
    Serial.printf("  Entry Before: %s, After: %s\n", 
      isCarPresent(TRIG_ENTRY_BEFORE, ECHO_ENTRY_BEFORE) ? "CAR" : "CLEAR",
      isCarPresent(TRIG_ENTRY_AFTER, ECHO_ENTRY_AFTER) ? "CAR" : "CLEAR");
    Serial.printf("  Exit Before: %s, After: %s\n", 
      isCarPresent(TRIG_EXIT_BEFORE, ECHO_EXIT_BEFORE) ? "CAR" : "CLEAR",
      isCarPresent(TRIG_EXIT_AFTER, ECHO_EXIT_AFTER) ? "CAR" : "CLEAR");
    Serial.println("----------------------------------------\n");
  }
  
  delay(100);
}



/*

#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =======================================================
// PIN DEFINITIONS
// =======================================================

// -------- RFID (Same RST pin as your working code) --------
#define SS_ENTRY_PIN 5
#define SS_EXIT_PIN 4
#define RST_PIN 27  // Same RST for both readers

// -------- Servo --------
#define SERVO_ENTRY_PIN 13
#define SERVO_EXIT_PIN 14

// -------- LCD --------
#define LCD_ENTRY_ADDR 0x27
#define LCD_EXIT_ADDR 0x26

// -------- Buzzer --------
#define BUZZER_PIN 2

// -------- Ultrasonic Sensors for Car Detection --------
// Entry sensors (before and after gate)
#define TRIG_ENTRY_BEFORE 32
#define ECHO_ENTRY_BEFORE 34
#define TRIG_ENTRY_AFTER 33
#define ECHO_ENTRY_AFTER 35

// Exit sensors (before and after gate)
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

// =======================================================
// PARKING SLOTS (Fixed number, no ultrasonic for slots)
// =======================================================
int totalSpaces = 10;
int occupiedSpaces = 4;

// =======================================================
// FUNCTION: Get distance from ultrasonic sensor
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

// =======================================================
// FUNCTION: Check if car is present (within 15cm)
// =======================================================
bool isCarPresent(int trig, int echo) {
  long distance = getDistance(trig, echo);
  return (distance < 15);
}

// =======================================================
// FUNCTION: Beep patterns
// =======================================================
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


// =======================================================
// FUNCTION: Open/Close Entry Gate
// =======================================================
void openEntryGate() {
  Serial.println("[ENTRY GATE] Opening...");
  servoEntry.write(90);
}

void closeEntryGate() {
  Serial.println("[ENTRY GATE] Closing...");
  servoEntry.write(0);
}

// =======================================================
// FUNCTION: Open/Close Exit Gate  
// =======================================================
void openExitGate() {
  Serial.println("[EXIT GATE] Opening...");
  servoExit.write(90);
}

void closeExitGate() {
  Serial.println("[EXIT GATE] Closing...");
  servoExit.write(0);
}

// =======================================================
// FUNCTION: Update LCD displays
// =======================================================
void updateDisplays() {
  int availableSpaces = totalSpaces - occupiedSpaces;
  
  // Entry LCD
  lcdEntry.clear();
  lcdEntry.setCursor(0, 0);
  lcdEntry.print("Welcome!");
  lcdEntry.setCursor(0, 1);
  lcdEntry.print("Free: ");
  lcdEntry.print(availableSpaces);
  lcdEntry.print("/");
  lcdEntry.print(totalSpaces);
  
  // Exit LCD
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
  Serial.println("PARKING SYSTEM - CAR DETECTION VERSION");
  Serial.println("========================================\n");
  
  // Initialize SPI
  SPI.begin();
  
  // Initialize RFID readers
  rfidEntry.PCD_Init();
  rfidExit.PCD_Init();
  Serial.println("[RFID] Both readers initialized");
  
  // Initialize servos
  servoEntry.attach(SERVO_ENTRY_PIN);
  servoExit.attach(SERVO_EXIT_PIN);
  servoEntry.write(0);
  servoExit.write(0);
  Serial.println("[SERVO] Servos initialized");
  
  // Initialize LCDs
  lcdEntry.init();
  lcdEntry.backlight();
  lcdExit.init();
  lcdExit.backlight();
  Serial.println("[LCD] Displays initialized");
  
  // Initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize ultrasonic sensors
  pinMode(TRIG_ENTRY_BEFORE, OUTPUT); pinMode(ECHO_ENTRY_BEFORE, INPUT);
  pinMode(TRIG_ENTRY_AFTER, OUTPUT);  pinMode(ECHO_ENTRY_AFTER, INPUT);
  pinMode(TRIG_EXIT_BEFORE, OUTPUT);  pinMode(ECHO_EXIT_BEFORE, INPUT);
  pinMode(TRIG_EXIT_AFTER, OUTPUT);   pinMode(ECHO_EXIT_AFTER, INPUT);
  Serial.println("[ULTRASONIC] Car detection sensors initialized");
  
  // Initial display
  updateDisplays();
  
  Serial.println("\n[SYSTEM] READY!");
  Serial.println("Entry RFID -> Opens ENTRY gate");
  Serial.println("Exit RFID -> Opens EXIT gate");
  Serial.println("========================================\n");
}

// =======================================================
// MAIN LOOP
// =======================================================
void loop() {
  // ========== ENTRY RFID CARD DETECTED ==========
  if (rfidEntry.PICC_IsNewCardPresent() && rfidEntry.PICC_ReadCardSerial()) {
    Serial.println("\n>>> [ENTRY RFID] Card detected <<<");
    
    if (occupiedSpaces < totalSpaces) {
      // Success - Open entry gate
      lcdEntry.clear();
      lcdEntry.setCursor(0, 0);
      lcdEntry.print("Access Granted");
      lcdEntry.setCursor(0, 1);
      lcdEntry.print("Gate Opening...");
      
      beepSuccess();
      openEntryGate();
      
      // Wait for car to enter (using AFTER sensor)
      unsigned long gateOpenTime = millis();
      bool carEntered = false;
      
      while (millis() - gateOpenTime < 10000) {
        if (isCarPresent(TRIG_ENTRY_AFTER, ECHO_ENTRY_AFTER)) {
          carEntered = true;
          Serial.println("[ENTRY] Car detected at AFTER sensor");
          break;
        }
        delay(50);
      }
      
      if (carEntered) {
        // Wait for car to clear the gate
        while (isCarPresent(TRIG_ENTRY_AFTER, ECHO_ENTRY_AFTER)) {
          delay(50);
        }
        occupiedSpaces++;
        Serial.printf("[ENTRY] Car entered. Occupied: %d/%d\n", occupiedSpaces, totalSpaces);
      }
      
      closeEntryGate();
      updateDisplays();
      
    } else {
      // Parking full
      lcdEntry.clear();
      lcdEntry.setCursor(0, 0);
      lcdEntry.print("PARKING FULL");
      lcdEntry.setCursor(0, 1);
      lcdEntry.print("Access Denied");
      
      beepError();
    }
    
    rfidEntry.PICC_HaltA();
    delay(1000);
  }
  
  // ========== EXIT RFID CARD DETECTED ==========
  if (rfidExit.PICC_IsNewCardPresent() && rfidExit.PICC_ReadCardSerial()) {
    Serial.println("\n>>> [EXIT RFID] Card detected <<<");
    
    if (occupiedSpaces > 0) {
      // Success - Open exit gate
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("Access Granted");
      lcdExit.setCursor(0, 1);
      lcdExit.print("Gate Opening...");
      
      beepSuccess();
      openExitGate();
      
      // Wait for car to exit (using AFTER sensor)
      unsigned long gateOpenTime = millis();
      bool carExited = false;
      
      while (millis() - gateOpenTime < 10000) {
        if (isCarPresent(TRIG_EXIT_AFTER, ECHO_EXIT_AFTER)) {
          carExited = true;
          Serial.println("[EXIT] Car detected at AFTER sensor");
          break;
        }
        delay(50);
      }
      
      if (carExited) {
        // Wait for car to clear the gate
        while (isCarPresent(TRIG_EXIT_AFTER, ECHO_EXIT_AFTER)) {
          delay(50);
        }
        occupiedSpaces--;
        Serial.printf("[EXIT] Car exited. Occupied: %d/%d\n", occupiedSpaces, totalSpaces);
      }
      
      closeExitGate();
      updateDisplays();
      
      // Show goodbye message
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("Goodbye!");
      lcdExit.setCursor(0, 1);
      lcdExit.print("Drive Safely!");
      delay(2000);
      updateDisplays();
      
    } else {
      // No cars to exit
      lcdExit.clear();
      lcdExit.setCursor(0, 0);
      lcdExit.print("No Cars");
      lcdExit.setCursor(0, 1);
      lcdExit.print("to Exit!");
      
      beepError();
    }
    
    rfidExit.PICC_HaltA();
    delay(1000);
  }
  
  // ========== CAR DETECTION FOR LCD MESSAGES (Before gates) ==========
  
  // Check for car at ENTRY before gate (waiting to enter)
  if (isCarPresent(TRIG_ENTRY_BEFORE, ECHO_ENTRY_BEFORE)) {
    lcdEntry.clear();
    lcdEntry.setCursor(0, 0);
    lcdEntry.print("Welcome!");
    lcdEntry.setCursor(0, 1);
    lcdEntry.print("Scan Your Card");
  } else {
    // Show normal display if no car waiting
    int availableSpaces = totalSpaces - occupiedSpaces;
    lcdEntry.setCursor(0, 1);
    lcdEntry.print("Free: ");
    lcdEntry.print(availableSpaces);
    lcdEntry.print("/");
    lcdEntry.print(totalSpaces);
  }
  
  // Check for car at EXIT before gate (waiting to exit)
  if (isCarPresent(TRIG_EXIT_BEFORE, ECHO_EXIT_BEFORE)) {
    lcdExit.clear();
    lcdExit.setCursor(0, 0);
    lcdExit.print("Goodbye!");
    lcdExit.setCursor(0, 1);
    lcdExit.print("Scan Your Card");
  } else {
    // Show normal display if no car waiting
    int availableSpaces = totalSpaces - occupiedSpaces;
    lcdExit.setCursor(0, 1);
    lcdExit.print("Free: ");
    lcdExit.print(availableSpaces);
    lcdExit.print("/");
    lcdExit.print(totalSpaces);
  }
  
  // Debug output every 2 seconds
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    lastDebug = millis();
    Serial.printf("[DEBUG] Occupied: %d/%d\n", occupiedSpaces, totalSpaces);
    Serial.printf("  Entry Before: %s, After: %s\n", 
      isCarPresent(TRIG_ENTRY_BEFORE, ECHO_ENTRY_BEFORE) ? "CAR" : "CLEAR",
      isCarPresent(TRIG_ENTRY_AFTER, ECHO_ENTRY_AFTER) ? "CAR" : "CLEAR");
    Serial.printf("  Exit Before: %s, After: %s\n", 
      isCarPresent(TRIG_EXIT_BEFORE, ECHO_EXIT_BEFORE) ? "CAR" : "CLEAR",
      isCarPresent(TRIG_EXIT_AFTER, ECHO_EXIT_AFTER) ? "CAR" : "CLEAR");
  }
  
  delay(100);
}*/