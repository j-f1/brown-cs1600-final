#include <LiquidCrystal.h>

const int rs = 0, en = 1, d4 = 2, d5 = 3, d6 = 4, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

String words;
int cursor;

#define ncols 16
#define nrows 2

#define prevButton 8
#define acceptButton 7
#define nextButton 6

#define COMPLETION_SEP ' '
#define END_OF_COMPLETIONS '\0'

void onPrev() {
    Serial.println("prev");
    if (cursor > 0) {
        do {
            cursor--;
        } while (cursor > 0 && words.charAt(cursor-1) != COMPLETION_SEP);
    }
    display();
}

void onNext() {
    Serial.println("next");
    do {
        Serial.println(words.charAt(cursor));
        cursor++;
    } while (words.charAt(cursor-1) != COMPLETION_SEP);
    if (cursor >= words.length()) {
        cursor = 0;
    }
    display();
}

void onAccept() {
    Serial.println("accept");
    if (words.length() > 0) {
        Serial.println(words.substring(cursor));
        Serial1.print(words.substring(cursor));
        Serial1.write(END_OF_COMPLETIONS); // TODO: necessary?
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial1.begin(9600);
    cursor = 0;
    lcd.begin(ncols, nrows);

    attachInterrupt(digitalPinToInterrupt(prevButton), onPrev, RISING);
    attachInterrupt(digitalPinToInterrupt(acceptButton), onAccept, RISING);
    attachInterrupt(digitalPinToInterrupt(nextButton), onNext, RISING);
}

void display() {
    lcd.clear();
    
    lcd.setCursor(0, 0);
    for (int i = 0; i < min(ncols, words.length()-cursor); i++) {
        if (words.charAt(cursor+i) == COMPLETION_SEP) {
            lcd.print('|');
        } else {
            lcd.print(words.charAt(cursor+i));
        }
    }

    lcd.setCursor(0, 1);
    int i = 0;
    while (words.charAt(cursor+i) != COMPLETION_SEP) {
        lcd.print('_');
        i++;
    }
    while (i < ncols) {
        lcd.print(COMPLETION_SEP);
        i++;
    }
}

void loop() {
    if (Serial1.available() > 0) {
        words = Serial1.readStringUntil(END_OF_COMPLETIONS);
        cursor = 0;
        display();
    }
}
