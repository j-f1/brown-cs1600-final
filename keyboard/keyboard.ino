#include <Keyboard.h>

int rows[] = {1,2,3};
#define NROWS 3
int cols[] = {4,5,6,7,8,9,0};
#define NCOLS 7

char keymap[NROWS][NCOLS] = {
  {'p', 'l', 'k', 'j', 'h', 'g', 'f'},
  {'t', 'r', 'd', 's', 'w', 'q', 'a'},
  {'m', 'n', 'b', 'v', 'c', 'x', 'z'},
};

// GPT completion setup
const int completionDelayMillis = 100;
int lastKeypressMillis;

// Buffer setup
#define BUFSIZE 256
char buf[BUFSIZE];
int curWordStart = 0;
int curWordLen = 0;
int curWordProcessedLen = 0;

// LED setup
int rPin = A3;
int gPin = 11;
int bPin = A4;
int pwmVal = 0;
int dir = 1;

/**
 * Sets the led to the provided (r, g, b) value.
 */
void setLedColor(int r, int g, int b) {
  analogWrite(rPin, r);
  analogWrite(gPin, g);
  analogWrite(bPin, b);
}


/**
 * Indicate idle state with a pulsing green light on the LED.
 */
void ledIndicateIdle() {
  setLedColor(0, pwmVal, 0);
  
  pwmVal = constrain(pwmVal + 10 * dir, 0, 255);
  if (pwmVal >= 255 || pwmVal <= 0) {
    dir = -dir;
  }
}

/**
 * Turns all of the row pins to the value [on], which should be either HIGH or LOW.
 */
void setAllRowOutputs(bool on) {
  for (int r = 0; r < NROWS; r++) {
    digitalWrite(rows[r], on);
  }
}

/**
 * Initializes each of the row pins as an output pin, and powers it by default.
 */
void initializeRowPins() {
  for (int r = 0; r < NROWS; r++) {
    pinMode(rows[r], OUTPUT);
  }
  setAllRowOutputs(HIGH);
}

/**
 * Initializes each of the column pins as an input pin, and attaches the `onKeypress` ISR to run on a rising edge.
 */
void initializeColumnPins() {
  // Set each column to be an input pin; attach `onKeypress` to rising edge.
  for (int c = 0; c < NCOLS; c++) {
    pinMode(cols[c], INPUT);
    attachInterrupt(digitalPinToInterrupt(cols[c]), onKeypress, RISING);
  }
}

/**
 * Initializes each of the LED pins as output pins and turns off the LED by default.
 */
void initializeLedPins() {
  // Initialize led pins as output pins and drive them low to start.
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);
  setLedColor(0, 0, 0);
}

/**
 * Main setup routine.
 */
void setup() {
  // Initialize serial & keyboard
  Serial.begin(9600);
  Keyboard.begin();
  while(!Serial);

  // Initialize pins
  initializeRowPins();
  initializeColumnPins();
  initializeLedPins();

  lastKeypressMillis = millis();

  setup_wifi();
//  setupWDT();

  // initialize the buffer
  memset(buf, '\0', BUFSIZE);
}

/**
 * Sends the new (unprocessed) keypresses to the computer to be typed.
 */
void sendNewKeypresses() {
  noInterrupts();

  for(; curWordProcessedLen < curWordLen; curWordProcessedLen++) {
    char charToProcess = buf[curWordStart + curWordProcessedLen];
    Keyboard.print(charToProcess);
  }

  interrupts();
}

/**
 * Gets the current word from the buffer, null-terminates it and returns it.
 */
char *getCurWord() {
  char curWord[BUFSIZE];
  memset(curWord, '\0', BUFSIZE);
  
  for (int i = 0; i < curWordLen; i++) {
    curWord[i] = buf[curWordStart + i];
  }

  return curWord;
}

/**
 * Gets the vowel-less word, sends it to GPT-3 and returns the resulting word. Returns 
 * true if successful, and false if the completion was not successful.
 */
bool completeWord() {
  char *curWord = getCurWord();

  // make the request to fill in the vowels
  char completedWord[BUFSIZE];
  bool completionResult = makeRequest(curWord, completedWord, 200);
  if (!completionResult) {
    Serial.println("Error: failed to make word completion");
    return false;
  }

  // clear the un-voweled word and type in the completed word
  for (int i = 0; i < curWordLen; i++) {
    Keyboard.press(KEY_BACKSPACE);
  }
  Keyboard.print(completionResult);
  Keyboard.print(' ');

  // reset the current word variables
  curWordStart += curWordLen;
  curWordLen = 0;
  curWordProcessedLen = 0;
  return true;
}


/**
 * Main loop routine.
 */
void loop() {
  ledIndicateIdle();
  sendNewKeypresses();

  // If it's been a second since the last keypress,
  // request completions from GPT-3
  if (millis() - lastKeypressMillis > completionDelayMillis) {
    noInterrupts();
    setLedColor(0, 0, 0);

    bool isCompletionSuccessful = completeWord();
    if (isCompletionSuccessful) {
      setLedColor(0, 255, 0);
    } else {
      setLedColor(255, 0, 255);
    }
    
    interrupts();
  }

  petWatchdog();
}

/**
 * Gets the index of the first active column when powering just [poweredRow]. If no column is
 * active, returns -1.
 */
int getActiveColumn(int poweredRow) {
  // power just the poweredRow
  setAllRowOutputs(LOW);
  digitalWrite(rows[poweredRow], HIGH);

  for (int c = 0; c < NCOLS; c++) {
    if (digitalRead(cols[c]) == HIGH) {
      return c;
    }
  }
  return -1;
}

/**
 * Returns true iff the buffer does not have space to store another letter.
 */
bool isBufferFull() {
  return (bufStart + bufLen) > BUFSIZE;
}

/**
 * Attempts to resize the buffer by dropping all previous words and moving the current
 * word to the beginning of the buffer.
 */
void resizeBuffer() {
  // TODO:
}

/**
 * ISR for handling keypresses. Run when a rising edge on a column is detected.
 * Subsequently, it cycles through powering each row individually to detect which row
 * the keypress occured in. Adds the resulting letter to the buffer.
 */
void onKeypress() {
  noInterrupts();

  // find the exact key that was pressed by powering each row individually and checking for a column press
  int activeCol = -1;
  int activeRow = -1;
  for (int r = 0; r < NROWS; r++) {
    activeCol = getActiveColumn(r);
    if (activeCol != -1) {
      activeRow = r;
      break;
    }
  }

  // Process key and add it to the buffer
  // check that we were able to detect the key
  if (activeCol != -1 && activeRow != -1) {
    char letter = keymap[activeRow][activeCol];

    // check that it's not the same key being held continuously
    if (millis() - lastKeypressMillis > 5 || letter != ...) {
      // if buffer is full, resize it first
      if (isBufferFull()) { resizeBuffer(); }

      buf[curWordStart + curWordLen] = letter;
      curWordLen++;
    }
  }
  else {
    Serial.println("Unable to detect which key was pressed.");
  }

  // Reset to the default state.
  setAllRowOutputs(HIGH);
  lastKeypressMillis = millis();
  interrupts();
}

/**
 * Sets up a 4 second watchdog timer. Adapted from lab 4.
 */
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
  Serial.println("Warning: watchdog reset imminent!");
  setLedColor(255, 0, 0);
}

void petWatchdog() {
  WDT->CLEAR.reg = 0xA5;
}
