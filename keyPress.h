/*
 * keyPress.h
 *
 *  Created on: Aug 6, 2023
 *      Author: luke
 */

#ifndef KEYPRESS_H_
#define KEYPRESS_H_

#include <arduino.h>
#include <stdint.h>
#include "AudioToneGen.h"
#include <queue>
#include "MorseSymbol.h"
#include "Qcontainer.h"
// Include the LED_Matrix library
//#include "Arduino_LED_Matrix.h"
#include "LEDmatrix.h"


class keyPress : public MorseSymbolDefn, LEDmatrix {
public:
  typedef enum keyState_e {
    keyStateIdle,
    keyStateDown,
    keyStateUp,
    keyStateStuck
  } keyState_t;

  typedef enum keyElement_e { Kup,
                              Kdown,
                              Kidle,
                              Kstuck } keyElement_t;


  typedef struct keyElementToken_st {
    uint32_t Duration;
    keyElement_t Event;
    morseElement_t morseElement;
    uint8_t score;
    uint8_t WPM;
    float tDitUnits;
  } keyElementToken_t;


  keyPress(AudioToneGen* atgen, Qcontainer* _Queues);
  virtual ~keyPress();

  void keyDebounce(void);
  keyState_t getKeyState(void);
  bool refreshRingBuffer(void);

  void getElement(void);
  void processKeyEntry(void);
  keyElementToken_t* getElementStream(void);
  bool getIsFullRingBuffer(void);
  void printKeyElement(keyElementToken_t* KET);
  char* printStrMorseElement(morseElement_t E);
  void calcWPM(morseCharToken_t* pMorseCharToken);

  inline keyElementToken_t* peekRingBuffer(uint8_t idx) {
    uint8_t getIdx = headRingBuffer + idx;
    if (getIdx >= sizeRingBuffer) {
      getIdx = getIdx - sizeRingBuffer;
    }
    return &ringBuffer[getIdx];
  }

  inline uint8_t getSizeRingBuffer(void) {
    return sizeRingBuffer;
  }

  inline uint8_t getCountRingBuffer(void) {
    return countRingBuffer;
  }

  inline void initialRingBuffer(void) {
    headRingBuffer = tailRingBuffer = countRingBuffer = 0;
    isMTRingBuffer = true;
    isfullRingBuffer = false;
  }

  /*-----------------------------------------------------------------*/
private:
  Qcontainer* Queues;

  uint32_t keyDiscretePin;
  uint32_t ledDiscretePin = 13;  // Built-in LED on  Arduino boards

  uint32_t stuckTimeout = 1000U;
  uint32_t idleTimeout = 2000U;
  const uint32_t debounceInterval = 10U;
  uint16_t runningWPM;
  uint16_t maximumWPM;
  uint16_t WPM;
  uint16_t timingMorseSpace;
  uint16_t timingMorseWordSpace;
  bool haveMark;

  uint16_t TshortestMark, TshortestDahDit;
  uint16_t currentTdit;

  keyElementToken_t ringBuffer[24];
  const uint8_t sizeRingBuffer = sizeof(ringBuffer)
                                 / sizeof(keyElementToken_t);
  uint8_t headRingBuffer;
  uint8_t tailRingBuffer;
  uint8_t countRingBuffer;
  bool isfullRingBuffer, isMTRingBuffer;

  void assignMorseElements(void);
  void analyzeRingBuffer(void);
  bool fillRingBuffer(void);
  bool spaceDetect(void);

  inline uint8_t incrRingBuffer(uint8_t idx) {
    idx++;
    if (idx >= sizeRingBuffer) {
      idx = 0;
    }
    return idx;
  }

  inline keyElementToken_t* pullRingBuffer(void) {
    keyElementToken_t* thisEntry;
    if (isMTRingBuffer == true) {
      thisEntry = NULL;
    } else {
      isfullRingBuffer = false;
      thisEntry = &ringBuffer[headRingBuffer];
      headRingBuffer = incrRingBuffer(headRingBuffer);
      countRingBuffer--;
      if (countRingBuffer == 0) {
        isMTRingBuffer = true;
      }
    }
    return thisEntry;
  }

  inline keyElementToken_t* pushRingBuffer(keyElementToken_t elementToken) {
    keyElementToken_t* thisEntry;

    if (isfullRingBuffer == true) {
      thisEntry = NULL;
    } else {
      ringBuffer[tailRingBuffer] = elementToken;
      thisEntry = &ringBuffer[tailRingBuffer];
      thisEntry->score = tailRingBuffer;
      tailRingBuffer = incrRingBuffer(tailRingBuffer);
      countRingBuffer++;
      isMTRingBuffer = false;
      if (countRingBuffer == sizeRingBuffer) {
        isfullRingBuffer = true;
      }
    }
    return thisEntry;
  }

  /*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
  typedef enum elementState_e {
    elementStateStart,
    elementStateDown,
    elementStateUp,
    elementStateIdle,
    elementStateStuck
  } elementState_t;

  typedef enum keyPos_e {
    keyUp,
    keyDown
  } keyPos_t;

  typedef enum KDB_Debounce_e { KDB_Debounce_Start,
                                KDB_Debounce_Dn,
                                KDB_Debounce_Up,
                                KDB_Down,
                                //KDB_Stuck,
                                KDB_Up
  } KDB_Debounce_t;
  uint32_t lastDuration;
  elementState_t Element_st;
  keyState_t keyState_st;
  KDB_Debounce_t keyDebounce_st;
  volatile bool Adc16ConversionDoneFlag;
  uint32_t Adc16ConversionValue;
  const uint32_t DEMO_ADC16_CHANNEL_GROUP = 0U;
  std::queue<keyElementToken_t> ElementStream;
  AudioToneGen* ATGen;
  uint32_t* senseKey(void);
  keyPos_t senseKeyPos(void);
  uint32_t getDuration(void);
  void setSideTone(void);

  /*******  LED Matrix **********/
  // Create an instance of the ArduinoLEDMatrix class
  //ArduinoLEDMatrix  matrix;
  // Define the frame array for the LED matrix
  //uint8_t frame[8][12];

  //LEDmatrix lsdmatrix;

  // Methods:
  void dsplyMorseLED(morseCharToken_t* pMorseCharToken);
  
  inline void setLEDpixel(uint8_t row, uint8_t col, uint8_t value) {
    if ((row >= 8) || (col >= 12)) {
      return;
    } else {
      //frame[row][col] = value;
    }
  }

};

#endif /* KEYPRESS_H_ */
