#define BLYNK_TEMPLATE_ID "TMPL698XwDz3U"
#define BLYNK_TEMPLATE_NAME "lab01"
#define BLYNK_AUTH_TOKEN "rnESLQK5eyLPffpdGqygW1k8Y-AWaJB5"  // Thay bằng mã token chính xác

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Thông tin WiFi
char ssid[] = "Nguyen Huu Dat";  
char pass[] = "Dat666666";  

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Cảm biến DHT11
#define DHTPIN 4  
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

BlynkTimer timer;
bool wifiConnected = false;

void sendSensorData() {
  float nhiet_do = dht.readTemperature();
  float do_am = dht.readHumidity();

  if (isnan(nhiet_do) || isnan(do_am)) {
    Serial.println("Lỗi cảm biến DHT!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Loi cam bien!");
    return;
  }

  Serial.print("Nhiet do: ");
  Serial.print(nhiet_do);
  Serial.print(" C, Do am: ");
  Serial.print(do_am);
  Serial.println(" %");

  // Hiển thị lên LCD
  lcd.setCursor(0, 0);
  lcd.print("Nhiet do: ");
  lcd.print(nhiet_do);
  lcd.print(" C ");

  lcd.setCursor(0, 1);
  lcd.print("Do am: ");
  lcd.print(do_am);
  lcd.print(" %  ");

  // Gửi dữ liệu lên Blynk
  Blynk.virtualWrite(V0, nhiet_do);
  Blynk.virtualWrite(V1, do_am);
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
  
  // Khởi tạo LCD và cảm biến DHT
  lcd.init();
  lcd.backlight();
  lcd.clear();
  dht.begin();

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
