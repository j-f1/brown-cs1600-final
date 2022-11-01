int rows[] = {0, 1};
#define nrows 2
int cols[] = {2};
#define ncols 1

char keymap[nrows][ncols] = {
  {'b'},
  {'z'}
};

void setup() {
  for (int i = 0; i < nrows; i++) {
    pinMode(rows[i], INPUT);
  }
  for (int i = 0; i < ncols; i++) {
    pinMode(cols[i], INPUT);
  }
}

void loop() {
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
        row = i;
      }
    }
  }

  if (col != -1 && row != -1) {
    Serial.print("Detected key press at ");
    Serial.print(col);
    Serial.print(", ");
    Serial.print(row);
    Serial.print(": ");
    Serial.print(keymap[row][col]);
  }
}
