#include <Wire.h>
#include <RtcDS3231.h>

#include <HardwareSerial.h>

int RELAY = D3;
int COOKING_LED_RED = D5;
int COOKING_LED_GREEN = D6;
int COOKING_LED_BLUE = D7;
int COOKING_BUTTON = D8;

int relayInitPosition = LOW;
int relayCookingPosition = HIGH;

//int currentCookingLedColor[3] = {0, 0, 0};
int cookingTimeInSeconds = 1 * 60;
bool cookingInProgress;
RtcDateTime startCookingDate;

void setup() {
    Serial.begin(57600);

    setupTimeKeeper();

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
    RtcDateTime now = getTime();
    if (cookingInProgress) {
        printDate(now);
        bool cookingButtonPressed = digitalRead(COOKING_BUTTON);
        if (isCookingOver(now) || cookingButtonPressed) {
            stopCoooking();
        }
    }
    else {
        bool cookingButtonPressed = digitalRead(COOKING_BUTTON);
        if (cookingButtonPressed) {
            startCooking(now);
            waitInMillisecond(600);
        }
    }
    setLedIndicator();
}

void startCooking(RtcDateTime now) {
    cookingInProgress = true;
    startCookingDate = now;
    digitalWrite(RELAY, relayCookingPosition);
    Serial.println("Started Cooking !");
}

void stopCoooking() {
    cookingInProgress = false;
    digitalWrite(RELAY, relayInitPosition);
    blinkCookingLedInRed();
    Serial.println("Stopped Cooking !");
}

bool isCookingOver(RtcDateTime now) {
    int nowInSeconds = getUnixTime(now);
    int startCookingDateInSeconds = getUnixTime(startCookingDate);
    return (nowInSeconds - startCookingDateInSeconds) > cookingTimeInSeconds;
}

void setLedIndicator() {
    if (cookingInProgress) {
        setCookingLedColor(0, 0, 255);
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
    int blinkSpeed = 200;
    setCookingLedColor(255, 38, 0);
    waitInMillisecond(blinkSpeed);
    setCookingLedColor(255, 147, 0);
    waitInMillisecond(blinkSpeed);
    setCookingLedColor(255, 251, 0);
    waitInMillisecond(blinkSpeed);
    setCookingLedColor(142, 250, 0);
    waitInMillisecond(blinkSpeed);
    setCookingLedColor(0, 250, 146);
    waitInMillisecond(blinkSpeed);
    setCookingLedColor(0, 253, 255);
    waitInMillisecond(blinkSpeed);
    setCookingLedColor(4, 51, 255);
    waitInMillisecond(blinkSpeed);
    setCookingLedColor(255, 64, 255);
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

