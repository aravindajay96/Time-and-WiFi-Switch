#include <Wire.h>
#include "RTClib.h"
#include <WiFi.h>
#include <WebServer.h>

RTC_DS3231 rtc;
WebServer server(80);

// Access Point
const char* ssid = "ESP32_Relay";
//bool relayState = false;
bool manualOverride = false;

int lastH = 0;

// Relay pins
#define RELAY1 27
#define RELAY2 14

// Schedule
int ON_HOUR = 645;
int OFF_HOUR = 745;


bool relaysOn = false;

// ===== Web Page =====
void handleRoot() {

  DateTime now = rtc.now();
  int currentTime = now.hour() * 100 + now.minute();

 String html;
html.reserve(1500);
html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;}";
  html += "input{width:100px;padding:5px;font-size:16px;}";
  html += "button{padding:10px;margin:5px;font-size:16px;}";
  html += "</style>";
  html += "</head><body>";

  html += "<h2>ESP32 Relay Timer</h2>";

  html += "<p><b>Current Time:</b> ";
  html += String(currentTime);
  html += "</p>";

  html += "<p><b>Relay Status:</b> ";
  html += (relaysOn ? "ON" : "OFF");
  html += "</p>";

  html += "<a href='/on'><button>Manual ON</button></a>";
  html += "<a href='/off'><button>Manual OFF</button></a>";

  html += "<h3>Set Schedule (HHMM Format)</h3>";
  html += "<form action='/set'>";

  html += "ON Time: <br>";
  html += "<input name='onh' type='number' min='0' max='2359' value='" + String(ON_HOUR) + "'><br><br>";

  html += "OFF Time: <br>";
  html += "<input name='offh' type='number' min='0' max='2359' value='" + String(OFF_HOUR) + "'><br><br>";

  html += "<input type='submit' value='Save'>";
  html += "</form>";

  html += "<p>Example: 0830 = 8:30 AM | 2200 = 10:00 PM</p>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ===== Set Time From Web =====
void handleSetTime() {

  if (server.hasArg("onh")) {
    ON_HOUR = server.arg("onh").toInt();
  }

  if (server.hasArg("offh")) {
    OFF_HOUR = server.arg("offh").toInt();
  }

  handleRoot();
}


void setup() {
  Serial.begin(115200);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);

  // Relays OFF at start (ACTIVE LOW)
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);

  Wire.begin(21, 22);

  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  Serial.println("RTC Relay Timer Started");

  WiFi.softAP(ssid);
  Serial.println("WiFi Started");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/set", handleSetTime);
  server.on("/on", manualOn);
  server.on("/off", manualOff);

  server.begin();
}

void loop() {

  DateTime now = rtc.now();
  int h = now.hour()*100 + now.minute();

Serial.printf("Time: %02d:%02d:%02d\n",
                now.hour(), now.minute(), now.second());

  server.handleClient();
Serial.printf("ON: %d OFF: %d\n", ON_HOUR, OFF_HOUR);

 // ===== Relay Control Logic =====

bool inSchedule = (h >= ON_HOUR && h < OFF_HOUR);

// Detect crossing OFF_HOUR
if (lastH < OFF_HOUR && h >= OFF_HOUR) {
  manualOverride = false;
  turnRelaysOff();
}

if (manualOverride) {
  if (!relaysOn) turnRelaysOn();
}
else {
  if (inSchedule) {
    if (!relaysOn) turnRelaysOn();
  } else {
    if (relaysOn) turnRelaysOff();
  }
}

lastH = h;
Serial.println(ESP.getFreeHeap());
  delay(200);
}


void turnRelaysOn() {
  Serial.println("Relays ON");
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  relaysOn = true;
}

void turnRelaysOff() {
  Serial.println("Relays OFF");
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);
  relaysOn = false;
}

void manualOn() {
  manualOverride = true;
  turnRelaysOn();
  handleRoot();
}

void manualOff() {
  manualOverride = false;
  turnRelaysOff();
  handleRoot();
}
