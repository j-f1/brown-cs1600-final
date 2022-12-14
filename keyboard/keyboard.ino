#include <Keyboard.h>

int rows[] = {1,2,3};
#define NROWS 3
int cols[] = {4,5,6,7,8,9,0};
#define NCOLS 7

const int backspaceButton = A1;

char keymap[NROWS][NCOLS] = {
  {'p', 'l', 'k', 'j', 'h', 'g', 'f'},
  {'t', 'r', 'd', 's', 'w', 'q', 'a'},
  {'m', 'n', 'b', 'v', 'c', 'x', 'z'},
};

// GPT completion setup
const int completionDelayMillis = 100;
int lastKeypressMillis;
bool completionRequested;

// Buffer setup
#define BUFSIZE 1024
char buf[BUFSIZE];
int bufStart;
int bufLen;

// LED setup
const int rPin = A3;
const int gPin = A4;
int pwmVal = 0;
int dir = 1;
// The LED's color should be maintained at its current value until this time
int maintainLedColorUntilMillis = 0;

/**
 * Sets the led to the provided (r, g) value.
 */
void setLedColor(int r, int g) {
  analogWrite(rPin, r);
  analogWrite(gPin, g);
}

/**
 * Indicate idle state with a pulsing green light on the LED.
 */
void ledIndicateIdle() {
  setLedColor(0, pwmVal);
  
  pwmVal = constrain(pwmVal + 1 * dir, 0, 255);
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
  setLedColor(0, 0);
}

/**
 * Main setup routine.
 */
void setup() {
  // Initialize serial & keyboard
  Serial.begin(9600);
  Serial1.begin(300);
  Keyboard.begin();

  // Initialize pins
  initializeRowPins();
  initializeColumnPins();
  initializeLedPins();

  lastKeypressMillis = millis();

  setup_wifi();
  setupWDT();

  pinMode(backspaceButton, INPUT);
  attachInterrupt(digitalPinToInterrupt(backspaceButton), onBackspace, RISING);

  // Fill the current word buffer with nul-terminators
  memset(buf, '\0', BUFSIZE);
  bufStart = 0;
  bufLen = 0;
  
  resetCompletions();
}

/**
 * Processes a keypress, sending it to the attached device.
 * If the keypress is a backspace, deletes a character from the buffer;
 * otherwise, adds the character to the buffer.
 * Returns true on success.
 * NB: Must be called with interrupts disabled.
 */
bool processKeypress(char c) {
  if (c == KEY_BACKSPACE) {
    if (bufLen == 0) {
      return false;
    } else {
      Keyboard.print(c);
      bufLen--;
      buf[bufLen] = '\0';
      lastKeypressMillis = millis();
      return true;
    }
  } else {
    if (isBufferFull()) {
      return false;
    } else {
      Keyboard.print(c);
      buf[bufLen] = c;
      bufLen++;
      lastKeypressMillis = millis();
      return true;
    }
  }
}

/**
 * Displays a space-separated list of completions on the LCD.
 */
void displayCompletions(char *completedWords) {
  Serial.print("Displaying completions: '");
  Serial.print(completedWords);
  Serial.println("'");
  Serial1.write(completedWords);
  Serial1.write('\0');
}

/**
 * Invalidates the current completions, resetting the completion LCD.
 */
void resetCompletions() {
  displayCompletions("");
  completionRequested = false;
}

/**
 * Sends the vowel-less word to GPT-3, and sends the results to the completion
 * display. Returns false if an error occurred.
 */
bool completeWord(char *curWord) {
  static char completedWords[BUFSIZE];

  // Make the request to fill in the vowels
  if (makeRequest(curWord, completedWords, BUFSIZE)) {
    displayCompletions(completedWords);
    return true;
  } else {
    Serial.println("Error: failed to make word completions");
    return false;
  }
}

/**
 * Requests completions from GPT-3 for the current word f the completion delay has elapsed.
 */
void completeCurrentWord() {
  // Wait for a short time since the last keypress before requesting completions to
  // avoid repeated requests when a user types multiple characters in short succession
  int delayElapsed = millis() - lastKeypressMillis > completionDelayMillis;

  // Determine the currently typed word
  noInterrupts();
  int curWordLen;
  char *curWord = getCurWord(&curWordLen);
  interrupts();
  
  if (delayElapsed || curWordLen == 0) {
    completionRequested = true;
  
    if (curWordLen > 0) {
      // Indicate request is in progress
      setLedColor(128, 128);
  
      if (completeWord(curWord)) {
        setLedColor(0, 255); // Indicate success
      } else {
        setLedColor(255, 0); // Indicate failure
      }
      maintainLedColorUntilMillis = millis() + 1000;
    }
  }
}

/**
 * Accepts an incoming accepted completion from the display controller if possible.
 * Subsequent calls invalidate previously returned strings.
 */
char *receiveAcceptedCompletion() {
  if (Serial1.available() == 0) {
    return 0;
  } else {
    static char word[BUFSIZE];
    memset(word, '\0', BUFSIZE);
    int wordLen = 0;
    char incoming;
    while (wordLen < BUFSIZE) {
      while (Serial1.available() == 0) {}
      incoming = Serial1.read();
      word[wordLen] = incoming;
      if (incoming == '\0') {
        break;
      } else {
        wordLen++;
      }
    }

    Serial.print("Received completion: ");
    Serial.println(word);
    return word;
  }
}

// Returns the most recently typed (partial) word.
// Must be called with interrupts disabled.
// Subsequent calls invalidate previously returned pointers.
char *getCurWord(int *startOffsetFromEnd) {
  static char curWord[BUFSIZE];
  memset(curWord, 0, BUFSIZE);
  
  int bufEnd = (bufStart+bufLen) % BUFSIZE;
  int curWordStart = bufEnd;
  int curWordLen = 0;
  int trailingSpaces = 0;
  bool seenChars = false;
  for (int i = 0; i < bufLen; i++) {
    int idx = (bufEnd-i-1) % BUFSIZE;
    if (buf[idx] == ' ') {
      if (seenChars) {
        break;
      } else {
        trailingSpaces++;
        continue;
      }
    } else {
      curWordStart = idx;
      curWordLen++;
      seenChars = true;
    }
  }
  for (int i = 0; i < curWordLen; i++) {
    curWord[i] = buf[(curWordStart+i) % BUFSIZE];
  }
  if (startOffsetFromEnd) {
    *startOffsetFromEnd = curWordLen + trailingSpaces;
  }
  return curWord;
}

/**
 * Main loop routine.
 */
void loop() {
  if (millis() > maintainLedColorUntilMillis) {
    ledIndicateIdle();
  }

  if (!completionRequested) {
    completeCurrentWord();
  }

  char *acceptedCompletion = receiveAcceptedCompletion();
  if (acceptedCompletion) {
    // Clear the un-voweled word and type in the completed word
    noInterrupts();
    int charsToDelete;
    char *curWord = getCurWord(&charsToDelete);
    for (int i = 0; i < charsToDelete; i++) {
      processKeypress(KEY_BACKSPACE);
    }
    for (char *c = acceptedCompletion; *c != 0; c++) {
      processKeypress(*c);
    }
    processKeypress(' ');
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
  return bufLen >= BUFSIZE;
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

    // get the previous letter
    char prevLetter = '\0';
    if (bufLen > 0) {
      prevLetter = buf[bufLen - 1];
    }

    // check that it's not the same key being held continuously
    if (millis() - lastKeypressMillis > 5 || letter != prevLetter) {
      processKeypress(letter);
      resetCompletions();
    }
  }
  else {
     // Serial.println("Unable to detect which key was pressed.");
  }

  // Reset to the default state.
  setAllRowOutputs(HIGH);
  interrupts();
}

void onBackspace() {
  static int lastPressed = 0;
  int now = millis();
  if (now - lastPressed < 25) return;
  lastPressed = now;
  
  Serial.println("backspace");
  processKeypress(KEY_BACKSPACE);
  resetCompletions();
}
