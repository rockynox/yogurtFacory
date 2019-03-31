#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

#include <HardwareSerial.h>
int RELAY = D3;
int COOKING_BUTTON = D1;
int STOP_COOKING_BUTTON = D2;
const byte COOKING_LED_RED = D5;
const byte COOKING_LED_GREEN = D6;


const byte COOKING_LED_BLUE = D7;
int relayInitPosition = LOW;

int relayCookingPosition = HIGH;

int currentCookingLedColor[3] = {0, 0, 0};
int initialCookingTimeInMilliseconds = 10 * 60 * 1000;
bool cookingInProgress;

int remainingCookingTimeInMilliseconds;

void setup() {

    setupTimeKeeper();
    Serial.begin(57600);

    pinMode(RELAY, OUTPUT);
    pinMode(COOKING_LED_RED, OUTPUT);
    pinMode(COOKING_LED_GREEN, OUTPUT);
    pinMode(COOKING_LED_BLUE, OUTPUT);
    pinMode(COOKING_BUTTON, INPUT);

    digitalWrite(RELAY, relayInitPosition);
    cookingInProgress = false;
    remainingCookingTimeInMilliseconds = 0;

    setCookingLedColor(0, 0, 0);
}

void loop() {
    if (cookingInProgress) {
        keepCooking();
        bool stopCookingButtonPressed = digitalRead(STOP_COOKING_BUTTON);
        if (remainingCookingTimeInMilliseconds < 0 || stopCookingButtonPressed) {
            stopCoooking();
        }
    }
    else {
        bool cookingButtonPressed = digitalRead(COOKING_BUTTON);
        if (cookingButtonPressed) {
            startCooking();
        }
    }
    setLedIndicator();
}

void startCooking() {
    cookingInProgress = true;
    remainingCookingTimeInMilliseconds = initialCookingTimeInMilliseconds;
    digitalWrite(RELAY, relayCookingPosition);
    Serial.println("Started Cooking !");
}

void keepCooking() {
    waitInMillisecond(1);
    remainingCookingTimeInMilliseconds--;
}
void stopCoooking() {
    cookingInProgress = false;
    digitalWrite(RELAY, relayInitPosition);
    blinkCookingLedInRed();
    Serial.println("Stopped Cooking !");
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

/* for normal hardware wire use above */

void setupTimeKeeper() {
    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);

    //--------RTC SETUP ------------
    // if you are using ESP-01 then uncomment the line below to reset the pins to
    // the available pins for SDA, SCL
    // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDate(compiled);
    Serial.println();

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

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
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
    printDate(now);

    return now;
}

void printDate(const RtcDateTime &dt) {
    char datestring[20];

    snprintf_P(datestring, countof(datestring), PSTR("%02u/%02u/%04u %02u:%02u:%02u"), dt.Month(), dt.Day(), dt.Year(),
               dt.Hour(), dt.Minute(), dt.Second());
    Serial.println(datestring);
}

