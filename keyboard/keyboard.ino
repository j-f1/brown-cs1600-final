#include <Keyboard.h>

int rows[] = {1,2,3};
#define nrows 3
int cols[] = {4,5,6,7,8,9,0};
#define ncols 7

char keymap[nrows][ncols] = {
  {'p', 'l', 'k', 'j', 'h', 'g', 'f'},
  {'t', 'r', 'd', 's', 'w', 'q', 'a'},
  {'m', 'n', 'b', 'v', 'c', 'x', 'z'},
};

const int completionDelayMillis = 100;
int lastKeypressMillis;
bool completionRequested;

#define bufcap 256
char buf[bufcap];
int bufStart;
int bufLen;
int bufLenProcessed;

char gptResult[200];

int rPin = A3; 
int gPin = 11;
int bPin = A4;
int pwmVal = 5;
int dir = 1;

void setup() {
  Serial.begin(9600);
  Keyboard.begin();

  for (int i = 0; i < nrows; i++) {
    pinMode(rows[i], OUTPUT);
    digitalWrite(rows[i], HIGH);
  }
  for (int i = 0; i < ncols; i++) {
    pinMode(cols[i], INPUT);
    attachInterrupt(digitalPinToInterrupt(cols[i]), onKeypress, RISING);
  }
  
  setup_wifi();
  setupWDT();
  lastKeypressMillis = millis();
  completionRequested = true;
  memset(buf, 0, bufcap);
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);
}

void loop() {
  // led indicator pulse green while idle/standing by for request
  analogWrite(rPin, 0);
  analogWrite(gPin, pwmVal);
  analogWrite(bPin, 0);
  pwmVal += 10 * dir;
  if (pwmVal == 255 || pwmVal == 5) {
    dir = -dir;
  }
  
  // Send new keypresses to computer
  noInterrupts();
  while (bufLenProcessed < bufLen) {
    char c = buf[(bufStart + bufLenProcessed) % bufcap];
    Keyboard.print(c);
    completionRequested = false;
    bufLenProcessed++;
  }
  interrupts();

  // If it's been a second since the last keypress,
  // request completions from GPT-3
  if (!completionRequested && millis() - lastKeypressMillis > completionDelayMillis) {
    noInterrupts();
    static char word[bufcap];
    
    // Sending word to gpt3: led off + reset vals
    analogWrite(rPin, 255);
    analogWrite(gPin, 255);
    analogWrite(bPin, 0);
    pwmVal = 5;
    dir = 1;

    // Scan backwards to find the start of the last word
    int wordLen;
    for (wordLen = 0; wordLen < bufLen; wordLen++) {
      if (buf[(bufStart+bufLen-wordLen-1) % bufcap] == ' ') {
        break;
      }
    }
    for (int i = 0; i < wordLen; i++) {
      Serial.println(buf[(bufStart+bufLen-wordLen+i) % bufcap]);
      word[i] = buf[(bufStart+bufLen-wordLen+i) % bufcap];
    }
    word[wordLen] = '\0';

    Serial.println("processing word");
    Serial.println(wordLen);
    if (wordLen > 0) {
      Serial.println(word);
      if (make_request(word, gptResult, 200)) {
        Serial.println(gptResult);
        // Success led green
        analogWrite(gPin, 255);
        analogWrite(bPin, 0);
        analogWrite(rPin, 0);
      } else {
        Serial.println("failure");
  
        // Error led red
        analogWrite(rPin, 255);
        analogWrite(bPin, 0);
        analogWrite(gPin, 0);
      }
      delay(10000);
    }
    completionRequested = true;
    interrupts();
  }

  // Pet the watchdog
  WDT->CLEAR.reg = 0xA5;
}

void onKeypress() {
  noInterrupts();
  for (int row = 0; row < nrows; row++) {
    digitalWrite(rows[row], LOW);
  }
  int activeRow = -1;
  int activeCol = -1;
  bool err = false;
  for (int row = 0; row < nrows; row++) {
    digitalWrite(rows[row], HIGH);
    for (int col = 0; col < ncols; col++) {
      if (digitalRead(cols[col]) == HIGH) {
        if (activeRow == -1 && activeCol == -1) {
          activeRow = row;
          activeCol = col;
        } else {
          err = true;
        }
      }
    }
    digitalWrite(rows[row], LOW);
  }
  if (activeRow != -1 && activeCol != -1 && !err) {
    if (bufLen < bufcap && millis() - lastKeypressMillis > 5) {
      Serial.print("Detected key press at ");
      Serial.println(millis());
      buf[(bufStart + bufLen) % bufcap] = keymap[activeRow][activeCol];
      bufLen++;
      lastKeypressMillis = millis();
      completionRequested = false;
    } // TODO: how to handle full buffer
  }
  for (int row = 0; row < nrows; row++) {
    digitalWrite(rows[row], HIGH);
  }
  interrupts();
}

// Set up a 4s watchdog timer
// Adapter from lab4
void setupWDT() {
  // Clear and enable WDT
  NVIC_DisableIRQ(WDT_IRQn);
  NVIC_ClearPendingIRQ(WDT_IRQn);
  NVIC_SetPriority(WDT_IRQn, 0);
  NVIC_EnableIRQ(WDT_IRQn);

  // Configure and enable WDT GCLK:
  GCLK->GENDIV.reg = GCLK_GENDIV_DIV(4) | GCLK_GENDIV_ID(5);
  while (GCLK->STATUS.bit.SYNCBUSY);
  // set GCLK->GENCTRL.reg and GCLK->CLKCTRL.reg
  GCLK->GENCTRL.reg = GCLK_GENCTRL_DIVSEL | GCLK_GENCTRL_ID(5) | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC(3);
  while (GCLK->STATUS.bit.SYNCBUSY);
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN(5) | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID(3);

  // Configure and enable WDT:
  // use WDT->CONFIG.reg, WDT->EWCTRL.reg, WDT->CTRL.reg
  WDT->EWCTRL.reg = WDT_EWCTRL_EWOFFSET(8);
  WDT->CONFIG.reg = WDT_CONFIG_PER(9);
  WDT->CTRL.reg = WDT_CTRL_ENABLE;
  while (WDT->STATUS.bit.SYNCBUSY);

  // Enable early warning interrupts on WDT:
  // reference WDT registers with WDT->register_name.reg
  WDT->INTENSET.reg = WDT_INTENSET_EW;
}

void WDT_Handler() {
  // Clear interrupt register flag
  // (reference register with WDT->register_name.reg)
  WDT->INTFLAG.bit.EW = 1;
  
  // Warn user that a watchdog reset may happen
  // TODO: light up status LED red
  Serial.println("Warning: watchdog reset imminent!");
}
