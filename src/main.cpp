#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ===== LCD SETUP =====
LiquidCrystal_I2C lcd(0x3F, 16, 2); // Set the LCD I2C address Default 0x3F or 0x27

// ===== CONFIG =====
const char *ssid = "test";         // your network SSID (name)
const char *password = "test1234"; // your network password

String endpoint = "https://api.binance.com/api/v3/avgPrice?symbol=BTCUSDT"; // API endpoint for BTC price
String coinName = "BTC";                                                    // Coin name for display

float deltaTrigger = 0.02; // Percentage change to trigger alert
int refreshSeconds = 15;   // Price fetch interval in seconds

const int LED_GREEN = 5; // GPIO for green LED
const int LED_RED = 4;   // GPIO for red LED

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

// Web server
WebServer server(80);

// Price history for chart
float priceHistory[300]; // Store last 300 data points
int priceIndex = 0;

// ===== WIFI RECONNECT =====
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

// ===== CALCULATE PERCENT =====
float getPercent(float p, float prev)
{
  if (prev == 0)
    return 0;
  return ((p - prev) * 100.0 / prev);
}

// ===== FLASH ALERT =====
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

// ===== FETCH PRICE FROM BINANCE =====
void fetchPrice()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  WiFiClientSecure client;
  client.setInsecure(); // skip SSL verification

  HTTPClient http;
  http.begin(client, endpoint);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  int code = http.GET();
  if (code == 200)
  {
    String payload = http.getString();               // Get response payload
    int a = payload.indexOf(":\"") + 2;              // Locate price in JSON
    int b = payload.indexOf("\"", a);                // End of price
    latestPrice = payload.substring(a, b).toFloat(); // Convert to float

    percentChange = getPercent(latestPrice, previousPrice); // Calculate percent change

    // LEDs
    digitalWrite(LED_GREEN, percentChange > 0 ? HIGH : LOW); // Green for up
    digitalWrite(LED_RED, percentChange < 0 ? HIGH : LOW);   // Red for down

    if (abs(percentChange) > deltaTrigger && previousPrice != 0)
      flashAlert(); // Flash alert if change exceeds threshold

    // Update chart data
    priceHistory[priceIndex] = latestPrice; // Store latest price
    priceIndex = (priceIndex + 1) % 50;     // Keep last 50 prices

    // LCD ticker
    lcd.clear();
    char arrowChar = percentChange > 0 ? 0 : (percentChange < 0 ? 1 : 2);            // Up, down, or neutral arrow
    String ticker = String(latestPrice, 2) + " @ " + String(percentChange, 2) + "%"; // Ticker text
    for (int i = 0; i <= ticker.length() - 16; i++)
    { // Scroll through ticker
      lcd.setCursor(0, 0);
      lcd.clear();
      for (int j = 0; j < 16; j++)
      {
        char c = ticker[i + j];
        if (c == '@')
        { // Insert arrow character
          if (arrowChar == 0)
            lcd.write(byte(0));
          else if (arrowChar == 1)
            lcd.write(byte(1));
          else
            lcd.print("-"); // Neutral
        }
        else
        {
          lcd.print(c);
        }
      }
      delay(250);
    }
    lcd.setCursor(0, 1);
    lcd.print(previousPrice, 3);
    lcd.print(" ");
    lcd.print(coinName);

    previousPrice = latestPrice;
  }
  http.end(); // Close connection
}

// ===== AJAX ENDPOINT =====
void handleData()
{                               // Serve JSON data for dashboard
  DynamicJsonDocument doc(512); // Allocate JSON document
  doc["latestPrice"] = latestPrice;
  doc["previousPrice"] = previousPrice;
  doc["percentChange"] = percentChange;

  JsonArray arr = doc.createNestedArray("history"); // Price history array
  for (int i = 0; i < 200; i++)
    arr.add(priceHistory[i]); // Add last 200 prices

  String response;
  serializeJson(doc, response);                   // Serialize JSON to string
  server.send(200, "application/json", response); // Send JSON response
}

// ===== MAIN DASHBOARD HTML =====
void handleRoot()
{
  String html = R"rawliteral(
<html>
<head>
  <title>BTC Monitor</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
  <h1>BTC Price Monitor</h1>
  <h2 id="currentPrice">Loading...</h2>
  <h3 id="previousPrice"></h3>
  <h3 id="percentChange"></h3>
  <canvas id="chart" width="400" height="200"></canvas>

  <script>
    const ctx = document.getElementById('chart').getContext('2d');
    const chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: Array(50).fill(''),
        datasets: [{
          label: 'BTC Price',
          data: Array(50).fill(0),
          borderWidth: 2,
          borderColor: 'orange',
          tension: 0.2
        }]
      },
      options: {
        responsive: true,
        scales: {
          y: {
            beginAtZero: false,
            grace: '5%'
          }
        }
      }
    });

    async function updateData() {
      const resp = await fetch('/data');
      const json = await resp.json();
      document.getElementById('currentPrice').textContent = 'Current Price: ' + json.latestPrice;
      document.getElementById('previousPrice').textContent = 'Previous: ' + json.previousPrice;
      document.getElementById('percentChange').textContent = 'Change: ' + json.percentChange.toFixed(2) + '%';

      chart.data.datasets[0].data = json.history;

      // Dynamically adjust y-axis range
      const prices = json.history;
      const min = Math.min(...prices);
      const max = Math.max(...prices);
      chart.options.scales.y.min = min - (max - min) * 0.1; // 10% padding below
      chart.options.scales.y.max = max + (max - min) * 0.1; // 10% padding above

      chart.update();
    }

    setInterval(updateData, 5000);
    updateData();
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// ===== SETUP =====
void setup()
{
  Serial.begin(115200);

  lcd.init();
  lcd.createChar(0, uparrow);
  lcd.createChar(1, downarrow);
  lcd.backlight();
  lcd.clear();
  lcd.print("BTC Monitor");

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

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

  configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // get NTP time
  struct tm timeinfo;                                // wait for time to be set
  while (!getLocalTime(&timeinfo))                   // wait for time
  {
    delay(500);
    Serial.println("Waiting for NTP...");
  }

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web Dashboard Ready!");
}

// ===== LOOP =====
unsigned long lastFetch = 0;
void loop()
{
  wifiReconnect();
  server.handleClient();

  if (millis() - lastFetch > refreshSeconds * 1000)
  {
    fetchPrice();
    lastFetch = millis();
  }
}
