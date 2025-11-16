#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===== LCD SETUP =====
LiquidCrystal_I2C lcd(0x3F, 16, 2);  // I2C address 0x27 (change if needed)

// ===== CONFIG =====
const char* ssid = "test";
const char* password = "test1234";

String endpoint = "https://api.binance.com/api/v3/avgPrice?symbol=BTCUSDT";

float deltaTrigger = 0.02;      // 2%
int refreshSeconds = 10;      // 10 seconds

const int LED_GREEN = 5;   // safe pin
const int LED_RED   = 4;   // safe pin

float previousPrice = 0.0;

// Calculate percent change
float getPercent(float price, float prev) {
  if (prev == 0) return 0;
  return ((price - prev) * 100.0 / price);
}

// Flash red LED fast for alert
void flashAlert() {
  for (int i = 0; i < 20; i++) {
    digitalWrite(LED_RED, HIGH);
    delay(70);
    digitalWrite(LED_RED, LOW);
    delay(70);
  }
}

void setup() {
  Serial.begin(115200);

  // LCD Init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BTC Monitor");

  // LED pins
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  // WiFi
  lcd.setCursor(0, 1);
  lcd.print("WiFi Connecting");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  delay(1000);
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(endpoint);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    int code = http.GET();

    if (code == 200) {

      String payload = http.getString();

      // Extract price
      int a = payload.indexOf(":\"") + 2;
      int b = payload.indexOf("\"", a);
      float price = payload.substring(a, b).toFloat();

      float percent = getPercent(price, previousPrice);

      // ===== LCD DISPLAY =====
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("P:");
      lcd.print(price, 0);         // current price no decimals
      lcd.setCursor(12, 0);
      if (percent >= 0) lcd.print("+");
      lcd.print(percent, 2);

      lcd.setCursor(0, 1);
      lcd.print("Prev:");
      lcd.print(previousPrice, 0);

      // ===== LED INDICATION =====
      digitalWrite(LED_GREEN, percent > 0 ? HIGH : LOW);
      digitalWrite(LED_RED, percent < 0 ? HIGH : LOW);

      // ===== ALERT TRIGGER =====
      if (abs(percent) > deltaTrigger && previousPrice != 0) {
        flashAlert();
      }

      previousPrice = price;

      // Serial debug
      Serial.println("========================");
      Serial.print("Current: "); Serial.println(price);
      Serial.print("Previous: "); Serial.println(previousPrice);
      Serial.print("Percent: "); Serial.println(percent);
    }
    http.end();
  }

  delay(refreshSeconds * 1000);
}
