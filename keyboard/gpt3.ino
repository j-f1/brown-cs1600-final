#include <SPI.h>
#include <Keyboard.h>
#include "shared.h"

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial);
  Keyboard.begin();

  setup_wifi();

  char result[200];
  Serial.setTimeout(10e3);
  while (true) {
    String prompt = Serial.readStringUntil('\n');
    if (prompt.length() && make_request(prompt.c_str(), result, 200)) {
      Keyboard.print(result);
    }
  }
}

void loop() {
}
