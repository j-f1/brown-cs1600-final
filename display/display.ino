#include <LiquidCrystal.h>

const int rs = 0, en = 1, d4 = 2, d5 = 3, d6 = 4, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

String words;
int cursor;

#define ncols 16
#define nrows 2

#define prevButton 9
#define acceptButton 8
#define nextButton 7

#define COMPLETION_SEP ' '
#define END_OF_COMPLETIONS '\0'

#define BUTTON_DEBOUNCE_MILLIS 250

void onPrev() {
    static int lastPressed = 0;
    int now = millis();
    if (now - lastPressed < BUTTON_DEBOUNCE_MILLIS) return;
    lastPressed = now;
    
    Serial.println("prev");
    if (cursor > 0) {
        do {
            cursor--;
        } while (cursor > 0 && words[cursor-1] != COMPLETION_SEP);
    }
    display();
}

void onNext() {
    static int lastPressed = 0;
    int now = millis();
    if (now - lastPressed < BUTTON_DEBOUNCE_MILLIS) return;
    lastPressed = now;
    
    Serial.println("next");
    do {
        cursor++;
    } while (cursor < words.length() && words[cursor-1] != COMPLETION_SEP);
    if (cursor >= words.length()) {
        cursor = 0;
    }
    display();
}

void onAccept() {
    static int lastPressed = 0;
    int now = millis();
    if (now - lastPressed < BUTTON_DEBOUNCE_MILLIS) return;
    lastPressed = now;
    
    Serial.println("accept");
    if (words.length() > 0) {
        int endCursor = cursor;
        while (endCursor < words.length() && words.charAt(endCursor) != COMPLETION_SEP) {
            endCursor++;
        }
        String word = words.substring(cursor, endCursor);
        Serial.print("Accepted suggestion: ");
        Serial.println(word);
        Serial1.print(word);
        Serial1.write(END_OF_COMPLETIONS);
    }
}

void setup() {
    Serial.begin(9600);
    Serial1.begin(300);
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
        if (words[cursor+i] == COMPLETION_SEP) {
            lcd.print('|');
        } else {
            lcd.print(words[cursor+i]);
        }
    }

    lcd.setCursor(0, 1);
    int i = 0;
    while (cursor+i < words.length() && words[cursor+i] != COMPLETION_SEP) {
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
        Serial.println(words);
        cursor = 0;
        display();
    }
}
