/*************************************************
 * JWSD ESP8266 MULTI PANEL P10 FINAL
 * ESP-12F + RTC DS3231 + HUB12
 *************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include <PrayerTimes.h>
#include <DMDESP.h>
#include <fonts/SystemFont5x7.h>

// ================= PANEL =================
#define PANELS_X 2
#define PANELS_Y 1

#define PIN_R   13
#define PIN_G   12
#define PIN_A   5
#define PIN_B   4
#define PIN_C   0
#define PIN_D   2
#define PIN_CLK 14
#define PIN_LAT 15
#define PIN_OE  16

#define BUZZER_PIN 3

DMDESP dmd(PANELS_X, PANELS_Y);

// ================= WIFI =================
ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600);

// ================= RTC =================
RTC_DS3231 rtc;

// ================= DATA =================
double latitude = -6.2;
double longitude = 106.8;
int timezone = 7;

int iqomahSubuh = 10;
int iqomahDzuhur = 10;
int iqomahAshar = 10;
int iqomahMaghrib = 5;
int iqomahIsya = 10;

String runningText = "SELAMAT DATANG DI MASJID";

PrayerTimes prayer;
double times[sizeof(TimeName)/sizeof(char*)];

// ================= STATE =================
bool iqomahActive = false;
int iqomahSeconds = 0;
String iqomahName;

// ================= UTIL =================
void buzzerBeep(int ms) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(ms);
  digitalWrite(BUZZER_PIN, LOW);
}

// ================= DISPLAY =================
void showText(String text) {
  dmd.clearScreen();
  dmd.drawText(0, 0, text);
}

void scrollText(String text) {
  for (int i = 0; i < text.length() * 6 + 64; i++) {
    dmd.clearScreen();
    dmd.drawText(64 - i, 0, text);
    delay(30);
  }
}

// ================= SHOLAT =================
void calculatePrayer() {
  prayer.setMethod(MWL);
  prayer.setLatitude(latitude);
  prayer.setLongitude(longitude);
  prayer.setTimeZone(timezone);

  DateTime now = rtc.now();
  prayer.getPrayerTimes(now.year(), now.month(), now.day(), times);
}

void checkIqomah() {
  DateTime now = rtc.now();
  int h = now.hour();
  int m = now.minute();

  struct {
    const char* name;
    int hour;
    int minute;
    int iqomah;
  } sholat[] = {
    {"SUBUH",  (int)times[0], (int)((times[0] - (int)times[0]) * 60), iqomahSubuh},
    {"DZUHUR", (int)times[2], (int)((times[2] - (int)times[2]) * 60), iqomahDzuhur},
    {"ASHAR",  (int)times[3], (int)((times[3] - (int)times[3]) * 60), iqomahAshar},
    {"MAGHRIB",(int)times[5], (int)((times[5] - (int)times[5]) * 60), iqomahMaghrib},
    {"ISYA",   (int)times[6], (int)((times[6] - (int)times[6]) * 60), iqomahIsya}
  };

  for (auto &s : sholat) {
    if (h == s.hour && m == s.minute && !iqomahActive) {
      iqomahActive = true;
      iqomahSeconds = s.iqomah * 60;
      iqomahName = s.name;
      buzzerBeep(2000);
    }
  }
}

// ================= WEB =================
void handleRoot() {
  server.send(200, "text/plain", "JWSD RUNNING");
}

void setupWeb() {
  server.on("/", handleRoot);
  server.begin();
}

// ================= SETUP =================
void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.begin(115200);
  Wire.begin(4, 5);

  rtc.begin();
  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  WiFi.begin("JWSD", "12345678");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  timeClient.begin();
  timeClient.update();
  rtc.adjust(DateTime(timeClient.getEpochTime()));

  setupWeb();

  dmd.start();
  dmd.setBrightness(255);

  calculatePrayer();
}

// ================= LOOP =================
void loop() {
  server.handleClient();

  if (iqomahActive) {
    dmd.clearScreen();
    dmd.drawText(0, 0, "IQOMAH");
    dmd.drawText(0, 8, iqomahName);
    delay(1000);
    iqomahSeconds--;

    if (iqomahSeconds <= 0) {
      iqomahActive = false;
      buzzerBeep(3000);
    }
  } else {
    DateTime now = rtc.now();
    char buf[10];
    sprintf(buf, "%02d:%02d", now.hour(), now.minute());
    showText(buf);
    delay(2000);
    scrollText(runningText);
    checkIqomah();
  }
}
