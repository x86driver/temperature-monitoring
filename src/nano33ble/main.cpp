#include <Arduino.h>
#include <RadioLib.h>
#include <Arduino_HS300x.h>
#include <RtcDS1302.h>

ThreeWire myWire(A4,A5,A2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
SX1278 radio = new Module(5, 7, 6, RADIOLIB_NC);

#define countof(a) (sizeof(a) / sizeof(a[0]))
#define DATETIME_LENGTH 26

void printDateTime(const RtcDateTime& dt)
{
  char datestring[DATETIME_LENGTH];

  snprintf_P(datestring, 
          countof(datestring),
          PSTR("%04u/%02u/%02u %02u:%02u:%02u"),
          dt.Year(),
          dt.Month(),
          dt.Day(),
          dt.Hour(),
          dt.Minute(),
          dt.Second() );
  Serial.print(datestring);
}

void getDateTime(const RtcDateTime& dt, char* datestring)
{
  if (datestring == NULL)
    return;
  snprintf_P(datestring,
          DATETIME_LENGTH,
          PSTR("%04u-%02u-%02u %02u:%02u:%02u"),
          dt.Year(),
          dt.Month(),
          dt.Day(),
          dt.Hour(),
          dt.Minute(),
          dt.Second() );
}

void setup_rtc() {
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid())
  {
      // Common Causes:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected())
  {
      Serial.println("RTC was write protected, enabling writing now");
      Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning())
  {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
      Serial.println("RTC is older than compile time!  (Updating DateTime)");
      Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
      Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
      Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }

  setup_rtc();

  while (true) {
    // initialize SX1278 with default settings
    Serial.print(F("[SX1278] Initializing ... "));
    int state = radio.begin();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
      break;
    } else {
      Serial.print(F("failed, code "));
      Serial.println(state);
      delay(1000);
    }
  }
}

void loop () {
  RtcDateTime now = Rtc.GetDateTime();

  char datestring[DATETIME_LENGTH] = {0};
  getDateTime(now, datestring);

  if (!now.IsValid())
  {
      // Common Causes:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
  }

  float temperature = HS300x.readTemperature();
  float humidity    = HS300x.readHumidity();

  Serial.print(F("[SX1278] Transmitting packet ... "));

  // you can transmit C-string or Arduino string up to
  // 255 characters long
  String str = "DateTime = " + String(datestring) +  ", Temperature = " + String(temperature) + " °C, Humidity = " + String(humidity) + " %";
  int state = radio.transmit(str);

  Serial.print(str);
  // you can also transmit byte array up to 256 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    int state = radio.transmit(byteArr, 8);
  */

  if (state == RADIOLIB_ERR_NONE) {
    // the packet was successfully transmitted
    Serial.println(F(" success!"));

    // print measured data rate
    Serial.print(F("[SX1278] Datarate:\t"));
    Serial.print(radio.getDataRate());
    Serial.println(F(" bps"));

  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));

  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    // timeout occurred while transmitting packet
    Serial.println(F("timeout!"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }

  // wait for a second before transmitting again
  delay(1000);
}
