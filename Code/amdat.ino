#define BLYNK_TEMPLATE_ID "TMPL698XwDz3U"
#define BLYNK_TEMPLATE_NAME "lab01"
#define BLYNK_AUTH_TOKEN "rnESLQK5eyLPffpdGqygW1k8Y-AWaJB5"  // Thay bằng mã token chính xác

#include <WiFi.h>
#include <WiFiClient.h>
#include "BlynkSimpleEsp32.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Thông tin WiFi
char ssid[] = "Nguyen Huu Dat";  
char pass[] = "Dat666666";  

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Cảm biến độ ẩm đất
#define SOIL_MOISTURE_PIN 34  // Chân ADC34 của ESP32

// Giá trị ADC thực tế (đo từ Serial Monitor)
#define ADC_100 2000   // Giá trị khi nhúng hoàn toàn trong nước
#define ADC_0   3500   // Giá trị khi cảm biến hoàn toàn khô

BlynkTimer timer;
bool wifiConnected = false;

void sendSensorData() {
  int rawValue = analogRead(SOIL_MOISTURE_PIN);  // Đọc giá trị từ cảm biến
  float do_am_dat = map(rawValue, ADC_100, ADC_0, 100, 0); // Chuyển đổi thành %

  // Giới hạn giá trị từ 0 - 100%
  do_am_dat = constrain(do_am_dat, 0, 100);

  Serial.print("ADC đọc được: ");
  Serial.print(rawValue);
  Serial.print(" | Độ ẩm đất: ");
  Serial.print(do_am_dat);
  Serial.println(" %");

  // Hiển thị lên LCD
  lcd.setCursor(0, 0);
  lcd.print("Do am dat: ");
  lcd.print(do_am_dat);
  lcd.print(" %  ");

  // Gửi dữ liệu lên Blynk
  Blynk.virtualWrite(V1, do_am_dat);
}

void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("Mất kết nối WiFi! Đang kết nối lại...");
      wifiConnected = false;
    }
    WiFi.disconnect();
    WiFi.reconnect();
  } else {
    if (!wifiConnected) {
      Serial.println("Đã kết nối WiFi!");
      wifiConnected = true;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Khởi động ESP32...");
  
  // Kết nối Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  // Khởi tạo LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Khoi dong ESP32...");
  
  // Chạy hàm đo cảm biến mỗi 2 giây
  timer.setInterval(2000L, sendSensorData);
  // Kiểm tra kết nối WiFi mỗi 10 giây
  timer.setInterval(10000L, checkWiFi);
}

void loop() {
  Blynk.run();
  timer.run();
}
