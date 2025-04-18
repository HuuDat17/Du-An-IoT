#define BLYNK_TEMPLATE_ID "TMPL6-HH21cTx"
#define BLYNK_TEMPLATE_NAME "Final 1"
#define BLYNK_AUTH_TOKEN "xxKbxD-1dxPZ6uPVlbllopvPCa6gSOSb"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

char ssid[] = "Nguyenhuudat";
char pass[] = "bepgastinhte3979@@@@";

// Pin cảm biến độ ẩm đất và nhiệt độ
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SOIL_PIN1 34
#define SOIL_PIN2 35
#define SOIL_PIN3 32

#define SENSOR_THRESHOLD 500

// Relay để điều khiển bơm
#define RELAY1 14
#define RELAY2 27
#define RELAY3 26

#define EMA_ALPHA 0.1
float emaMoisture1 = 0.0, emaMoisture2 = 0.0, emaMoisture3 = 0.0;
bool sensor1_connected = false, sensor2_connected = false, sensor3_connected = false;

int operationMode = 0;  // 0 = tự động, 1 = thủ công
int autoThresholdOn = 30, autoThresholdOff = 50;
int manualThreshold1 = 60, manualThreshold2 = 60, manualThreshold3 = 60;
bool relayState1 = false, relayState2 = false, relayState3 = false;

BlynkTimer timer;

// ===== BLYNK WRITE HANDLERS =====
BLYNK_WRITE(V13) { 
  operationMode = param.asInt();  // Chuyển chế độ từ auto sang thủ công
}
BLYNK_WRITE(V10) { autoThresholdOn = param.asInt(); }
BLYNK_WRITE(V11) { autoThresholdOff = param.asInt(); }
BLYNK_WRITE(V20) { manualThreshold1 = param.asInt(); }
BLYNK_WRITE(V21) { manualThreshold2 = param.asInt(); }
BLYNK_WRITE(V22) { manualThreshold3 = param.asInt(); }

BLYNK_WRITE(V6) { 
  if (operationMode == 1) {  // Chỉ cho phép điều khiển thủ công
    relayState1 = param.asInt(); 
    digitalWrite(RELAY1, relayState1);  // Điều khiển bơm 1
  }
}
BLYNK_WRITE(V7) { 
  if (operationMode == 1) {
    relayState2 = param.asInt(); 
    digitalWrite(RELAY2, relayState2);  // Điều khiển bơm 2
  }
}
BLYNK_WRITE(V8) { 
  if (operationMode == 1) { 
    relayState3 = param.asInt(); 
    digitalWrite(RELAY3, relayState3);  // Điều khiển bơm 3
  }
}

// ===== FUNCTIONS =====
int readSoilMoisture(int pin, bool &connected) {
  int raw = analogRead(pin);
  if (raw < SENSOR_THRESHOLD) {
    connected = false;
    return 0;
  }
  connected = true;
  int percent = map(raw, 2503, 956, 0, 100); 
  return constrain(percent, 0, 100);
}

float calculateEMA(float previousEMA, float currentReading) {
  return (EMA_ALPHA * currentReading) + ((1 - EMA_ALPHA) * previousEMA);
}

void checkSensorConnection() {
  int raw1 = analogRead(SOIL_PIN1);
  int raw2 = analogRead(SOIL_PIN2);
  int raw3 = analogRead(SOIL_PIN3);
  sensor1_connected = (raw1 > SENSOR_THRESHOLD);
  sensor2_connected = (raw2 > SENSOR_THRESHOLD);
  sensor3_connected = (raw3 > SENSOR_THRESHOLD);

  Serial.print("Raw Values: ");
  Serial.print(raw1); Serial.print(" | ");
  Serial.print(raw2); Serial.print(" | ");
  Serial.println(raw3);
}

void updateSoilMoisture() {
  int moisture1 = readSoilMoisture(SOIL_PIN1, sensor1_connected);
  int moisture2 = readSoilMoisture(SOIL_PIN2, sensor2_connected);
  int moisture3 = readSoilMoisture(SOIL_PIN3, sensor3_connected);

  if (sensor1_connected) emaMoisture1 = calculateEMA(emaMoisture1, moisture1);
  if (sensor2_connected) emaMoisture2 = calculateEMA(emaMoisture2, moisture2);
  if (sensor3_connected) emaMoisture3 = calculateEMA(emaMoisture3, moisture3);
}

void sendSensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Blynk.virtualWrite(V1, t);  // Gửi nhiệt độ lên Blynk
  Blynk.virtualWrite(V2, h);  // Gửi độ ẩm lên Blynk
  Blynk.virtualWrite(V3, emaMoisture1);  // Gửi độ ẩm đất 1 lên Blynk
  Blynk.virtualWrite(V4, emaMoisture2);  // Gửi độ ẩm đất 2 lên Blynk
  Blynk.virtualWrite(V5, emaMoisture3);  // Gửi độ ẩm đất 3 lên Blynk
}

void handlePumpAuto(int relayPin, float moisture, int onThreshold, int offThreshold, int vPin) {
  if (moisture == 0) {
    digitalWrite(relayPin, LOW);
    Blynk.virtualWrite(vPin, LOW);
    return;
  }
  static bool state[3] = {false, false, false};
  int idx = (relayPin == RELAY1) ? 0 : (relayPin == RELAY2 ? 1 : 2);
  if (!state[idx] && moisture < onThreshold) state[idx] = true;
  else if (state[idx] && moisture > offThreshold) state[idx] = false;

  digitalWrite(relayPin, state[idx]);
  Blynk.virtualWrite(vPin, state[idx]);
}

void manualControl(int relayPin, float moisture, int threshold, bool manualState, int vPin) {
  if (moisture == 0) {
    digitalWrite(relayPin, LOW);
    Blynk.virtualWrite(vPin, LOW);
    return;
  }
  if (manualState || moisture < threshold) {
    digitalWrite(relayPin, HIGH);
    Blynk.virtualWrite(vPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
    Blynk.virtualWrite(vPin, LOW);
  }
}

void controlPumpTask() {
  if (operationMode == 0) {  // Chế độ tự động
    handlePumpAuto(RELAY1, emaMoisture1, autoThresholdOn, autoThresholdOff, V6);
    handlePumpAuto(RELAY2, emaMoisture2, autoThresholdOn, autoThresholdOff, V7);
    handlePumpAuto(RELAY3, emaMoisture3, autoThresholdOn, autoThresholdOff, V8);
  } else {  // Chế độ thủ công
    manualControl(RELAY1, emaMoisture1, manualThreshold1, relayState1, V6);
    manualControl(RELAY2, emaMoisture2, manualThreshold2, relayState2, V7);
    manualControl(RELAY3, emaMoisture3, manualThreshold3, relayState3, V8);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(SOIL_PIN1, INPUT_PULLDOWN);
  pinMode(SOIL_PIN2, INPUT_PULLDOWN);
  pinMode(SOIL_PIN3, INPUT_PULLDOWN);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);

  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, LOW);

  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  checkSensorConnection();  // Kiểm tra kết nối ban đầu

  // Task định kỳ
  timer.setInterval(3000L, sendSensorData);
  timer.setInterval(3000L, updateSoilMoisture);
  timer.setInterval(9000L, checkSensorConnection);
  timer.setInterval(3500L, controlPumpTask);
}

void loop() {
  Blynk.run();
  timer.run();
}
