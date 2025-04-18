#define BLYNK_TEMPLATE_ID "..."  // Điền ID mẫu của bạn ở đây
#define BLYNK_TEMPLATE_NAME "..." // Điền tên mẫu của bạn ở đây
#define BLYNK_AUTH_TOKEN "..."   // Điền mã thông báo xác thực của bạn ở đây

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <EEPROM.h>

// Thông tin kết nối Wi-Fi
char ssid[] = "your_wifi";  // Tên Wi-Fi
char pass[] = "your_pass";  // Mật khẩu Wi-Fi

// Cảm biến DHT11 (Nhiệt độ và độ ẩm không khí)
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Chân cảm biến độ ẩm đất
#define SOIL_PIN1 34
#define SOIL_PIN2 35
#define SOIL_PIN3 32

// Chân điều khiển các relay (bơm)
#define RELAY1 14
#define RELAY2 27
#define RELAY3 26

// Trạng thái hệ thống
int operationMode = 0; // 0: Auto, 1: Manual
int autoThresholdOn = 50;
int autoThresholdOff = 70;
int manualThreshold1 = 60;
int manualThreshold2 = 60;
int manualThreshold3 = 60;

bool relayState1 = false;
bool relayState2 = false;
bool relayState3 = false;

// Làm mượt độ ẩm đất
#define EMA_ALPHA 0.1
float emaMoisture1 = 0.0;
float emaMoisture2 = 0.0;
float emaMoisture3 = 0.0;

// EEPROM
#define EEPROM_SIZE 32
#define ADDR_AUTO_ON       0
#define ADDR_AUTO_OFF      4
#define ADDR_MANUAL1       8
#define ADDR_MANUAL2       12
#define ADDR_MANUAL3       16

void saveThresholds() {
  EEPROM.writeInt(ADDR_AUTO_ON, autoThresholdOn);
  EEPROM.writeInt(ADDR_AUTO_OFF, autoThresholdOff);
  EEPROM.writeInt(ADDR_MANUAL1, manualThreshold1);
  EEPROM.writeInt(ADDR_MANUAL2, manualThreshold2);
  EEPROM.writeInt(ADDR_MANUAL3, manualThreshold3);
  EEPROM.commit();
}

void loadThresholds() {
  autoThresholdOn = EEPROM.readInt(ADDR_AUTO_ON);
  autoThresholdOff = EEPROM.readInt(ADDR_AUTO_OFF);
  manualThreshold1 = EEPROM.readInt(ADDR_MANUAL1);
  manualThreshold2 = EEPROM.readInt(ADDR_MANUAL2);
  manualThreshold3 = EEPROM.readInt(ADDR_MANUAL3);

  // Gán giá trị mặc định nếu chưa từng được lưu
  if (autoThresholdOn <= 0 || autoThresholdOn > 100) autoThresholdOn = 50;
  if (autoThresholdOff <= 0 || autoThresholdOff > 100) autoThresholdOff = 70;
  if (manualThreshold1 <= 0 || manualThreshold1 > 100) manualThreshold1 = 60;
  if (manualThreshold2 <= 0 || manualThreshold2 > 100) manualThreshold2 = 60;
  if (manualThreshold3 <= 0 || manualThreshold3 > 100) manualThreshold3 = 60;
}

// BLYNK WRITE HANDLERS
BLYNK_WRITE(V13) {
  operationMode = param.asInt();
}

BLYNK_WRITE(V10) {
  autoThresholdOn = param.asInt();
  saveThresholds();
}

BLYNK_WRITE(V11) {
  autoThresholdOff = param.asInt();
  saveThresholds();
}

BLYNK_WRITE(V20) {
  manualThreshold1 = param.asInt();
  saveThresholds();
}
BLYNK_WRITE(V21) {
  manualThreshold2 = param.asInt();
  saveThresholds();
}
BLYNK_WRITE(V22) {
  manualThreshold3 = param.asInt();
  saveThresholds();
}

BLYNK_WRITE(V6) {
  relayState1 = param.asInt();
  if (operationMode == 1) digitalWrite(RELAY1, relayState1);
}
BLYNK_WRITE(V7) {
  relayState2 = param.asInt();
  if (operationMode == 1) digitalWrite(RELAY2, relayState2);
}
BLYNK_WRITE(V8) {
  relayState3 = param.asInt();
  if (operationMode == 1) digitalWrite(RELAY3, relayState3);
}

// Tính EMA
float calculateEMA(float previousEMA, float currentReading) {
  return (EMA_ALPHA * currentReading) + ((1 - EMA_ALPHA) * previousEMA);
}

// Đọc độ ẩm đất (0–100%)
int readSoilMoisture(int pin) {
  int raw = analogRead(pin);
  if (raw < 100) return 0;
  return raw / 40.95;
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  loadThresholds();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);

  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, LOW);
}

void loop() {
  Blynk.run();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  Blynk.virtualWrite(V1, t);
  Blynk.virtualWrite(V2, h);

  int moisture1 = readSoilMoisture(SOIL_PIN1);
  int moisture2 = readSoilMoisture(SOIL_PIN2);
  int moisture3 = readSoilMoisture(SOIL_PIN3);

  emaMoisture1 = calculateEMA(emaMoisture1, moisture1);
  emaMoisture2 = calculateEMA(emaMoisture2, moisture2);
  emaMoisture3 = calculateEMA(emaMoisture3, moisture3);

  Blynk.virtualWrite(V3, emaMoisture1);
  Blynk.virtualWrite(V4, emaMoisture2);
  Blynk.virtualWrite(V5, emaMoisture3);

  if (operationMode == 0) {
    handlePumpAuto(RELAY1, emaMoisture1, autoThresholdOn, autoThresholdOff, V6);
    handlePumpAuto(RELAY2, emaMoisture2, autoThresholdOn, autoThresholdOff, V7);
    handlePumpAuto(RELAY3, emaMoisture3, autoThresholdOn, autoThresholdOff, V8);
  } else {
    manualControl(RELAY1, emaMoisture1, manualThreshold1, relayState1, V6);
    manualControl(RELAY2, emaMoisture2, manualThreshold2, relayState2, V7);
    manualControl(RELAY3, emaMoisture3, manualThreshold3, relayState3, V8);
  }

  delay(2000);
}

void handlePumpAuto(int relayPin, float moisture, int onThreshold, int offThreshold, int vPin) {
  if (moisture == 0) {
    digitalWrite(relayPin, LOW);
    Blynk.virtualWrite(vPin, LOW);
    return;
  }

  static bool state[3] = {false, false, false};
  int idx = (relayPin == RELAY1) ? 0 : (relayPin == RELAY2 ? 1 : 2);

  if (!state[idx] && moisture < onThreshold) {
    state[idx] = true;
  } else if (state[idx] && moisture > offThreshold) {
    state[idx] = false;
  }

  digitalWrite(relayPin, state[idx]);
  Blynk.virtualWrite(vPin, state[idx]);
}

void manualControl(int relayPin, float moisture, int threshold, bool manualState, int vPin) {
  if (moisture == 0) {
    digitalWrite(relayPin, LOW);
    Blynk.virtualWrite(vPin, LOW);
    return;
  }

  if (manualState) {
    digitalWrite(relayPin, HIGH);
    Blynk.virtualWrite(vPin, HIGH);
    return;
  }

  if (moisture < threshold) {
    digitalWrite(relayPin, HIGH);
    Blynk.virtualWrite(vPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
    Blynk.virtualWrite(vPin, LOW);
  }
}
