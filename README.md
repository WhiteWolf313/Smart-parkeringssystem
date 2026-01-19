# 🚗 Smart Parkeringssystem (Arduino & RFID)

Detta projekt är ett automatiserat parkeringssystem byggt med Arduino. Systemet använder RFID-teknik för att hantera in- och utpassage, en RTC-modul för exakt tidsberäkning och servomotorer för att styra bommarna.

## 📋 Översikt

Systemet simulerar en verklig parkering där användaren "checkar in" med ett RFID-kort (biljett). Systemet loggar starttiden och öppnar bommen. Vid utgången scannar användaren kortet igen, varpå systemet räknar ut parkeringstiden och kostnaden. Efter simulerad betalning öppnas bommen.

### Nyckelfunktioner

* **RFID-incheckning:** Ingen pappersbiljett behövs; kortets unika ID (UID) fungerar som biljett.
* **Tidsberäkning:** Realtidsklocka (RTC) används för att beräkna exakt parkeringstid.
* **Priskalkyl:** Automatisk uträkning av kostnad (t.ex. 10 kr/h).
* **Kapacitetskontroll:** Håller koll på antalet lediga platser (Max 50 bilar).
* **Visuell feedback:** LCD-skärm visar status, tid och pris.

---

## 🛠 Hårdvara

För att bygga detta system krävs följande komponenter:

### Huvudkomponenter

* **Mikrokontroller:** Arduino Uno (x1 eller x2 beroende på uppdelning)
* **RFID-läsare:** MFRC522 (RC522)
* **Klockmodul (RTC):** DS3231 (för exakt tidshållning)
* **Display:** LCD 16x2 (gärna med I2C-modul)
* **Motorer:** 2x Servomotorer (SG90 eller MG995 för bommar)

### Sensorer & Input

* **IR-sensor:** För att detektera att en bil står vid ingången.
* **Tryckknapp:** För att simulera betalning vid utgången.
* **Summer (Piezo):** För ljudsignal vid scanning (valfritt).
* **RFID-taggar:** Kort eller nyckelbrickor.

---

## 💾 Bibliotek

Följande Arduino-bibliotek behövs för att koden ska fungera. Dessa kan installeras via *Library Manager* i Arduino IDE:

* `SPI.h` (Kommunikation med RFID)
* `MFRC522.h` (För RFID-läsaren)
* `Wire.h` (I2C-kommunikation)
* `RTClib.h` (För DS3231 klockan)
* `LiquidCrystal_I2C.h` (För LCD-skärmen)
* `Servo.h` (För bommarna)

---

## ⚙️ Så fungerar det (Logik)

Systemet bygger på en `struct` för att spara data om varje parkerad bil:

```cpp
struct Ticket {
  String id;               // RFID-kortets unika ID
  unsigned long startTid;  // Tidpunkt då bilen kom in (Unix tid)
};
