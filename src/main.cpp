#include <Arduino.h>

// --- HARDWARE PIN CONFIGURATION ---
const int BUZZER_PIN = 5;  // Connects to Buzzer (+)
const int LDR_PIN = 34;    // Connects to the LDR Voltage Divider Junction
const int RESET_PIN = 18;  // Connects to your Jumper Wire / Reset Switch (Touches to GND)

// --- SYSTEM CONSTANTS ---
const int DEBOUNCE_DELAY = 50;  // Time (ms) the beam must be broken to trigger
const int SIREN_SPEED = 200;    // Oscillation speed (ms) of the audio alarm

// --- SYSTEM STATES ---
enum SecurityState { STATE_CALIBRATING, STATE_ARMED, STATE_BREACHED };
SecurityState currentState = STATE_CALIBRATING;

// --- CALIBRATION VARIABLES ---
int ambientLight = 0;
int laserLight = 0;
int triggerThreshold = 0;

// --- FUNCTION PROTOTYPES ---
void runIndependentCalibration();
void monitorPerimeter();
void playSirenAndCheckReset();

void setup() {
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n=== SYSTEM BOOT: INDEPENDENT POWER LASER GRID ===");

  // Define I/O Pin Modes (Notice LASER_PIN is removed)
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);

  // Execute manual-assist calibration
  runIndependentCalibration();

  currentState = STATE_ARMED;
  Serial.println(">> SYSTEM STATUS: PERIMETER ARMED AND ACTIVE <<\n");
}

void loop() {
  switch (currentState) {
    case STATE_ARMED:
      monitorPerimeter();
      break;

    case STATE_BREACHED:
      playSirenAndCheckReset();
      break;

    default:
      break;
  }
}

/**
 * Calibrates the system with an externally powered laser.
 * Expects the laser to be actively hitting the LDR on boot.
 */
void runIndependentCalibration() {
  Serial.println("[!] Starting Optical Calibration...");
  Serial.println("-> Step 1: Ensure external laser is aligned and hitting the LDR.");
  delay(3000); // Give the user time to verify alignment
  
  // 1. Capture the maximum intensity (Laser ON)
  laserLight = analogRead(LDR_PIN);
  Serial.printf("   Laser Target Value Captured: %d\n", laserLight);

  // 2. Estimate ambient light safely
  // Since we can't turn the laser off, we assume standard room ambient light baseline.
  // Alternatively, you can block the beam with your hand for a second here.
  Serial.println("-> Step 2: Sampling baseline variations...");
  delay(1000);
  ambientLight = laserLight / 4; // Mathematical fallback assumption for high contrast
  Serial.printf("   Estimated Ambient Baseline: %d\n", ambientLight);

  // 3. Compute dynamic threshold
  triggerThreshold = ambientLight + ((laserLight - ambientLight) / 2);
  Serial.printf("[SUCCESS] Dynamic Trigger Threshold set to: %d\n", triggerThreshold);

  // Check for weak signal
  if (laserLight < 1500) {
    Serial.println("🚨 WARNING: Weak laser signal! Align the beam better or shield the LDR from room lights.");
  }
}

void monitorPerimeter() {
  int currentRead = analogRead(LDR_PIN);

  if (currentRead < triggerThreshold) {
    delay(DEBOUNCE_DELAY); 
    if (analogRead(LDR_PIN) < triggerThreshold) {
      Serial.printf("\n🚨 BREACH DETECTED! Current Value: %d (Threshold: %d)\n", analogRead(LDR_PIN), triggerThreshold);
      currentState = STATE_BREACHED;
    }
  }
}

void playSirenAndCheckReset() {
  // Phase A: Low Frequency Oscillation (~800 Hz)
  unsigned long startTime = millis();
  while (millis() - startTime < SIREN_SPEED) {
    if (digitalRead(RESET_PIN) == LOW) {
      delay(50); // Debounce
      if (digitalRead(RESET_PIN) == LOW) {
        Serial.println("\n[!] Reset Signal Received. Quieting Siren...");
        digitalWrite(BUZZER_PIN, LOW);
        runIndependentCalibration(); // Recalibrate
        currentState = STATE_ARMED;
        return; 
      }
    }
    
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(625); 
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(625);
  }
  
  // Phase B: High Frequency Oscillation (~1300 Hz)
  startTime = millis();
  while (millis() - startTime < SIREN_SPEED) {
    if (digitalRead(RESET_PIN) == LOW) {
      delay(50);
      if (digitalRead(RESET_PIN) == LOW) {
        Serial.println("\n[!] Reset Signal Received. Quieting Siren...");
        digitalWrite(BUZZER_PIN, LOW);
        runIndependentCalibration();
        currentState = STATE_ARMED;
        return;
      }
    }

    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(384); 
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(384);
  }
}