
#define BLYNK_TEMPLATE_ID "TMPL6ni_Y4x35"
#define BLYNK_TEMPLATE_NAME "More test"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Thông tin kết nối WiFi và Blynk
char auth[] = "-bj-_JrcIiO2NURvkqbu77NE8n2yDpMB";  // Thay bằng Auth Token của bạn
char ssid[] = "AKAMABU";         // Thay bằng tên WiFi của bạn
char pass[] = "12345678910";       // Thay bằng mật khẩu WiFi của bạn

// Định nghĩa các chân cảm biến (ESP32 ADC 12-bit: 0 - 4095)
const int sensorPins[] = {35, 34, 32};
const int numSensors = sizeof(sensorPins) / sizeof(sensorPins[0]);

// Gán Virtual Pin tương ứng cho từng cảm biến trên ứng dụng Blynk
const int virtualPins[] = {V1, V2, V3};

// Sử dụng BlynkTimer đã tích hợp sẵn trong thư viện Blynk
BlynkTimer timer;

// Hàm đọc và gửi dữ liệu độ ẩm đất lên Blynk
void sendSoilMoisture() {
  for (int i = 0; i < numSensors; i++) {
    // Đọc giá trị thô từ cảm biến
    int sensorValue = analogRead(sensorPins[i]);

    // Hiệu chuẩn giá trị:
    // - Giả sử với cảm biến của bạn: 
    //   + ~1023 tương ứng với đất ướt (100% độ ẩm)
    //   + ~2750 tương ứng với đất khô (0% độ ẩm)
    int moisturePercent = map(sensorValue, 1023, 2750, 100, 0);
    moisturePercent = constrain(moisturePercent, 0, 100);

    // In kết quả ra Serial để kiểm tra
    Serial.print("Cảm biến ở chân ");
    Serial.print(sensorPins[i]);
    Serial.print(" -> Giá trị thô: ");
    Serial.print(sensorValue);
    Serial.print(" => Độ ẩm đất: ");
    Serial.print(moisturePercent);
    Serial.println("%");

    // Gửi dữ liệu lên Blynk qua Virtual Pin tương ứng
    Blynk.virtualWrite(virtualPins[i], moisturePercent);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Khởi động và kết nối với WiFi, Blynk...");

  // Kết nối đến WiFi và Blynk
  Blynk.begin(auth, ssid, pass);

  // Thiết lập hàm sendSoilMoisture() chạy định kỳ mỗi 1 giây
  timer.setInterval(1000L, sendSoilMoisture);
}

void loop() {
  Blynk.run();
  timer.run();
}
