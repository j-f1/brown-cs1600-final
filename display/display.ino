#include <LiquidCrystal.h>

const int rs = 0, en = 1, d4 = 2, d5 = 3, d6 = 4, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define bufcap 256
char buf[bufcap];
int buflen;
int cursor;
int writepos;

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
    display();
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
    display();
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
    Serial1.begin(9600);
    memset(buf, 0, bufcap);
    memcpy(buf, "food11\0food22\0food33\0", 21);
    buflen = 21;
    cursor = 0;
    writepos = 0;
    lcd.begin(ncols, nrows);

    attachInterrupt(digitalPinToInterrupt(leftButton), onLeft, RISING);
    attachInterrupt(digitalPinToInterrupt(acceptButton), onAccept, RISING);
    attachInterrupt(digitalPinToInterrupt(rightButton), onRight, RISING);
}

void display() {
    lcd.clear();
    
    lcd.setCursor(0, 0);
    for (int i = 0; i < min(ncols, buflen-cursor); i++) {
        if (buf[cursor+i] == '\0') {
            lcd.print('|');
        } else {
            lcd.print(buf[cursor+i]);
        }
    }

    lcd.setCursor(0, 1);
    int i = 0;
    while (buf[cursor+i] != '\0') {
        lcd.print('_');
        i++;
    }
    while (i < ncols) {
        lcd.print(' ');
        i++;
    }
}

void loop() {
    if (Serial1.available() > 0) {
        static byte incoming;
        incoming = Serial1.read();
        Serial.println((char) incoming); // Debugging
        if (incoming == '\n') {
            buflen = 0;
            cursor = 0;
            writepos = 0;
        } else if (writepos < bufcap) {
            buf[writepos++] = incoming;
            if (incoming == '\0') {
                buflen = writepos;
                display();
            }
        }
    }
}
