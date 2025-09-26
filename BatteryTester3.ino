#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin definitions
const int switch1Pin = A0;   // Mode select (Manual/Auto)
const int switch2Pin = A1;   // Manual charge/discharge control
const int relay1Pin  = 8;    // Charge relay
const int relay2Pin  = 9;    // Discharge relay
const int potPin     = A2;   // Potentiometer for time setting

// Variables
bool manualMode = true;
bool autoSetupDone = false;
unsigned long startMillis = 0;
unsigned long intervalMillis = 0;
bool charging = true;

int chargeTime = 1;       // charging time in minutes
int dischargeTime = 1;    // discharging time in minutes

// Adjustable delay between relay switching (in ms)
const unsigned long relayDelay = 1500; 

// Track previous state in manual mode to detect change
bool lastManualChargingState = false;

void setup() {
  pinMode(switch1Pin, INPUT_PULLUP);
  pinMode(switch2Pin, INPUT_PULLUP);
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);

  // Relays off initially (HIGH = off for active-low relays)
  digitalWrite(relay1Pin, HIGH);
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
    autoSetupDone = false; // reset auto setup when switching to manual
    handleManualMode();
  } else {
    handleAutoMode();
  }
}

// ---------------- Manual Mode -----------------
void handleManualMode() {
  bool switch2State = digitalRead(switch2Pin); // HIGH = Charging, LOW = Discharging
  bool currentManualCharging = switch2State;

  // Detect if state changed from previous loop iteration
  if (currentManualCharging != lastManualChargingState) {
    // Turn both relays OFF first for safety
    digitalWrite(relay1Pin, HIGH);
    digitalWrite(relay2Pin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Switching...");
    lcd.setCursor(0, 1);
    lcd.print("Please wait...");
    delay(relayDelay); // Delay before switching relays
  }

  // Apply new state after safe delay
  if (currentManualCharging) {
    digitalWrite(relay1Pin, LOW);   // Charge ON
    digitalWrite(relay2Pin, HIGH);  // Discharge OFF
    lcdPrint("Mode: Manual", "Charging...");
  } else {
    digitalWrite(relay1Pin, HIGH);  // Charge OFF
    digitalWrite(relay2Pin, LOW);   // Discharge ON
    lcdPrint("Mode: Manual", "Discharging...");
  }

  lastManualChargingState = currentManualCharging;
}

// ---------------- Automatic Mode -----------------
void handleAutoMode() {
  if (!autoSetupDone) {
    // -------- Get CHARGING TIME --------
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set CHARGE time:");

    unsigned long setupStart = millis();
    while (millis() - setupStart < 10000) {
      if (digitalRead(switch1Pin) == HIGH) return; // exit if manual selected
      int potValue = analogRead(potPin);
      chargeTime = map(potValue, 0, 1023, 1, 30); // 1–30 min
      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(chargeTime);
      lcd.print(" min   ");
      delay(300);
    }

    // -------- Get DISCHARGING TIME --------
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set DISCHG time:");

    setupStart = millis();
    while (millis() - setupStart < 10000) {
      if (digitalRead(switch1Pin) == HIGH) return; // exit if manual selected
      int potValue = analogRead(potPin);
      dischargeTime = map(potValue, 0, 1023, 1, 30); // 1–30 min
      lcd.setCursor(0, 1);
      lcd.print("Value: ");
      lcd.print(dischargeTime);
      lcd.print(" min   ");
      delay(300);
    }

    // Lock values and start auto cycle
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Chg: ");
    lcd.print(chargeTime);
    lcd.print("m Dis: ");
    lcd.print(dischargeTime);
    lcd.print("m");
    lcd.setCursor(0, 1);
    lcd.print("Starting Cycle...");
    delay(2000);

    autoSetupDone = true;
    charging = true;
    intervalMillis = chargeTime * 60000UL;
    startMillis = millis();
  }

  // Switch mode when time elapsed
  if (millis() - startMillis >= intervalMillis) {
    // Turn both relays OFF first (safe switching)
    digitalWrite(relay1Pin, HIGH);
    digitalWrite(relay2Pin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Switching...");
    lcd.setCursor(0, 1);
    lcd.print("Please wait...");
    delay(relayDelay); // 1.5 second delay between relay switching

    charging = !charging;
    intervalMillis = (charging ? chargeTime : dischargeTime) * 60000UL;
    startMillis = millis();
  }

  unsigned long remaining = (intervalMillis - (millis() - startMillis)) / 1000;

  if (charging) {
    digitalWrite(relay1Pin, LOW);   // Charge ON
    digitalWrite(relay2Pin, HIGH);  // Discharge OFF
    lcd.setCursor(0, 0);
    lcd.print("Mode: Auto       ");
    lcd.setCursor(0, 1);
    lcd.print("Charging ");
    lcd.print(remaining / 60);
    lcd.print("m ");
    lcd.print(remaining % 60);
    lcd.print("s   ");
  } else {
    digitalWrite(relay1Pin, HIGH);  // Charge OFF
    digitalWrite(relay2Pin, LOW);   // Discharge ON
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
