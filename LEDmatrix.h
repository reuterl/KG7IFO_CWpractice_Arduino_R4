#ifndef LEDMATRIX_H_
#define LEDMATRIX_H_

#include "Arduino.h"
#include "FspTimer.h"

extern char sSprintf[];

// Timer hoo called from NyquistTimer
typedef void (*timerHook)(void);
extern timerHook pTimerHook;

static const int pin_zero_index = 28;
const uint8_t pins[96][2] = {
  { 7, 3 },  // 0
  { 3, 7 },
  { 7, 4 },
  { 4, 7 },
  { 3, 4 },
  { 4, 3 },
  { 7, 8 },
  { 8, 7 },
  { 3, 8 },
  { 8, 3 },
  { 4, 8 },  // 10
  { 8, 4 },
  { 7, 0 },
  { 0, 7 },
  { 3, 0 },
  { 0, 3 },
  { 4, 0 },
  { 0, 4 },
  { 8, 0 },
  { 0, 8 },
  { 7, 6 },  // 20
  { 6, 7 },
  { 3, 6 },
  { 6, 3 },
  { 4, 6 },
  { 6, 4 },
  { 8, 6 },
  { 6, 8 },
  { 0, 6 },
  { 6, 0 },
  { 7, 5 },  // 30
  { 5, 7 },
  { 3, 5 },
  { 5, 3 },
  { 4, 5 },
  { 5, 4 },
  { 8, 5 },
  { 5, 8 },
  { 0, 5 },
  { 5, 0 },
  { 6, 5 },  // 40
  { 5, 6 },
  { 7, 1 },
  { 1, 7 },
  { 3, 1 },
  { 1, 3 },
  { 4, 1 },
  { 1, 4 },
  { 8, 1 },
  { 1, 8 },
  { 0, 1 },  // 50
  { 1, 0 },
  { 6, 1 },
  { 1, 6 },
  { 5, 1 },
  { 1, 5 },
  { 7, 2 },
  { 2, 7 },
  { 3, 2 },
  { 2, 3 },
  { 4, 2 },
  { 2, 4 },
  { 8, 2 },
  { 2, 8 },
  { 0, 2 },
  { 2, 0 },
  { 6, 2 },
  { 2, 6 },
  { 5, 2 },
  { 2, 5 },
  { 1, 2 },
  { 2, 1 },
  { 7, 10 },
  { 10, 7 },
  { 3, 10 },
  { 10, 3 },
  { 4, 10 },
  { 10, 4 },
  { 8, 10 },
  { 10, 8 },
  { 0, 10 },
  { 10, 0 },
  { 6, 10 },
  { 10, 6 },
  { 5, 10 },
  { 10, 5 },
  { 1, 10 },
  { 10, 1 },
  { 2, 10 },
  { 10, 2 },
  { 7, 9 },
  { 9, 7 },
  { 3, 9 },
  { 9, 3 },
  { 4, 9 },
  { 9, 4 },
};



#define LED_MATRIX_PORT0_MASK ((1 << 3) | (1 << 4) | (1 << 11) | (1 << 12) | (1 << 13) | (1 << 15))
#define LED_MATRIX_PORT2_MASK ((1 << 4) | (1 << 5) | (1 << 6) | (1 << 12) | (1 << 13))

static union phrame {
  uint8_t frameMatrix[8][12];
  uint8_t frameLinear[96];
} u_phrame;

static void turnLed(int idx, bool on) {
  R_PORT0->PCNTR1 &= ~((uint32_t)LED_MATRIX_PORT0_MASK);
  R_PORT2->PCNTR1 &= ~((uint32_t)LED_MATRIX_PORT2_MASK);

  if (on) {
    bsp_io_port_pin_t pin_a = g_pin_cfg[pins[idx][0] + pin_zero_index].pin;
    R_PFS->PORT[pin_a >> 8].PIN[pin_a & 0xFF].PmnPFS =
      IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_HIGH;

    bsp_io_port_pin_t pin_c = g_pin_cfg[pins[idx][1] + pin_zero_index].pin;
    R_PFS->PORT[pin_c >> 8].PIN[pin_c & 0xFF].PmnPFS =
      IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_LOW;
  }
}

static inline void doctorHook(void) {
  static int pin = 0;
  static uint64_t lastTime = 0;
  uint64_t thisTime = millis();
  if (true) {
    turnLed(pin, u_phrame.frameLinear[pin]);
    pin += 1;
    if (pin > 95) pin = 0;
    lastTime = thisTime;
  }
}


class LEDmatrix {

private:

  /*---------------------------------------------------------------------------*/

public:
  LEDmatrix() {
    Serial.println("LEDmatrix: constructor");
    pTimerHook = &doctorHook;
    for (int i = 0; i < 96; i++) {
      u_phrame.frameLinear[i] = 0;
    }
    dsplyLEDmatrix();
  }

  // apply matrix contents to LED drivers
  inline void dsplyLEDmatrix(void) {
    for (int i = 0; i < 96; i++) {
      turnLed(i, u_phrame.frameLinear[i]);
    }
  }

  // Scroll up one row
  inline void scrollLEDmatrix(void) {
    int dest = 0;
    for (int i = 12; i < 96; i++) {
      u_phrame.frameLinear[dest++] = u_phrame.frameLinear[i];
    }
    for (int i = 83; i< 96; i++){
      u_phrame.frameLinear[i] = 0;
    }
  }

  inline void setLEDmatrix(uint8_t row, uint8_t col, bool on) {
    if ((row > 7) || (col > 11)) {
      return;
    } else {
      u_phrame.frameMatrix[row][col] = on ? 1 : 0;
    }
  }
};
#endif