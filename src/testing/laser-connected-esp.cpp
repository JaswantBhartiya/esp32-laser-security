#include <Arduino.h>

// --- HARDWARE PIN CONFIGURATION ---
const int LASER_PIN = 4;   // Connects to Laser Signal/VCC
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
void runAutoCalibration();
void monitorPerimeter();
void playSirenAndCheckReset();

void setup() {
  // Initialize Serial Monitor for system logs
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n=== SYSTEM BOOT: ZERO-DRIVER LASER SECURITY ACTIVE ===");

  // Define I/O Pin Modes
  pinMode(LASER_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  
  // Set up the Jumper Wire / Reset pin with internal pull-up resistor
  pinMode(RESET_PIN, INPUT_PULLUP);

  // Execute initial optical profiling
  runAutoCalibration();

  currentState = STATE_ARMED;
  Serial.println(">> SYSTEM STATUS: PERIMETER ARMED AND ACTIVE <<\n");
}

void loop() {
  // Application State Engine
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
 * Turns the laser on and off dynamically to compute an optimized 
 * tripwire threshold independent of background room lighting.
 */
void runAutoCalibration() {
  Serial.println("[!] Starting Optical Calibration...");

  // 1. Measure natural room light with Laser OFF
  Serial.println("-> Measuring ambient room light (Laser OFF)...");
  digitalWrite(LASER_PIN, LOW);
  delay(2000); // Allow sensor to settle
  ambientLight = analogRead(LDR_PIN);
  Serial.printf("   Ambient Baseline Raw Value: %d\n", ambientLight);

  // 2. Measure direct beam illumination with Laser ON
  Serial.println("-> Measuring laser target intensity (Laser ON)...");
  digitalWrite(LASER_PIN, HIGH);
  delay(2000); 
  laserLight = analogRead(LDR_PIN);
  Serial.printf("   Laser Baseline Raw Value: %d\n", laserLight);

  // 3. Set the trigger point exactly halfway between Light and Dark
  triggerThreshold = ambientLight + ((laserLight - ambientLight) / 2);
  Serial.printf("[SUCCESS] Dynamic Trigger Threshold set to: %d\n", triggerThreshold);

  // Safety contrast check (Ensures alignment is solid)
  if (abs(laserLight - ambientLight) < 400) {
    Serial.println("🚨 WARNING: Low optical contrast! Check alignment or shield the LDR.");
  }
}

/**
 * Checks the ADC pin continuously to see if the laser path is blocked.
 */
void monitorPerimeter() {
  int currentRead = analogRead(LDR_PIN);

  // If light drops past the midpoint threshold, a breach may be occurring
  if (currentRead < triggerThreshold) {
    delay(DEBOUNCE_DELAY); // Wait out instantaneous spikes or dust particles
    
    // Double-check the reading to confirm an actual obstacle
    if (analogRead(LDR_PIN) < triggerThreshold) {
      Serial.printf("\n🚨 BREACH DETECTED! Current Value: %d (Threshold: %d)\n", analogRead(LDR_PIN), triggerThreshold);
      currentState = STATE_BREACHED;
    }
  }
}

/**
 * Plays a non-blocking dual-tone siren wave, monitoring the 
 * reset wire every few microseconds to ensure instant disarming.
 */
void playSirenAndCheckReset() {
  // Phase A: Low Frequency Oscillation (~800 Hz)
  unsigned long startTime = millis();
  while (millis() - startTime < SIREN_SPEED) {
    // If the GPIO 18 wire touches a GND wire, the state drops to LOW
    if (digitalRead(RESET_PIN) == LOW) {
      delay(50); // Account for wire scraping/bouncing noise
      if (digitalRead(RESET_PIN) == LOW) {
        Serial.println("\n[!] Reset Signal Received. Quieting Siren...");
        digitalWrite(BUZZER_PIN, LOW);
        runAutoCalibration(); // Recalibrate immediately for current room state
        currentState = STATE_ARMED;
        return; // Terminate execution and return to monitoring
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
        runAutoCalibration();
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