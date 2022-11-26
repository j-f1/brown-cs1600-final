#include <LiquidCrystal.h>

const int rs = 0, en = 1, d4 = 2, d5 = 3, d6 = 4, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define bufcap 256
char buf[bufcap];
int buflen;
int cursor;

#define ncols 16
#define nrows 2

#define leftButton 8
#define acceptButton 7
#define rightButton 6

void onLeft() {
    Serial.println("left");
    if (cursor > 0) {
        do {
            cursor--;
        } while (cursor > 0 && buf[cursor-1] != '\0');
    }
    display(buf+cursor, buflen-cursor);
}

void onRight() {
    Serial.println("right");
    do {
        Serial.println(buf[cursor]);
        cursor++;
    } while (buf[cursor-1] != '\0');
    if (cursor >= buflen) {
        cursor = 0;
    }
    display(buf+cursor, buflen-cursor);
}

void onAccept() {
    Serial.println("accept");
    acceptCandidate(&buf[cursor]);
}

void acceptCandidate(char *candidate) {
  
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    memset(buf, 0, bufcap);
    memcpy(buf, "food11\0food22\0food33\0", 21);
    Serial.println(buf);
    buflen = 21;
    cursor = 0;
    lcd.begin(ncols, nrows);

    attachInterrupt(digitalPinToInterrupt(leftButton), onLeft, RISING);
    attachInterrupt(digitalPinToInterrupt(acceptButton), onAccept, RISING);
    attachInterrupt(digitalPinToInterrupt(rightButton), onRight, RISING);
}

void display(char buf[], int buflen) {
    lcd.clear();
    lcd.setCursor(0, 0);
    for (int i = 0; i < ncols && i < buflen; i += strlen(&buf[i])+1) {
        Serial.println(&buf[i]);
        lcd.print(&buf[i]);
        lcd.print("|");
    }

    lcd.setCursor(0, 1);
    int i = 0;
    while (buf[i] != '\0') {
        lcd.print('_');
        i++;
    }
    while (i < ncols) {
        lcd.print(' ');
        i++;
    }
}

void loop() {
    display(buf, buflen);
    delay(1000000);
}
