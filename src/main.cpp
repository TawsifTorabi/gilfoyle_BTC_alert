#include <WiFi.h>
#include <HTTPClient.h>

// ===== CONFIG =====
const char* ssid = "test";
const char* password = "test1234";

String ticker = "BTC";
String convert = "USDT";
float deltaTrigger = 0.01;      // 1%
int refreshSeconds = 10;

const int LED_PIN = 2;          // Your alert LED (built-in LED = 2)

// Binance API endpoint
String endpoint = "https://api.binance.com/api/v3/avgPrice?symbol=BTCUSDT";

float previousPrice = 0.0;

float getPercent(float price, float prev) {
  if (prev == 0) return 0;
  return ((price - prev) * 100.0 / price);
}

void flashLEDAlert() {
  for (int i = 0; i < 20; i++) {   // fast flash
    digitalWrite(LED_PIN, HIGH);
    delay(80);
    digitalWrite(LED_PIN, LOW);
    delay(80);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(endpoint);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();

      // Extract the price
      int start = payload.indexOf(":\"") + 2;
      int end = payload.indexOf("\"", start);
      float price = payload.substring(start, end).toFloat();

      float percent = getPercent(price, previousPrice);

      Serial.println("=========================================");
      Serial.println("Asset: BTC");
      Serial.print("Previous: ");
      Serial.println(previousPrice);
      Serial.print("Current : ");
      Serial.println(price);
      Serial.print("Delta % : ");
      Serial.println(percent);
      Serial.println("=========================================");

      // Check trigger
      if (abs(percent) > deltaTrigger && previousPrice != 0) {
        Serial.println("ALERT! Price movement detected. Flashing LED...");
        flashLEDAlert();
      }

      previousPrice = price;
    } 
    else {
      Serial.print("HTTP Error: ");
      Serial.println(httpCode);
    }

    http.end();
  }

  delay(refreshSeconds * 1000);
}
