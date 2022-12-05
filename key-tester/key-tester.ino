#include <Keyboard.h>

int rows[] = {0, 1};
#define nrows 2
int cols[] = {7,6};
#define ncols 2

//6, 4 and 5? which pin isnt being used for keyboard
int rPin = 4; 
int gPin = 5;
int bPin = 6;
int pwmVal = 5;
int dir = 1;


String input = "";




char keymap[nrows][ncols] = {
  {'b', 'm'},
  {'z', 'j'},
};

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Keyboard.begin();
  for (int i = 0; i < nrows; i++) {
    pinMode(rows[i], INPUT);
    attachInterrupt(digitalPinToInterrupt(rows[i]), onKeypress, RISING);
  }
  for (int i = 0; i < ncols; i++) {
    pinMode(cols[i], INPUT);
  }
  pinMode(statusPin, OUTPUT);
  pinMode(successPin, OUTPUT);
}

void onKeypress() {
  int row = -1;
  for (int i = 0; i < nrows; i++) {
    if (digitalRead(rows[i]) == HIGH) {
      if (row != -1) {
        Serial.print("Detected multiple rows pressed! ");
        Serial.print(row);
        Serial.print(" and ");
        Serial.println(i);
      } else {
        row = i;
      }
    }
  }

  int col = -1;
  for (int i = 0; i < ncols; i++) {
    if (digitalRead(cols[i]) == HIGH) {
      if (col != -1) {
        Serial.print("Detected multiple cols pressed! ");
        Serial.print(col);
        Serial.print(" and ");
        Serial.println(i);
      } else {
        col = i;
      }
    }
  }
  if (row != -1 && col != -1) {
    Serial.print("Detected key press at ");
    Serial.print(col);
    Serial.print(", ");
    Serial.print(row);
    Serial.print(": ");
    Serial.print(keymap[row][col]);
    Serial.println();
    Keyboard.print(keymap[row][col]);
    input = String(input+keymap[row][col]);
  }
}

void loop() {}

void signalIndicator() {
  pwmVal = 5;
  while(!sending) {
    analogWrite(rPin, pwmVal);
    analogWrite(gPin, pwmVal);
    analogWrite(bPin, 0);
    pwmVal += 10 * dir;
    if (pwmVal == 255) {
      dir = -dir;
    }
    delay(100);
  }
  analogWrite(rPin, 0);
  analogWrite(gPin, 0);
  analogWrite(bPin, 0);
  pwmVal = 5;
  dir = 1;
  while(sending && pwmVal < 255) {
    analogWrite(gPin, pwmVal);
    pwmVal += 5 * dir;
    delay(100);
  }
  if (success) {
    analogWrite(gPin, 255);
    analogWrite(bPin, 0);
    analogWrite(rPin, 0);
  } else {
    analogWrite(rPin, 255);
    analogWrite(bPin, 0);
    analogWrite(gPin, 0);
  }
}
