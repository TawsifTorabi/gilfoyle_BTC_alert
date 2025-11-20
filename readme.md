
# 💰 Gilfoyle BTC Alert 🚨

![Stars](https://img.shields.io/github/stars/TawsifTorabi/gilfoyle_BTC_alert?style=flat-square) ![Forks](https://img.shields.io/github/forks/TawsifTorabi/gilfoyle_BTC_alert?style=flat-square) ![Issues](https://img.shields.io/github/issues/TawsifTorabi/gilfoyle_BTC_alert?style=flat-square) ![License](https://img.shields.io/github/license/TawsifTorabi/gilfoyle_BTC_alert?style=flat-square) ![Build](https://img.shields.io/badge/build-passing-brightgreen?style=flat-square) ![Version](https://img.shields.io/badge/version-1.0.0-blue?style=flat-square)

A smart Bitcoin price monitoring system built for ESP8266/ESP32 with LCD display, LEDs, Web Dashboard, and real-time Binance API alerts.


![Gilfoyle](docs/gilfoyle.png)


- - -

## 📌 Overview

**Gilfoyle BTC Alert** is an IoT-based Bitcoin monitoring system inspired by the character Gilfoyle from Silicon Valley. It fetches BTC price data from the Binance API, displays current market values on an LCD, tracks price direction with LEDs, provides alerts, and also includes a built-in Web Dashboard with live charts.

- - -

## ✨ Features

*   📡 Real-time BTC price fetch from Binance
*   📟 16x2 LCD display for price, direction, and trend
*   🔴🟢 Price direction LEDs (UP/DOWN)
*   📉 Percentage change tracking
*   🔔 Price alert logic
*   ⚡ Lightweight & efficient codebase
*   🔐 SSL support for Binance API (with fallback HTTP mode)

- - -

## 🧩 Hardware Requirements

*   ESP8266 or ESP32
*   16x2 LCD with I2C module
*   1x RED LED (Price Down)
*   1x GREEN LED (Price Up)
*   WiFi connection

- - -

## 📁 Project Structure

gilfoyle\_BTC\_alert/
├── esp8266/
│   ├── \*.ino (Full ESP8266 project code)
│
├── esp32/
│   ├── \*.ino (Full ESP32 project code)
│
├── web/
│   ├── dashboard.html (Live chart + stats)
│
└── assets/
    ├── images, diagrams, etc.

- - -

## ⚙️ How It Works

1.  ESP connects to WiFi.
2.  Makes GET requests to Binance Ticker API.
3.  Parses JSON and extracts:
    *   Current Price
    *   Previous Close
    *   Percent Change
4.  Updates:
    *   LCD screen
    *   LED indicators
    *   Internal dashboard data
5.  Serves a local web dashboard for real-time monitoring.

- - -

## 🌐 Web Dashboard

The built-in dashboard shows:

*   Current BTC price
*   Previous price
*   % change
*   Live chart with auto-scrolling price history

Dashboard auto-refreshes using JavaScript.

- - -

## 🔧 Setup

### 1\. Install Libraries

*   **ArduinoJSON**
*   **LiquidCrystal\_I2C**
*   **WiFi / WiFiClientSecure** (comes with ESP boards)

### 2\. Update WiFi Credentials

``` const char\* ssid = "YOUR\_WIFI"; const char\* password = "YOUR\_PASSWORD"; ```

### 3\. Flash to ESP8266/ESP32

*   Select correct board
*   Set upload speed to 115200
*   Flash the sketch

- - -

## 🔍 API Used

**Binance 24hr Price Ticker**

`https://api.binance.com/api/v3/ticker/24hr?symbol=BTCUSDT`

- - -

## 🖥️ Screenshot (Optional)

Add images here later…

- - -

## ✨ Future Work

*   🌐 Built-in Web Dashboard served by ESP8266/ESP32
*   📈 Live chart (Chart.js)
*   💱 Multiple coin tracking: Allow users to select and monitor multiple cryptocurrencies (BTC, ETH, LTC, etc.) directly from the web dashboard
*   🔔 Custom alerts per coin: Set up price thresholds for different coins
*   📊 Historical price charts for each coin
*   🎵 Fun Gilfoyle Napalm Death sound alert using DFPlayer Mini when a coin hits a price hike threshold

- - -

## 🤝 Contributing

Pull requests are welcome! If you'd like to improve hardware diagrams, code, or documentation—feel free!

- - -

## 📜 License

This project is open-source under the MIT License.

- - -

## ⚡ Built by Tawsif Torabi ❤️