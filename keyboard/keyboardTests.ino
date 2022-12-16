#define NTESTS 4
bool (*tests[NTESTS])() = {
  testLEDIdle, testKeyProcess, testBufOverflow, testAcceptCompletion,
};

static bool testing = 0;

static bool mockKeyState[NROWS][NCOLS] = {
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
};
static int mockRow = -1;

void runTests() {
  testing = 1;
  Serial.println("Running tests....");
  int passed = 0;
  for (int i = 0; i < NTESTS; i++) {
    resetState();
    bool testPassed = tests[i]();
    passed += (int)testPassed;
  }
  Serial.print("Tests Passed: ");
  Serial.print(passed);
  Serial.print(", Tests Failed: ");
  Serial.println(NTESTS-passed);
  testing = 0;
}

void resetState() {
  emptyBuffer();
  clearMockKeyState();
}

bool simulateTyping(String input) {
  for (int i = 0; i < input.length(); i++) {
    char c = input[i];
    processKeypress(c);
    if (buf[(bufStart+bufLen-1) % BUFSIZE] != c) return false;
  }
  return true;
}

bool bufEquals(char *expected) {
  int i = 0;
  // Exhaust the buffer
  for (; i < bufLen; i++) {
    if (*expected == 0 || *expected != buf[(bufStart+i) % BUFSIZE]) return false;
  }
  // Make sure we've exhausted the expected string
  return *expected == 0;
}

/**
 * Test that the LED's PWM duty cycle is commanded up/down by one step
 * with each call to `ledIndicateIdle()`.
 */
bool testLEDIdle() {
  for (int i = 0; i < 1000; i++) {
    int prev = pwmVal;
    ledIndicateIdle();
    if (abs(pwmVal-prev) != 1) {
      return false;
    }
  }
  return true;
}

// TODO: test pressing no keys, pressing multiple keys at once
void writePin(int pin, int val) {
  if (testing) {
    for (int i = 0; i < NROWS; i++) {
      if (pin == rows[i]) {
        if (val == LOW && mockRow == i) {
          mockRow = -1;
        } else if (val == HIGH && mockRow == -1) {
          mockRow = i;
        }
        return;
      }
    }
  } else {
    digitalWrite(pin, val);
  }
}

int readPin(int pin) {
  if (testing) {
    for (int i = 0; i < NCOLS; i++) {
      if (pin == cols[i]) {
        return mockKeyState[mockRow][i];
      }
    }
  } else {
    return digitalRead(pin);
  }
}

void clearMockKeyState() {
  for (int i = 0; i < NROWS; i++) {
    for (int j = 0; j < NCOLS; j++) {
      mockKeyState[i][j] = 0;
    }
  }
}
// returns the row that the key was pressed
int mockKeypress(char c) {
  for (int i = 0; i < NROWS; i++) {
    for (int j = 0; j < NCOLS; j++) {
      if (keymap[i][j] == c) {
        mockKeyState[i][j] = 1;
        return i;
      }
    }
  }
}

void testNoKeysPressed() {
  onKeypress();
  if (bufLen != 0) {
    Serial.println("testNoKeysPressed failed");
  }
}

void testTwoKeysPressed() {
  mockKeypress('x');
  mockKeypress('r');
  onKeypress();
  if (bufLen != 1) {
    Serial.println("testTwoKeysPressed failed");
  }
}

void testOneKeyPressed() {
  for (int i = 0; i < NROWS; i++) {
    for (int j = 0; j < NCOLS; j++) {
      resetState();
      mockKeyState[i][j] = 1;
      onKeypress();
      if (bufLen != 1 && buf[bufStart] != keymap[i][j]) {
        Serial.println("testOneKeyPressed failed");
      }
    }
  }
}

/**
 * Test that a small number of characters can be added to an empty buffer.
 */
bool testKeyProcess() {
  return simulateTyping("hello");
}

/**
 * Test that the buffer truncates the oldest word from the buffer to avoid overflow.
 */
bool testBufOverflow() {
  int wordLen = 10;
  for (int i = 0; i < 10; i++) {
    processKeypress('a');
  }
  processKeypress(' ');
  while (bufLen < BUFSIZE) processKeypress('b');
  processKeypress('c');
  return (bufStart == wordLen+1 && buf[bufStart] == 'b' && bufLen == BUFSIZE-wordLen-1);
}

/**
 * Test that accepting a completion replaces the last word in the buffer.
 */
bool testAcceptCompletion() {
  if (!simulateTyping("hll")) return false;

  acceptCompletion("hallo");
  if (!bufEquals("hallo ")) return false;

  acceptCompletion("hello");
  if (!bufEquals("hello ")) return false;

  if (!simulateTyping("wrld")) return false;
  if (!bufEquals("hello wrld")) return false;

  acceptCompletion("world");
  if (!bufEquals("hello world ")) return false;

  return true;
}

// TODO: test request timeout w/ watchdog
