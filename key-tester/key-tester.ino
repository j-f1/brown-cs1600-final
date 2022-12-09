#include <Keyboard.h>

int rows[] = {1,2,3};
#define nrows 3
int cols[] = {4,5,6,7,8,9,0};
#define ncols 7

char keymap[nrows][ncols] = {
  {'a', 'a', 'a', 'a', 'a', 'a', 'a'},
  {'a', 'a', 'a', 'a', 'a', 'a', 'a'},
  {'a', 'a', 'a', 'a', 'a', 'a', 'a'},
};

int activeRow;

void setup() {
  Serial.begin(9600);
  Keyboard.begin();

  for (int i = 0; i < nrows; i++) {
    pinMode(rows[i], OUTPUT);
  }
  for (int i = 0; i < ncols; i++) {
    pinMode(cols[i], INPUT);
    attachInterrupt(digitalPinToInterrupt(cols[i]), onKeypress, RISING);
  }
  
  setupWDT();
  activeRow = 0;
}

void loop() {
  digitalWrite(rows[activeRow], LOW);
  activeRow = (activeRow + 1) % nrows;
  digitalWrite(rows[activeRow], HIGH);

  // Pet the watchdog
  WDT->CLEAR.reg = 0xA5;
}

void onKeypress() {
  Serial.println("Key pressed");
  for (int col = 0; col < ncols; col++) {
    if (digitalRead(cols[col]) == HIGH) {
      Serial.print("Detected key press at ");
      Serial.print(col);
      Serial.print(", ");
      Serial.print(activeRow);
      Serial.print(": ");
      Serial.print(keymap[activeRow][col]);
      Serial.println();
      Keyboard.print(keymap[activeRow][col]);
      break; // TODO: how to handle multiple active cols
    }
  }
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
