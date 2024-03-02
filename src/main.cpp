#include <Arduino.h>
#include <RadioLib.h>
#include <Arduino_HS300x.h>

#if defined(ESP32)
SX1268 radio = new Module(5, 25, 16, -1);
#elif defined(ARDUINO_ARDUINO_NANO33BLE)
// Arduino Nano 33 BLE Sense Rev2
SX1278 radio = new Module(5, 7, 6, RADIOLIB_NC);
#else
#error "Please select a board!"
#endif

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!HS300x.begin()) {
    Serial.println("Failed to initialize humidity temperature sensor!");
    while (1);
  }

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

void loop_tx() {
  float temperature = HS300x.readTemperature();
  float humidity    = HS300x.readHumidity();

  Serial.print(F("[SX1278] Transmitting packet ... "));

  // you can transmit C-string or Arduino string up to
  // 255 characters long
  String str = "Temperature = " + String(temperature) + " °C, Humidity = " + String(humidity) + " %";
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

void loop_rx() {

}

void loop() {
  #if defined(ESP32)
    loop_rx();
  #elif defined(ARDUINO_ARDUINO_NANO33BLE)
    loop_tx();
  #endif
}
