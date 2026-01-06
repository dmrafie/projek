#include <WiFi.h>
#include <time.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// WiFi Credentials
const char* ssid     = "YOUR SSID HERE";
const char* password = "YOUR PASSWORD HERE";

// Hardware SPI for ESP32-C3 Super Mini
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // Most common 8x32 modules
#define MAX_DEVICES 4
#define CS_PIN      7  // Chip Select
#define DATA_PIN    6  // MOSI
#define CLK_PIN     4  // SCK

// Initialize Parola
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// NTP Settings for Indonesia
const char* ntpServer = "id.pool.ntp.org"; // Indonesian NTP Server
const long  gmtOffset_sec = 25200;          // WIB (UTC+7) = 7 * 3600
const int   daylightOffset_sec = 0;        // No DST in Indonesia

char timeString[9]; // HH:mm:ss
const int LED_PIN = 8;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  P.begin();
  P.setIntensity(0); // Brightness 0-15
  P.displayClear();
  P.displayText("WIFI...", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  P.displayAnimate();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    P.displayText("ERR", PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayAnimate();
    return;
  }

  // Blinking logic: if seconds are even, show ":", if odd, show " "
  bool showColon = (timeinfo.tm_sec % 2 == 0);

  if (showColon) {
    // Format with colon (HH:mm)
    strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
    digitalWrite(LED_PIN, LOW);
  } else {
    // Format with space instead of colon (HH mm)
    strftime(timeString, sizeof(timeString), "%H %M", &timeinfo);
    digitalWrite(LED_PIN, HIGH);
  }

  // Update the display
  if (P.displayAnimate()) {
    P.displayText(timeString, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
  }
}
