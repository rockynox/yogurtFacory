#include <Wire.h>
#include <RtcDS3231.h>

#include <HardwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>


//---- WIFI SETUP ----
const char *ssid = "Pierre_qui_roule";
const char *password = "n amasse pas mousse";

ESP8266WebServer server(80);

// ---- HARDWARE SETUP ----
int RELAY = D3;
int COOKING_LED_RED = D5;
int COOKING_LED_GREEN = D6;
int COOKING_LED_BLUE = D7;
int COOKING_BUTTON = D8;

int relayInitPosition = LOW;
int relayCookingPosition = HIGH;

// ---- NOTIFICATION ----
String registeredPhones = "[{\"identifier\":\"33659708146\"}]"


                          //---- CONST SETUP ----
                          unsigned long ledPeriode = 3000;
int cookingTimeInSeconds = 1 * 60;
bool cookingInProgress;
RtcDateTime startCookingDate;

void setup() {
  Serial.begin(57600);

  setupTimeKeeper();

  setupWifiServer();

  pinMode(RELAY, OUTPUT);
  pinMode(COOKING_LED_RED, OUTPUT);
  pinMode(COOKING_LED_GREEN, OUTPUT);
  pinMode(COOKING_LED_BLUE, OUTPUT);
  pinMode(COOKING_BUTTON, INPUT);

  digitalWrite(RELAY, relayInitPosition);
  cookingInProgress = false;

  blinkHello();
}

void loop() {
  bool cookingButtonPressed = digitalRead(COOKING_BUTTON);

  if (cookingInProgress && (getRemainingCookingTime() < 0 || cookingButtonPressed)) {
    sendNotification("Cooking stopped !");
    stopCooking();
  }
  else if (cookingButtonPressed) {
    sendNotification("Cooking started !");
    startCooking();
    waitInMillisecond(600);
  }
  setLedIndicator();
  server.handleClient();

}

void sendNotification(String text) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://api.catapush.com/1/messages", "AD 09 14 55 6D 10 BE 36 F7 C9 42 57 03 08 35 CE BB 39 79 7C");
    http.addHeader("accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("authorization", "Bearer 9fd0eb18af3cded40646cf28bf52b7ab076ae5ea");
    int responseCode = http.POST("{\"mobileAppId\":318,\"text\":\"" + text + "\",\"recipients\":" + registeredPhones + "}");
    String responsePayload = http.getString();

    Serial.println(responseCode);
    Serial.println(responsePayload);
    http.end();
  } else {
    Serial.println("No WiFi connection");
  }
}

void startCooking() {
  RtcDateTime now = getTime();
  startCooking(now);
}

void startCooking(RtcDateTime now) {
  cookingInProgress = true;
  startCookingDate = now;
  digitalWrite(RELAY, relayCookingPosition);
  Serial.println("Started Cooking !");
}

void stopCooking() {
  cookingInProgress = false;
  digitalWrite(RELAY, relayInitPosition);
  blinkCookingLedInRed();
  Serial.println("Stopped Cooking !");
}

int getRemainingCookingTime() {
  RtcDateTime now = getTime();
  int nowInSeconds = getUnixTime(now);
  int startCookingDateInSeconds = getUnixTime(startCookingDate);
  return cookingTimeInSeconds - (nowInSeconds - startCookingDateInSeconds);
}

void setLedIndicator() {
  if (cookingInProgress) {
    unsigned long now = millis();
    int blueValue = 128 + 127 * cos(2 * PI / ledPeriode * now);
    setCookingLedColor(0, 0, blueValue);
  }
  else {
    setCookingLedColor(0, 255, 0);
  }
}

void blinkCookingLedInRed() {
  for (int i = 0; i < 3; i += 1) {
    setCookingLedColor(255, 0, 0);
    waitInMillisecond(500);
    setCookingLedColor(0, 0, 0);
    waitInMillisecond(500);
  }
}

void blinkHello() {
  int blinkSpeed = 300;
  setCookingLedColor(255, 0, 0);
  waitInMillisecond(blinkSpeed);
  setCookingLedColor(255, 200, 0);
  waitInMillisecond(blinkSpeed);
  setCookingLedColor(0, 255, 0);
  waitInMillisecond(blinkSpeed);
  setCookingLedColor(0, 255, 200);
  waitInMillisecond(blinkSpeed);
  setCookingLedColor(0, 0, 255);
  waitInMillisecond(blinkSpeed);
  setCookingLedColor(200, 0, 255);
  waitInMillisecond(blinkSpeed);
  setCookingLedColor(255, 255, 255);
  waitInMillisecond(blinkSpeed);
  setCookingLedColor(0, 0, 0);
}

void setCookingLedColor(unsigned int red, unsigned int green, unsigned int blue) {
  analogWrite(COOKING_LED_RED, red);
  analogWrite(COOKING_LED_GREEN, green);
  analogWrite(COOKING_LED_BLUE, blue);
}

void waitInMillisecond(int millisecondToWait) {
  delay(millisecondToWait);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
RtcDS3231 <TwoWire> Rtc(Wire);

void setupTimeKeeper() {
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Wire.begin(D1, D2); //(SDA, SCL)
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDate(compiled);

  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
      // we have a communications error
      // see https://www.arduino.cc/en/Reference/WireEndTransmission for
      // what the number means
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else {
      // Common Cuases:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      Serial.println("RTC lost confidence in the DateTime!");

      // following line sets the RTC to the date & time this sketch was compiled
      // it will also reset the valid flag internally unless the Rtc device is
      // having an issue

      Rtc.SetDateTime(compiled);
    }
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
}

RtcDateTime getTime() {
  if (!Rtc.IsDateTimeValid()) {
    if (Rtc.LastError() != 0) {
      // we have a communications error
      // see https://www.arduino.cc/en/Reference/WireEndTransmission for
      // what the number means
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else {
      // Common Cuases:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }
  RtcDateTime now = Rtc.GetDateTime();
  return now;
}

void printDate(const RtcDateTime &dt) {
  char datestring[20];

  snprintf_P(datestring, countof(datestring), PSTR("%02u/%02u/%04u %02u:%02u:%02u"), dt.Month(), dt.Day(), dt.Year(),
             dt.Hour(), dt.Minute(), dt.Second());
  Serial.println(datestring);
}

int getUnixTime(RtcDateTime date) {
  return ((((((date.Year() - 1970)) * 12 + date.Month()) * 30 + date.Day()) * 24 + date.Hour()) * 60 +
          date.Minute()) * 60 + date.Second();
}

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
  blinkHello();
}

void setupWifiServer() {
  WiFi.begin(ssid, password);
  int maxNumberOfConnectionTry = 10;
  int numberOfConnectionTry = 0;

  while (WiFi.status() != WL_CONNECTED) {
    if (numberOfConnectionTry > maxNumberOfConnectionTry) {
      break;
    }
    numberOfConnectionTry++;
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/cook", []() {
    server.send(200, "text/plain", "Starting cooking !");
    startCooking();
  });

  server.on("/stop", []() {
    server.send(200, "text/plain", "stopping cooking");
    stopCooking();
  });

  server.on("/status", handleStatus);

  server.begin();
  Serial.println("HTTP server started");

}

void handleStatus() {
  String statusMessage = "";
  if (cookingInProgress) {
    statusMessage = "Currently cooking... - Remaining time:" + String(getRemainingCookingTime());
  }
  else {
    statusMessage = "Not cooking right now.";
  }

  server.send(200, "text/plain", statusMessage);
}

