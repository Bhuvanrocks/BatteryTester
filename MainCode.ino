#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin definitions
const int switch1Pin = A0;   // Mode select (Manual/Auto)
const int switch2Pin = A1;   // Manual charge/discharge control
const int relay1Pin  = 8;   // Charge relay
const int relay2Pin  = 9;   // Discharge relay
const int potPin     = A2;  // Potentiometer for time setting

// Variables
bool manualMode = true;
bool autoSetupDone = false;
int totalTime = 1;  // minutes
unsigned long startMillis = 0;
unsigned long intervalMillis = 0;
bool charging = true;

void setup() {
  pinMode(switch1Pin, INPUT_PULLUP);
  pinMode(switch2Pin, INPUT_PULLUP);
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);

  digitalWrite(relay1Pin, HIGH); // Initially off
  digitalWrite(relay2Pin, HIGH);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Charge-Discharge");
  lcd.setCursor(0, 1);
  lcd.print("Controller Ready");
  delay(2000);
  lcd.clear();
}

void loop() {
  manualMode = digitalRead(switch1Pin); // HIGH = manual, LOW = auto

  if (manualMode) {
    autoSetupDone = false; // reset auto setup when going manual
    handleManualMode();
  } else {
    handleAutoMode();
  }
}

// ---------------- Manual Mode -----------------
void handleManualMode() {
  bool switch2State = digitalRead(switch2Pin);

  if (switch2State == HIGH) {
    digitalWrite(relay1Pin, HIGH); // Charge ON
    digitalWrite(relay2Pin, LOW);
    lcdPrint("Mode: Manual", "Charging...");
  } else {
    digitalWrite(relay1Pin, LOW);
    digitalWrite(relay2Pin, HIGH); // Discharge ON
    lcdPrint("Mode: Manual", "Discharging...");
  }
}

// ---------------- Automatic Mode -----------------
void handleAutoMode() {
  if (!autoSetupDone) {
    // Give user 10s to set time
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set Time (mins):");

    unsigned long setupStart = millis();
    while (millis() - setupStart < 10000) {
      int potValue = analogRead(potPin);
      totalTime = map(potValue, 0, 1023, 1, 30); // 1–30 min

      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(totalTime);
      lcd.print(" min   ");
      delay(300);
    }

    // Lock time value
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time Locked: ");
    lcd.print(totalTime);
    lcd.print("m");
    lcd.setCursor(0, 1);
    lcd.print("Starting Cycle...");
    delay(2000);

    autoSetupDone = true;
    charging = true;
    intervalMillis = totalTime * 60000UL;
    startMillis = millis();
  }

  // Auto cycle running
  if (millis() - startMillis >= intervalMillis) {
    charging = !charging; // toggle mode
    startMillis = millis();
  }

  unsigned long remaining = (intervalMillis - (millis() - startMillis)) / 1000; // sec

  if (charging) {
    digitalWrite(relay1Pin, HIGH);
    digitalWrite(relay2Pin, LOW);
    lcd.setCursor(0, 0);
    lcd.print("Mode: Auto       ");
    lcd.setCursor(0, 1);
    lcd.print("Charging ");
    lcd.print(remaining / 60);
    lcd.print("m ");
    lcd.print(remaining % 60);
    lcd.print("s   ");
  } else {
    digitalWrite(relay1Pin, LOW);
    digitalWrite(relay2Pin, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("Mode: Auto       ");
    lcd.setCursor(0, 1);
    lcd.print("Dischg ");
    lcd.print(remaining / 60);
    lcd.print("m ");
    lcd.print(remaining % 60);
    lcd.print("s   ");
  }
}

// ---------------- Helper -----------------
void lcdPrint(String line1, String line2) {
  lcd.setCursor(0, 0);
  lcd.print(line1 + "        ");
  lcd.setCursor(0, 1);
  lcd.print(line2 + "        ");
}
