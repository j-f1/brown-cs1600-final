#include <Keyboard.h>

int rows[] = {0, 1};
#define nrows 2
int cols[] = {7,6};
#define ncols 2

char keymap[nrows][ncols] = {
  {'b', 'm'},
  {'z', 'j'},
};

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Keyboard.begin();
  for (int i = 0; i < nrows; i++) {
    pinMode(rows[i], INPUT);
    attachInterrupt(digitalPinToInterrupt(rows[i]), onKeypress, RISING);
  }
  for (int i = 0; i < ncols; i++) {
    pinMode(cols[i], INPUT);
  }
  
  setupWDT();
}

void onKeypress() {
  Serial.println("press");
  int row = -1;
  for (int i = 0; i < nrows; i++) {
    if (digitalRead(rows[i]) == HIGH) {
      if (i == 1) {
      Serial.print("this is the j row: ");
      Serial.println(i);
      }
      if (row != -1) {
        Serial.print("Detected multiple rows pressed! ");
        Serial.print(row);
        Serial.print(" and ");
        Serial.println(i);
      } else {
        row = i;
      }
    }
  }

  int col = -1;
  for (int i = 0; i < ncols; i++) {
    if (digitalRead(cols[i]) == HIGH) {
      if (i == 1) {
      Serial.print("this is the j col: ");
      Serial.println(i);
      }
      if (col != -1) {
        Serial.print("Detected multiple cols pressed! ");
        Serial.print(col);
        Serial.print(" and ");
        Serial.println(i);
      } else {
        col = i;
      }
    }
  }
//should be outputting j
  if (row != -1 && col != -1) {
    Serial.print("Detected key press at ");
    Serial.print(col);
    Serial.print(", ");
    Serial.print(row);
    Serial.print(": ");
    Serial.print(keymap[row][col]);
    Serial.println();
    Keyboard.print(keymap[row][col]);
  }
}

void loop() {
  // Pet the watchdog
  WDT->CLEAR.reg = 0xA5;
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
