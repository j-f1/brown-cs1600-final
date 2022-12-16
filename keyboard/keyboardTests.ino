#define NTESTS 3
bool (*tests[NTESTS])() = {testLEDIdle, testKeyProcess, testBufOverflow};

void runTests() {
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

}

void resetState() {
  emptyBuffer();
}

bool simulateTyping(String input) {
  for (int i = 0; i < input.length(); i++) {
    char c = input[i];
    processKeypress(c);
    if (buf[(bufStart+bufLen-1) % BUFSIZE] != c) return false;
  }
  return true;
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
