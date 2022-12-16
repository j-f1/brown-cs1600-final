bool (*tests[])() = {
  testLEDIdle, testKeyProcess, testBufOverflow, testAcceptCompletion, testNoKeysPressed, testTwoKeysPressed, testOneKeyPressed,
  NULL
};

static bool testing = 0;
bool testsDone = false;

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
  int total = 0;
  for (int i = 0; tests[i]; i++) {
    resetState();
    bool testPassed = tests[i]();
    passed += (int)testPassed;
    total++;
  }
  Serial.print("Tests Passed: ");
  Serial.print(passed);
  Serial.print(", Tests Failed: ");
  Serial.println(total - passed);
  testing = 0;
  testsDone = true;
}

void resetState() {
  emptyBuffer();
  clearMockKeyState();
}

bool simulateTyping(String input) {
  for (uint i = 0; i < input.length(); i++) {
    char c = input[i];
    processKeypress(c);
    if (buf[(bufStart+bufLen-1) % BUFSIZE] != c) return false;
  }
  return true;
}

bool bufEquals(const char *expected) {
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
      Serial.println("testLEDIdle failed");
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
    // invalid pin read
    abort();
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
  return -1;
}

/*
 * Tests that no action is taken when no keys are pressed
 */
bool testNoKeysPressed() {
  onKeypress();
  if (bufLen != 0) {
    Serial.println("testNoKeysPressed failed");
    return false;
  }
  return true;
}

/*
 * Tests that when two keys are pressed, only one of the letter is added to buffer
 */
bool testTwoKeysPressed() {
  mockKeypress('x');
  mockKeypress('r');
  onKeypress();
  if (bufLen != 1) {
    Serial.println("testTwoKeysPressed failed");
    return false;
  }
  return true;
}

/*
 * Tests a single key press registers and is added to buffer
 */
bool testOneKeyPressed() {
  for (int i = 0; i < NROWS; i++) {
    for (int j = 0; j < NCOLS; j++) {
      resetState();
      mockKeyState[i][j] = 1;
      onKeypress();
      if (bufLen != 1 && buf[bufStart] != keymap[i][j]) {
        Serial.println("testOneKeyPressed failed");
        return false;
      }
    }
  }
  return true;
}

/**
 * Test that a small number of characters can be added to an empty buffer.
 */
bool testKeyProcess() {
  if (simulateTyping("hello")) {
    return true;
  }
  Serial.println("testKeyProcess failed");
  return false;
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
  if (bufStart == wordLen+1 && buf[bufStart] == 'b' && bufLen == BUFSIZE-wordLen) {
    return true;
  }
  Serial.print("testBufOverflow failed: bufStart = ");
  Serial.print(bufStart);
  Serial.print(", expected: ");
  Serial.print(wordLen + 1);
  Serial.print("; buf[bufStart] = '");
  Serial.print(buf[bufStart]);
  Serial.print("' (");
  Serial.print((int)buf[bufStart]);
  Serial.print("), expected 'b'; bufLen = ");
  Serial.print(bufLen);
  Serial.print(", expected ");
  Serial.println(BUFSIZE-wordLen);

  return false;
}

/**
 * Test that accepting a completion replaces the last word in the buffer.
 */
bool testAcceptCompletion() {
  if (!simulateTyping("hll")) {
    Serial.println("similateTyping failed in testAcceptCompletion");
    return false;
  }

  acceptCompletion("hallo");
  if (!bufEquals("hallo ")) {
    Serial.println("testAcceptCompletion failed");
    return false;
  }

  acceptCompletion("hello");
  if (!bufEquals("hello ")) {
    Serial.println("testAcceptCompletion failed");
    return false;
  }

  if (!simulateTyping("wrld")) {
    Serial.println("similateTyping failed in testAcceptCompletion");
    return false;
  }
  if (!bufEquals("hello wrld")) {
    Serial.println("testAcceptCompletion failed");
    return false;
  }

  acceptCompletion("world");
  if (!bufEquals("hello world ")) {
    Serial.println("testAcceptCompletion failed");
    return false;
  }

  return true;
}

// TODO: test request timeout w/ watchdog
