#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===== LCD SETUP =====
LiquidCrystal_I2C lcd(0x3F, 16, 2);

// ===== CONFIG =====
const char *ssid = "test";
const char *password = "test1234";

String endpoint = "https://api.binance.com/api/v3/avgPrice?symbol=BTCUSDT";
String coinName = "BTC";

float deltaTrigger = 0.02;
int refreshSeconds = 10;

const int LED_GREEN = 5;
const int LED_RED = 4;

float previousPrice = 0.0;
float latestPrice = 0.0;
float percentChange = 0.0;

byte downarrow[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
    0b00000};

byte uparrow[8] = {
    0b00000,
    0b00000,
    0b00100,
    0b01110,
    0b11111,
    0b00000,
    0b00000,
    0b00000};

// Web dashboard
WebServer server(80);

// Live graph data buffer (last 50 points)
float priceHistory[50];
int priceIndex = 0;

// =========== WIFI AUTO RECONNECT ============
void wifiReconnect()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.disconnect();
    WiFi.begin(ssid, password);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reconnecting...");
    Serial.println("WiFi reconnecting...");

    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 25)
    {
      delay(500);
      Serial.print(".");
      tries++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      lcd.clear();
      lcd.print("WiFi Restored");
      delay(500);
    }
  }
}

// =========== WEB DASHBOARD PAGE ============
String getDashboardHTML()
{
  String arrow = percentChange > 0 ? "⬆️" : (percentChange < 0 ? "⬇️" : "➡️");

  String html = "<html><head>"
                "<title>BTC Monitor</title>"
                "<meta http-equiv='refresh' content='10'>"
                "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
                "</head><body>"
                "<h1>BTC Price Monitor</h1>"
                "<h2>Current Price: " +
                String(latestPrice) + " " + arrow + "</h2>"
                                                    "<h3>Previous: " +
                String(previousPrice) + "</h3>"
                                        "<h3>Change: " +
                String(percentChange, 2) + "%</h3>"

                                           "<canvas id='chart' width='400' height='200'></canvas>"
                                           "<script>"
                                           "var ctx = document.getElementById('chart').getContext('2d');"
                                           "var chart = new Chart(ctx, { type: 'line', data: { labels: [";

  // add labels
  for (int i = 0; i < 50; i++)
  {
    html += "'" + String(i) + "',";
  }

  html += "], datasets: [{ label: 'BTC Price', data: [";

  for (int i = 0; i < 50; i++)
  {
    html += String(priceHistory[i]) + ",";
  }

  html += "], borderWidth: 2 }] }, options: {} });"
          "</script></body></html>";

  return html;
}

void handleRoot()
{
  server.send(200, "text/html", getDashboardHTML());
}

// ============= LCD TICKER SCROLL ============
void lcdTickerSpecial(String text, char arrowChar)
{
  for (int i = 0; i <= text.length() - 16; i++)
  {
    lcd.setCursor(0, 0);
    lcd.clear();

    for (int j = 0; j < 16; j++)
    {
      char c = text[i + j];

      if (c == '@')
      {
        if (arrowChar == 0)
          lcd.write(byte(0)); // up arrow
        else if (arrowChar == 1)
          lcd.write(byte(1)); // down arrow
        else
          lcd.print("-"); // neutral
      }
      else
      {
        lcd.print(c);
      }
    }

    delay(250);
  }
}

// Calculate percent
float getPercent(float p, float prev)
{
  if (prev == 0)
    return 0;
  return ((p - prev) * 100.0 / prev);
}

// Flash red LED fast
void flashAlert()
{
  for (int i = 0; i < 20; i++)
  {
    digitalWrite(LED_RED, HIGH);
    delay(70);
    digitalWrite(LED_RED, LOW);
    delay(70);
  }
}

void setup()
{
  Serial.begin(115200);

  // LCD
  lcd.init();
  lcd.createChar(0, uparrow);   // custom char index 0
  lcd.createChar(1, downarrow); // custom char index 1
  lcd.backlight();
  lcd.clear();
  lcd.print("BTC Monitor");

  // LEDs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  // WiFi
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(400);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("WiFi OK");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  while (!getLocalTime(&timeinfo))
  {
    Serial.println("Waiting for NTP...");
    delay(500);
  }

  // Start web server
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web Dashboard Ready!");
}

void loop()
{
  wifiReconnect();
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure client;
    client.setInsecure(); // SKIP SSL CERT CHECK

    HTTPClient http;
    http.begin(client, endpoint);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    int code = http.GET();
    if (code == 200)
    {
      String payload = http.getString();
      int a = payload.indexOf(":\"") + 2;
      int b = payload.indexOf("\"", a);
      latestPrice = payload.substring(a, b).toFloat();

      percentChange = getPercent(latestPrice, previousPrice);
      Serial.println("===============");
      Serial.println("Latest Price: " + String(latestPrice));
      Serial.println("Percent Change: " + String(percentChange)); 
      Serial.println("Previous Price: " + String(previousPrice));
      Serial.println("===============");

      // Update LED
      digitalWrite(LED_GREEN, percentChange > 0 ? HIGH : LOW);
      digitalWrite(LED_RED, percentChange < 0 ? HIGH : LOW);

      // Trigger alarm
      if (abs(percentChange) > deltaTrigger && previousPrice != 0)
        flashAlert();

      // Update graph buffer
      priceHistory[priceIndex] = latestPrice;
      priceIndex = (priceIndex + 1) % 50;

      // ===== LCD DISPLAY =====
      lcd.clear();

      char arrowChar = percentChange > 0 ? 0 : (percentChange < 0 ? 1 : 2);
      String ticker = String(latestPrice, 2) + " @ " + String(percentChange, 2) + "%";  //Latest price set to 1 decimal place because BTC price is large for LCD Screen :p

      lcdTickerSpecial(ticker, arrowChar);

      lcd.setCursor(0, 1);
      lcd.print(previousPrice, 3);
      lcd.print(" ");
      lcd.print(coinName);

      previousPrice = latestPrice;

      Serial.println("===============");
      Serial.println("Current: " + String(latestPrice));
    }

    http.end();
  }

  delay(refreshSeconds * 1000);
}
