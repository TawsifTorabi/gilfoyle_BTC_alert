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

// Default coin
String symbol = "BTCUSDT"; // used for API endpoint
String coinName = "BTC";   // short name shown on LCD

// Build endpoint from symbol
String endpointBase = "https://api.binance.com/api/v3/ticker/24hr?symbol=";
String endpoint = endpointBase + symbol;

// Web server for dashboard
WebServer server(80);

// LEDs
const int LED_GREEN = 5;
const int LED_RED = 4;

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
    0b00000};

byte downArrow[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000};

// ===== WIFI RECONNECT =====
void wifiReconnect()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Reconnecting WiFi...");
    WiFi.reconnect();
  }
}

// ===== HELPER: update endpoint when symbol changes =====
void updateEndpointFromSymbol()
{
  endpoint = endpointBase + symbol;
  // coinName = first letters before "USDT" (assumes symbol ends with USDT)
  int idx = symbol.indexOf("USDT");
  if (idx > 0)
    coinName = symbol.substring(0, idx);
  else
    coinName = symbol;
}

// ===== FETCH FROM BINANCE =====
void fetchPrice()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(3000); // 3 sec max freeze
  http.begin(client, endpoint);
  int code = http.GET();

  if (code == 200)
  {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, payload);
    if (err)
    {
      Serial.print("JSON err: ");
      Serial.println(err.c_str());
      http.end();
      return;
    }

    // Get values
    latestPrice = doc["lastPrice"].as<float>();
    change24h = doc["priceChangePercent"].as<float>();

    if (previousPrice != 0)
      percentChange = ((latestPrice - previousPrice) / previousPrice) * 100.0;
    else
      percentChange = 0;

    // LED logic (based on immediate percentChange)
    digitalWrite(LED_GREEN, percentChange > 0 ? HIGH : LOW);
    digitalWrite(LED_RED, percentChange < 0 ? HIGH : LOW);

    // Update previousPrice for next fetch
    previousPrice = latestPrice;

    Serial.printf("Fetched %s: last=%.8f  24h%%=%.3f  delta%%=%.3f\n",
                  symbol.c_str(), latestPrice, change24h, percentChange);
  }
  else
  {
    Serial.printf("HTTP GET failed, code: %d\n", code);
  }

  http.end();
}

// ===== DISPLAY HANDLING =====
void showScreen1()
{
  lcd.clear();
  lcd.setCursor(0, 0);

  // First line: LatestPrice   arrow percent
  lcd.print(latestPrice, 2);
  lcd.print(" ");
  if (percentChange > 0)
    lcd.write(byte(0));
  else if (percentChange < 0)
    lcd.write(byte(1));
  else
    lcd.print("-");

  lcd.print(" ");
  lcd.print(percentChange, 2);
  lcd.print("%");

  // Second line: PreviousPrice and coinName
  lcd.setCursor(0, 1);
  lcd.print(previousPrice, 2);
  int spaceLeft = 16 - String(previousPrice, 2).length() - 1; // -1 for space
  // Print coinName at right if fits
  lcd.setCursor(16 - coinName.length(), 1);
  lcd.print(coinName);
}

void showScreen2()
{
  lcd.clear();

  // First line: LatestPrice   arrow percent
  lcd.setCursor(0, 0);
  lcd.print(latestPrice, 2);
  lcd.print(" ");
  if (percentChange > 0)
    lcd.write(byte(0));
  else if (percentChange < 0)
    lcd.write(byte(1));
  else
    lcd.print("-");

  lcd.print(" ");
  lcd.print(percentChange, 2);
  lcd.print("%");

  // Second line: 24h arrow 24h% coinName
  lcd.setCursor(0, 1);
  lcd.print("24h ");
  if (change24h > 0)
    lcd.write(byte(0));
  else if (change24h < 0)
    lcd.write(byte(1));
  else
    lcd.print("-");

  lcd.print(" ");
  lcd.print(change24h, 2);
  lcd.print("% ");

  // coinName at end if fits
  int used = 4 + 1 + String(change24h, 2).length() + 1 + 1; // approx
  int col = 16 - coinName.length();
  if (col > 0)
  {
    lcd.setCursor(col, 1);
    lcd.print(coinName);
  }
}

// ===== WEB DASHBOARD =====
// Serve a simple page with dropdown to pick symbol
void handleRoot()
{
  String html = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'/><title>Coin Selector</title></head><body>";
  html += "<h3>ESP Coin Selector</h3>";
  html += "<form action='/set' method='get'>";
  html += "<label for='sym'>Choose coin:</label>";
  html += "<select name='sym' id='sym'>";
  html += "<option value='BTCUSDT'" + String((symbol == "BTCUSDT") ? " selected" : "") + ">BTC/USDT</option>";
  html += "<option value='ETHUSDT'" + String((symbol == "ETHUSDT") ? " selected" : "") + ">ETH/USDT</option>";
  html += "<option value='BNBUSDT'" + String((symbol == "BNBUSDT") ? " selected" : "") + ">BNB/USDT</option>";
  html += "<option value='LTCUSDT'" + String((symbol == "LTCUSDT") ? " selected" : "") + ">LTC/USDT</option>";
  html += "<option value='XRPUSDT'" + String((symbol == "XRPUSDT") ? " selected" : "") + ">XRP/USDT</option>";
  html += "</select>";
  html += "<input type='submit' value='Set Coin'>";
  html += "</form>";
  html += "<p>Current symbol: <b>" + symbol + "</b></p>";
  html += "<p><a href='/status'>View status JSON</a></p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Handle /set?sym=ETHUSDT
void handleSet()
{
  if (server.hasArg("sym"))
  {
    String newSym = server.arg("sym");
    newSym.trim();
    if (newSym.length() > 0)
    {
      symbol = newSym;
      updateEndpointFromSymbol();
      previousPrice = 0; // reset so percent is recalculated next fetch
      // immediate fetch to populate values
      fetchPrice();
    }
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// Provide status JSON for web/dashboard
void handleStatus()
{
  DynamicJsonDocument doc(512);
  doc["symbol"] = symbol;
  doc["coin"] = coinName;
  doc["latestPrice"] = latestPrice;
  doc["previousPrice"] = previousPrice;
  doc["percentChange"] = percentChange;
  doc["24hPercent"] = change24h;
  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

// ===== SETUP =====
void setup()
{
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  // Start WiFi
  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.print("Connecting WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40)
  {
    delay(250);
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    String sip = ip.toString();
    lcd.clear();
    lcd.print("IP:");
    lcd.setCursor(0, 1);
    lcd.print(sip);
    Serial.print("Connected, IP: ");
    Serial.println(sip);
    delay(5000); // show IP for 5 seconds
  }
  else
  {
    lcd.clear();
    lcd.print("WiFi failed");
    Serial.println("WiFi not connected");
    delay(2000);
  }

  // Show starting message briefly
  lcd.clear();
  lcd.print("Starting...");
  delay(800);

  // Initialize web server routes
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/status", handleStatus);
  server.begin();

  // Ensure endpoint built from defaults
  updateEndpointFromSymbol();

  // Initial fetch
  fetchPrice();
}

// ===== LOOP =====
unsigned long lastFetch = 0;

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    wifiReconnect();
  }

  server.handleClient();

  // Fetch every 5s
  if (millis() - lastFetch > 5000)
  {
    fetchPrice();
    lastFetch = millis();
  }

  // Swap LCD screen every 2 seconds
  if (millis() - lastLCDSwap > 2000)
  {
    showPrevious = !showPrevious;
    if (showPrevious)
      showScreen1();
    else
      showScreen2();
    lastLCDSwap = millis();
  }
}
