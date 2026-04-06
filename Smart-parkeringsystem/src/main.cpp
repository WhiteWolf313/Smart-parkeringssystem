/*
 * SMART PARKING SYSTEM - PROFESSIONELL VERSION (NON-BLOCKING)
 * Hårdvara: ESP32 DevKitC V4 (38-pin), 2x RFID, 2x Servo, 2x LCD, 4x HC-SR04, 1x Buzzer
 * Databas: Redis via Wi-Fi (med LittleFS Offline-stöd)
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Redis.h>

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

// =======================================================
// 1. INSTÄLLNINGAR & VARIABLER
// =======================================================
const char* ssid = "DITT_WIFI_NAMN";
const char* password = "DITT_WIFI_LOSENORD";
const char* redis_host = "192.168.1.100"; // ange IP för din ziro
const int redis_port = 6379;
const char* queueFile = "/queue.txt";

int freeSpots = 50; 
WiFiClient redisClient;

// =======================================================
// 2. NÄTVERK OCH DATABAS
// =======================================================

void connectWiFi() {
  Serial.print("Ansluter till Wi-Fi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500); 
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println(" OFFLINE! Fortsätter utan internet.");
  }
}

bool checkAccess(String uid) {
  if (WiFi.status() == WL_CONNECTED) {
    if (redisClient.connect(redis_host, redis_port)) {
      Redis redis(redisClient); 
      String access = redis.get(("access:" + uid).c_str());
      redisClient.stop(); 
      return (access == "1"); 
    }
  }
  Serial.println("Databas ej nåbar. Accepterar kort via Offline-fallback.");
  return true; // Låt alla köra ut/in om nätet är dött för att undvika kaos
}

void logPassage(String uid, String direction) {
  String logData = uid + "," + direction + "," + String(millis());
  if (WiFi.status() == WL_CONNECTED) {
    if (redisClient.connect(redis_host, redis_port)) {
      Redis redis(redisClient);
      redis.rpush("parking_logs", logData.c_str());
      redisClient.stop();
      return; 
    }
  }
  
  // Offline lagring
  File file = LittleFS.open(queueFile, FILE_APPEND);
  if (file) {
    file.println(logData);
    file.close();
  }
}

void syncOfflineData() {
  if (WiFi.status() == WL_CONNECTED && LittleFS.exists(queueFile)) {
    if (redisClient.connect(redis_host, redis_port)) {
      Redis redis(redisClient);
      File file = LittleFS.open(queueFile, FILE_READ);
      while (file.available()) {
        String logData = file.readStringUntil('\n');
        logData.trim();
        if (logData.length() > 0) {
          redis.rpush("parking_logs", logData.c_str());
        }
      }
      file.close(); 
      redisClient.stop();
      LittleFS.remove(queueFile); 
      Serial.println("Offline-data uppladdad till Redis!");
    }
  }
}