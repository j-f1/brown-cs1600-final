#include <Keyboard.h>

int rows[] = {0, 1};
#define nrows 2
int cols[] = {7};
#define ncols 1

char keymap[nrows][ncols] = {
  {'b'},
  {'z'},
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
}

void onKeypress() {
  Serial.println("press");
  int row = -1;
  for (int i = 0; i < nrows; i++) {
    if (digitalRead(rows[i]) == HIGH) {
      if (i == 1) {
      Serial.print("this is the j row: ");
      Serial.println(i);
      }
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

  int col = 0;
  for (int i = 0; i < ncols; i++) {
    if (digitalRead(cols[i]) == HIGH) {
      if (i == 1) {
      Serial.print("this is the j col: ");
      Serial.println(i);
      }
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
//should be outputting j
  if (row != -1 && col != -1) {
    Serial.print("Detected key press at ");
    Serial.print(col);
    Serial.print(", ");
    Serial.print(row);
    Serial.print(": ");
    Serial.print(keymap[row][col]);
    Serial.println();
    Keyboard.print(keymap[row][col]);
  }
}

void loop() {}
