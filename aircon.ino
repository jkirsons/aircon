/* Copyright 2019 David Conran
*
* An IR LED circuit *MUST* be connected to the ESP8266 on a pin
* as specified by kIrLed below.
*
* TL;DR: The IR LED needs to be driven by a transistor for a good result.
*
* Suggested circuit:
*     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
*
* Common mistakes & tips:
*   * Don't just connect the IR LED directly to the pin, it won't
*     have enough current to drive the IR LED effectively.
*   * Make sure you have the IR LED polarity correct.
*     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
*   * Typical digital camera/phones can be used to see if the IR LED is flashed.
*     Replace the IR LED with a normal LED if you don't have a digital camera
*     when debugging.
*   * Avoid using the following pins unless you really know what you are doing:
*     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
*     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
*     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
*   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
*     for your first time. e.g. ESP-12 etc.
*/
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>

#include <IRsend.h>
#include <ir_MitsubishiHeavy.h>

const uint16_t kIrLed = 12;         // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRMitsubishiHeavy152Ac ac(kIrLed);  // Set the GPIO used for sending messages.

const char *ssid = "wifi_id";
const char *password = "password";
WiFiMulti WiFiMulti;
HTTPClient http;

void printState() {
  // Display the settings.
  Serial.println("Mitsubishi Heavy A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char *ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kMitsubishiHeavy152StateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

void setup() {
  M5.begin(true, false, true);

  WiFiMulti.addAP(
    ssid,
    password);
  Serial.print(
    "\nWaiting connect to WiFi...");
  M5.dis.fillpix(0xff0000);
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  M5.dis.fillpix(0x00ff00);
  Serial.println(WiFi.localIP());
  delay(500);

  ac.begin();
  Serial.begin(115200);
  delay(200);

  // Set up what we want to send. See ir_MitsubishiHeavy.(cpp|h) for all the
  // options.
  Serial.println("Default state of the remote.");
  printState();
  Serial.println("Setting desired state for A/C.");
  ac.setPower(true);                                       // Turn it on.
  ac.setFan(kMitsubishiHeavy152FanMed);                    // Medium Fan
  ac.setMode(kMitsubishiHeavyCool);                        // Cool mode
  ac.setTemp(26);                                          // Celsius
  ac.setSwingVertical(kMitsubishiHeavy152SwingVAuto);      // Swing vertically
  ac.setSwingHorizontal(kMitsubishiHeavy152SwingHMiddle);  // Swing Horizontally
}

void loop() {

  const char *host = "http://ec2-54-197-16-12.compute-1.amazonaws.com:8000/index.html";
  http.begin(host);
  http.setTimeout(15000);
  int httpCode = http.GET();

  if (httpCode > 0) {  // httpCode will be negative on error.
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {  // file found at server.
      String payload = http.getString();
      Serial.println(payload);  // Print
                                //  files read on the server
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n",
                  http.errorToString(httpCode).c_str());
  }

  http.end();

  // Now send the IR signal.
  Serial.println("Sending IR command to A/C ...");
  ac.send();
  printState();
  delay(5000);
}
