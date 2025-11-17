#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ===== LCD =====
LiquidCrystal_I2C lcd(0x3F, 16, 2);

// ===== WIFI CONFIG =====
const char *ssid = "test";
const char *password = "test1234";

// Binance 24hr ticker API
String endpoint = "https://api.binance.com/api/v3/ticker/24hr?symbol=BTCUSDT";
String coinName = "BTC";

// LEDs
const int LED_GREEN = 5;
const int LED_RED   = 4;

// Price variables
float latestPrice = 0;
float previousPrice = 0;
float percentChange = 0;
float change24h = 0;

unsigned long lastLCDSwap = 0;
bool showPrevious = true;

// ===== Custom LCD Characters =====
byte upArrow[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

byte downArrow[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};

// ===== WIFI RECONNECT =====
void wifiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    lcd.clear();
    lcd.print("Reconnecting...");
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 20) {
      delay(500);
      tries++;
    }
    lcd.clear();
    lcd.print("WiFi OK");
  }
}

// ===== FETCH FROM BINANCE =====
void fetchPrice() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  http.begin(client, endpoint);
  int code = http.GET();

  if (code == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, payload);
    if (err) return;

    latestPrice = doc["lastPrice"].as<float>();
    change24h   = doc["priceChangePercent"].as<float>();

    if (previousPrice != 0)
      percentChange = ((latestPrice - previousPrice) / previousPrice) * 100.0;
    else
      percentChange = 0;

    // LED logic
    digitalWrite(LED_GREEN, percentChange > 0);
    digitalWrite(LED_RED, percentChange < 0);

    previousPrice = latestPrice;
  }

  http.end();
}

// ===== DISPLAY HANDLING =====
void showScreen1() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(latestPrice, 2);
  lcd.print(" ");

  if (percentChange > 0) lcd.write(byte(0));
  else lcd.write(byte(1));

  lcd.print(" ");
  lcd.print(percentChange, 2);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print(previousPrice, 2);
  lcd.print(" ");
  lcd.print(coinName);
}

void showScreen2() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(latestPrice, 2);
  lcd.print(" ");

  if (percentChange > 0) lcd.write(byte(0));
  else lcd.write(byte(1));

  lcd.print(" ");
  lcd.print(percentChange, 2);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("24h");
  if (change24h > 0) lcd.write(byte(0));
  else lcd.write(byte(1));
  lcd.print(" ");

  lcd.print(change24h, 2);
  lcd.print("% ");
  lcd.print(coinName);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);

  lcd.print("Starting...");

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
  }

  lcd.clear();
  lcd.print("WiFi OK");
  delay(1000);
}

// ===== LOOP =====
unsigned long lastFetch = 0;

void loop() {
  wifiReconnect();

  // Fetch every 5s
  if (millis() - lastFetch > 5000) {
    fetchPrice();
    lastFetch = millis();
  }

  // Swap LCD screen every 2 seconds
  if (millis() - lastLCDSwap > 2000) {
    showPrevious = !showPrevious;
    if (showPrevious) showScreen1();
    else showScreen2();
    lastLCDSwap = millis();
  }
}
