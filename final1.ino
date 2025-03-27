#define BLYNK_TEMPLATE_ID "TMPL6-HH21cTx"
#define BLYNK_TEMPLATE_NAME "Final 1"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Cấu hình Blynk
char auth[] = "xxKbxD-1dxPZ6uPVlbllopvPCa6gSOSb";  
char ssid[] = "Nino";   
char pass[] = "28101995";   
#define NUM_SENSORS 3 

int sensorPins[NUM_SENSORS] = {34, 35, 32}; // ADC pins for soil moisture sensors
int relayPins[NUM_SENSORS] = {5, 17, 16};   // Digital pins for relays

// Virtual pins for Blynk (need to be configured in the Blynk app)
// V0: Global operation mode (0=auto, 1=manual)
// V1: Global moisture threshold to turn off pumps
// For each sensor/relay pair (i):
// V(10+i): Moisture level display
// V(20+i): Manual relay control
// V(30+i): Moisture threshold to turn on pump

// Variables for storing settings and readings
int moistureValues[NUM_SENSORS];         // Current moisture values
int moistureOnThresholds[NUM_SENSORS];   // Individual thresholds to turn ON pumps
int moistureOffThreshold = 0;            // Global threshold to turn OFF pumps
boolean operationMode = 0;               // 0=auto, 1=manual

void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Initialize all relay pins as outputs and turn them off
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
    
    // Initialize thresholds with default values
    moistureOnThresholds[i] = 30; // Default: turn on pump when moisture is below 30%
  }
  
  // Connect to Blynk
  Blynk.begin(auth, ssid, pass);
}

void loop() {
  Blynk.run();
  
  // Read all moisture sensors and update Blynk
  for (int i = 0; i < NUM_SENSORS; i++) {
    // Read moisture value
    moistureValues[i] = analogRead(sensorPins[i]);
    moistureValues[i] = map(moistureValues[i], 0, 4095, 100, 0); // ESP32 has 12-bit ADC (0-4095)
    
    // Send value to Blynk
    Blynk.virtualWrite(3 + i, moistureValues[i]);
    Serial.println("Soil moisture " + String(i + 1) + ": " + String(moistureValues[i]) + "%");
    
    // In auto mode, control relays based on moisture levels
    if (operationMode == 0) { // Auto mode
      if (moistureValues[i] < moistureOnThresholds[i]) {
        // Turn on pump if moisture is below threshold
        digitalWrite(relayPins[i], HIGH);
        Blynk.virtualWrite(6 + i, HIGH);
        Serial.println("Turning ON pump " + String(i + 1));
      } else if (moistureValues[i] > moistureOffThreshold) {
        // Turn off pump if moisture is above global off threshold
        digitalWrite(relayPins[i], LOW);
        Blynk.virtualWrite(6 + i, LOW);
        Serial.println("Turning OFF pump " + String(i + 1));
      }
    }
  }
  
  // Add a small delay to avoid flooding the serial monitor and Blynk server
  delay(1000);
}

// This function will be called when ESP32 connects to Blynk
BLYNK_CONNECTED() {
  // Synchronize all relevant virtual pins when connecting
  Blynk.syncVirtual(V13, V12); // Global settings
  
  // Sync individual sensor settings
  for (int i = 0; i < NUM_SENSORS; i++) {
    Blynk.syncVirtual(20 + i); // Relay states
    Blynk.syncVirtual(30 + i); // ON thresholds
  }
}

// Global operation mode (Auto/Manual)
BLYNK_WRITE(V13) {
  operationMode = param.asInt();
  Serial.println("Operation mode: " + String(operationMode ? "Manual" : "Auto"));
}

// Global moisture threshold to turn OFF all pumps
BLYNK_WRITE(V12) {
  moistureOffThreshold = param.asInt();
  Serial.println("Global OFF threshold: " + String(moistureOffThreshold) + "%");
}

// Handle individual relay controls and thresholds
// These functions are generated dynamically based on NUM_SENSORS
BLYNK_WRITE(V6) { // Relay 1 manual control
  if (operationMode == 1) { // Manual mode
    int value = param.asInt();
    digitalWrite(relayPins[0], value);
    Serial.println("Manual control - Relay 1: " + String(value ? "ON" : "OFF"));
  }
}

BLYNK_WRITE(V7) { // Relay 2 manual control
  if (operationMode == 1) { // Manual mode
    int value = param.asInt();
    digitalWrite(relayPins[1], value);
    Serial.println("Manual control - Relay 2: " + String(value ? "ON" : "OFF"));
  }
}

BLYNK_WRITE(V8) { // Relay 3 manual control
  if (operationMode == 1) { // Manual mode
    int value = param.asInt();
    digitalWrite(relayPins[2], value);
    Serial.println("Manual control - Relay 3: " + String(value ? "ON" : "OFF"));
  }
}

// Individual moisture thresholds to turn ON pumps
BLYNK_WRITE(V9) {
  moistureOnThresholds[0] = param.asInt();
  Serial.println("Moisture ON threshold 1: " + String(moistureOnThresholds[0]) + "%");
}

BLYNK_WRITE(V10) {
  moistureOnThresholds[1] = param.asInt();
  Serial.println("Moisture ON threshold 2: " + String(moistureOnThresholds[1]) + "%");
}

BLYNK_WRITE(V11) {
  moistureOnThresholds[2] = param.asInt();
  Serial.println("Moisture ON threshold 3: " + String(moistureOnThresholds[2]) + "%");
}