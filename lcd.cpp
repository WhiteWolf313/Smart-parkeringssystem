#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 for the first screen and 0x3F for the second
// If 0x3F doesn't work, common alternatives are 0x20 or 0x3E
LiquidCrystal_I2C lcdIn(0x27, 16, 2);  // Entry Display
LiquidCrystal_I2C lcdOut(0x3F, 16, 2); // Exit Display

int totalSpaces = 10;
int occupiedSpaces = 4; // Example starting value

void setup() {
  // Start I2C communication
  Wire.begin(); 

  // Initialize Entry LCD
  lcdIn.init();
  lcdIn.backlight();
  
  // Initialize Exit LCD
  lcdOut.init();
  lcdOut.backlight();

  // Initial welcome message
  refreshDisplays();
}

void refreshDisplays() {
  int available = totalSpaces - occupiedSpaces;

  // --- Update Entry Display ---
  lcdIn.clear();
  lcdIn.setCursor(0, 0);
  lcdIn.print("Welcome!");
  lcdIn.setCursor(0, 1);
  lcdIn.print("Spaces: ");
  lcdIn.print(available);

  // --- Update Exit Display ---
  lcdOut.clear();
  lcdOut.setCursor(0, 0);
  lcdOut.print("Have a nice day");
  lcdOut.setCursor(0, 1);
  lcdOut.print("Status: Online");
}

void loop() {
  // In your full project, you would call refreshDisplays() 
  // whenever a car enters or exits.
}