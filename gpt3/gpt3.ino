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
  if (make_request("hll", result, 200)) {
    Keyboard.print(result);
  }
}

void loop() {
}
