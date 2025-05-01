#define BLYNK_TEMPLATE_ID "TMPL6-HH21cTx"
#define BLYNK_TEMPLATE_NAME "Final 1"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <DHT_U.h>

// Cấu hình Blynk
char auth[] = "xxKbxD-1dxPZ6uPVlbllopvPCa6gSOSb";  
char ssid[] = "Nino";   
char pass[] = "28101995";

#define NUM_SENSORS 3 
int sensorPins[NUM_SENSORS] = {34, 35, 32};  // Cảm biến độ ẩm đất
int relayPins[NUM_SENSORS] = {5, 17, 16};    // Relay điều khiển bơm

#define V1 1  // Độ ẩm DHT11
#define V2 2  // Nhiệt độ DHT11

#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

BlynkTimer timer;  // Sử dụng timer để điều khiển tần suất đọc cảm biến

// Biến lưu giá trị cảm biến
int moistureValues[NUM_SENSORS];         
int moistureOnThresholds[NUM_SENSORS];   
int moistureOffThreshold = 0;           
boolean operationMode = 0;               // 0 = Auto, 1 = Manual

void setup() {
    Serial.begin(115200);
    dht.begin();
    Blynk.begin(auth, ssid, pass);
    
    for (int i = 0; i < NUM_SENSORS; i++) {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], LOW);
        moistureOnThresholds[i] = 30; // Mặc định bơm bật khi độ ẩm < 30%
    }

    // Đọc cảm biến mỗi 10 giây
    timer.setInterval(10000L, readSoilMoisture);
    timer.setInterval(30000L, readDHT11);
}

void loop() {
    Blynk.run();
    timer.run();
}

// Đọc cảm biến độ ẩm đất
void readSoilMoisture() {
    for (int i = 0; i < NUM_SENSORS; i++) {
        int newMoisture = analogRead(sensorPins[i]);
        newMoisture = map(newMoisture, 0, 4095, 100, 0);

        // Chỉ gửi nếu có thay đổi >2%
        if (abs(newMoisture - moistureValues[i]) > 2) {
            moistureValues[i] = newMoisture;
            Blynk.virtualWrite(3 + i, moistureValues[i]);
            Serial.println("Soil moisture " + String(i + 1) + ": " + String(moistureValues[i]) + "%");
        }

        // Điều khiển bơm tự động
        if (operationMode == 0) { 
            if (moistureValues[i] < moistureOnThresholds[i]) {
                digitalWrite(relayPins[i], HIGH);
                Blynk.virtualWrite(6 + i, HIGH);
            } else if (moistureValues[i] > moistureOffThreshold) {
                digitalWrite(relayPins[i], LOW);
                Blynk.virtualWrite(6 + i, LOW);
            }
        }
    }
}

// Đọc cảm biến DHT11
void readDHT11() {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (!isnan(h) && !isnan(t)) {
        static float lastH = -1, lastT = -1;
        if (abs(h - lastH) > 1) { 
            Blynk.virtualWrite(V1, h);
            lastH = h;
        }
        if (abs(t - lastT) > 1) {
            Blynk.virtualWrite(V2, t);
            lastT = t;
        }
        Serial.println("DHT11 - Độ ẩm: " + String(h) + "%, Nhiệt độ: " + String(t) + "°C");
    } else {
        Serial.println("❌ Lỗi đọc cảm biến DHT11!");
    }
}

// Đồng bộ dữ liệu từ Blynk khi ESP32 kết nối lại
BLYNK_CONNECTED() {
    Blynk.syncVirtual(V13, V12);
    for (int i = 0; i < NUM_SENSORS; i++) {
        Blynk.syncVirtual(6 + i);
        Blynk.syncVirtual(9 + i);
    }
}

// Chế độ Auto/Manual
BLYNK_WRITE(V13) {
    operationMode = param.asInt();
    Serial.println("Operation mode: " + String(operationMode ? "Manual" : "Auto"));
}

// Ngưỡng độ ẩm tắt bơm
BLYNK_WRITE(V12) {
    moistureOffThreshold = param.asInt();
    Serial.println("Global OFF threshold: " + String(moistureOffThreshold) + "%");
}

// Điều khiển bơm thủ công
void controlRelay(int relayIndex, int pin, int value, int vPin) {
    if (operationMode == 1) { 
        if (digitalRead(pin) != value) { 
            digitalWrite(pin, value);
            Blynk.virtualWrite(vPin, value);
            Serial.println("Manual control - Relay " + String(relayIndex + 1) + ": " + String(value ? "ON" : "OFF"));
        }
    } else {
        Blynk.virtualWrite(vPin, digitalRead(pin));
    }
}

BLYNK_WRITE(V6) { controlRelay(0, relayPins[0], param.asInt(), V6); }
BLYNK_WRITE(V7) { controlRelay(1, relayPins[1], param.asInt(), V7); }
BLYNK_WRITE(V8) { controlRelay(2, relayPins[2], param.asInt(), V8); }

// Cập nhật ngưỡng độ ẩm bật bơm
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
