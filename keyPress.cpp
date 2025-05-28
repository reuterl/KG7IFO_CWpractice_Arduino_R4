/*
 * keyPress.cpp
 *
 *  Created on: Aug 6, 2023
 *      Author: luke
 */

#include "keyPress.h"
#include "msggenerator.h"
#include "ProcessCommands.h"


keyPress::keyPress(AudioToneGen *atgen, Qcontainer *_Queues) {
  Serial.println("KeyPress: Constructor");
  ATGen = atgen;

  keyDiscretePin = 7;
  ledDiscretePin = LED_BUILTIN;  // Built-in LED on  Arduino boards
  pinMode(keyDiscretePin, INPUT);
  pinMode(keyDiscretePin, INPUT_PULLUP);
  pinMode(ledDiscretePin, OUTPUT);
  digitalWrite(ledDiscretePin, LOW);  // HIGH == LED on

  keyDebounce_st = KDB_Debounce_Start;
  keyState_st = keyStateIdle;
  Element_st = elementStateStart;
  runningWPM = 5;
  maximumWPM = 10;
  WPM = 5;
  timingMorseSpace = 3;
  timingMorseWordSpace = 5;
  Queues = _Queues;
  initialRingBuffer();
}

keyPress::~keyPress() {
  // TODO Auto-generated destructor stub
  Serial.println("KeyPress: Destructor");
}


keyPress::keyPos_t keyPress::senseKeyPos(void) {
  static keyPos_t lastKeyPos = keyUp;
  keyPos_t currentKeyPos;

  uint32_t thisKeyPos = digitalRead(keyDiscretePin);

  if (thisKeyPos == HIGH) {
    digitalWrite(ledDiscretePin, HIGH);
    currentKeyPos = keyUp;
  } else {
    digitalWrite(ledDiscretePin, LOW);
    currentKeyPos = keyDown;
  }
  lastKeyPos = currentKeyPos;
  return currentKeyPos;
}

void keyPress::keyDebounce(void) {
  uint32_t Duration;

  switch (keyDebounce_st) {
    case KDB_Debounce_Start:
      if (senseKeyPos() == keyDown) {
        keyDebounce_st = KDB_Debounce_Dn;
      } else {
        keyDebounce_st = KDB_Debounce_Up;
      }
      startInterval(2);
      break;

    case KDB_Debounce_Dn:
      if (senseKeyPos() == keyDown) {
        if (getElapsed(2) < debounceInterval) {
          keyDebounce_st = KDB_Debounce_Dn;
        } else {
          keyDebounce_st = KDB_Down;
          keyState_st = keyStateDown;
        }
      } else {
        keyDebounce_st = KDB_Debounce_Start;
      }
      break;

    case KDB_Debounce_Up:
      if (senseKeyPos() == keyUp) {
        if (getElapsed(2) < debounceInterval) {
          keyDebounce_st = KDB_Debounce_Up;
        } else {
          keyDebounce_st = KDB_Up;
          keyState_st = keyStateUp;
        }
      } else {
        keyDebounce_st = KDB_Debounce_Start;
      }
      break;

    case KDB_Down:
      Duration = getElapsed(2);
      if (senseKeyPos() == keyDown) {
        keyDebounce_st = KDB_Down;
      } else {
        keyDebounce_st = KDB_Debounce_Start;
        lastDuration = Duration;
      }
      break;

    case KDB_Up:
      Duration = getElapsed(2);
      if (senseKeyPos() == keyUp) {
        keyDebounce_st = KDB_Up;
      } else {
        keyDebounce_st = KDB_Debounce_Start;
        lastDuration = Duration;
      }
      break;

    default:
      break;
  }
}

void keyPress::setSideTone(void) {
  switch (Element_st) {
    case elementStateDown:
      ATGen->setEnabled(0, false);
      ATGen->setEnabled(1, false);
      ATGen->setEnabled(2, true);
      break;
    case elementStateStuck:
      ATGen->setEnabled(2, false);
      ATGen->setEnabled(0, true);
      ATGen->setEnabled(1, true);
      break;
    default:
      ATGen->setEnabled(0, false);
      ATGen->setEnabled(1, false);
      ATGen->setEnabled(2, false);
      break;
  }
}
keyPress::keyState_t keyPress::getKeyState(void) {
  return keyState_st;
}

uint32_t keyPress::getDuration(void) {
  return lastDuration;
}
keyPress::keyElementToken_t *keyPress::getElementStream(void) {
  static keyElementToken_t KET;
  if (ElementStream.empty()) {
    return NULL;
  } else {
    KET = ElementStream.front();
    ElementStream.pop();
    return &KET;
  }
}


void keyPress::getElement(void) {
  keyState_t keyState;
  static keyElementToken_t KET;
  uint32_t Duration;

  keyDebounce();
  keyState = getKeyState();

  switch (Element_st) {
    case elementStateStart:
      switch (keyState) {
        case keyStateDown:
          Element_st = elementStateDown;
          startInterval(0);
          break;
        case keyStateUp:
          Element_st = elementStateUp;
          startInterval(0);
          break;
        default:
          break;
      }
      break;

    case elementStateDown:
      Duration = getElapsed(0);
      switch (keyState) {
        case keyStateDown:
          if (Duration < stuckTimeout) {
            Element_st = elementStateDown;
          } else {
            Element_st = elementStateStuck;
            KET.Duration = Duration;
            KET.Event = Kstuck;
            ElementStream.push(KET);
          }
          break;
        case keyStateUp:
          Element_st = elementStateStart;
          KET.Duration = Duration;
          KET.Event = Kdown;
          ElementStream.push(KET);
          break;
        default:
          break;
      }
      break;

    case elementStateUp:
      Duration = getElapsed(0);
      switch (keyState) {
        case keyStateUp:
          if (Duration < idleTimeout) {
            Element_st = elementStateUp;
          } else {
            Element_st = elementStateIdle;
            KET.Duration = Duration;
            KET.Event = Kidle;
            ElementStream.push(KET);
          }
          break;
        case keyStateDown:
          Element_st = elementStateStart;
          KET.Duration = Duration;
          KET.Event = Kup;
          ElementStream.push(KET);
          break;
        default:
          break;
      }
      break;

    case elementStateIdle:
      if (keyState != keyStateUp) {
        Element_st = elementStateStart;
      }
      break;

    case elementStateStuck:
      if (keyState != keyStateDown) {
        Element_st = elementStateStart;
      }
      break;
    default:
      break;
  }
  setSideTone();
}

bool keyPress::getIsFullRingBuffer(void) {
  return isfullRingBuffer;
}

bool keyPress::refreshRingBuffer(void) {
  keyElementToken_t elementToken;
  while ((isfullRingBuffer != true) && (!ElementStream.empty())) {
    elementToken = ElementStream.front();
    ElementStream.pop();

    pushRingBuffer(elementToken);
  }
  return true;
}

void keyPress::printKeyElement(keyElementToken_t *KET) {
  char *newLine;
  char noExtra[] = "";
  char extra[] = "\n\n";

  if (KET != NULL) {
    newLine = noExtra;
    switch (KET->Event) {
      default:
      case keyPress::Kup:
        //PRINTF("\r\nUP");
        break;
      case keyPress::Kdown:
        //PRINTF("\r\nDOWN");
        break;
      case keyPress::Kidle:
        //PRINTF("\r\nIDLE");
        newLine = extra;
        break;
      case keyPress::Kstuck:
        //PRINTF("\r\nSTUCK");
        break;
        //PRINTF("\r\n**unknown**");
        break;
    }

    if (KET->tDitUnits == 0) {
      KET->tDitUnits = -0.1;
    }

    //PRINTF(", RBidx = %d    Duration = %dmS  Event = %s  tDitUnits = %f\r\n%s",
    //		  KET->score,
    //		  KET->Duration,
    //		  printStrMorseElement(KET->morseElement),
    //		  KET->tDitUnits,
    //		  newLine);
  }
}

char *keyPress::printStrMorseElement(morseElement_t E) {
  switch (E) {
    case morseDit:
      return (char *)"morseDit";
      break;
    case morseDah:
      return (char *)"morseDah";
      break;
    case morseMark:
      return (char *)"morseMark";
      break;
    case morseSpace:
      return (char *)"morseSpace";
      break;
    case morseWordSpace:
      return (char *)"morseWordSpace";
      break;
    case morseIdle:
      return (char *)"morseIdle";
      break;
    default:
      return (char *)"*Unkown*";
      break;
  }
}

bool keyPress::fillRingBuffer(void) {
  keyElementToken_t elementToken;

  while ((isfullRingBuffer != true) && (ElementStream.empty() == false)) {
    elementToken = ElementStream.front();
    ElementStream.pop();
    if ((elementToken.Event == Kidle) || (elementToken.Event == Kstuck)) {
      elementToken.Duration = 0;
      pushRingBuffer(elementToken);
      return true;
    } else {
      pushRingBuffer(elementToken);
    }
  }
  return false;
}

void keyPress::analyzeRingBuffer(void) {
  uint8_t countRingBuffer;
  keyElementToken_t *pElementToken;

  countRingBuffer = getCountRingBuffer();
  //Start off assuming slowest rate
  TshortestMark = TshortestDahDit = WPM2ms(5);
  haveMark = false;

  for (int idx = 0; idx < countRingBuffer; idx++) {
    pElementToken = peekRingBuffer(idx);
    if (pElementToken->Event == Kdown) {
      if (pElementToken->Duration < TshortestDahDit) {
        TshortestDahDit = pElementToken->Duration;
      }
    } else {
      if (pElementToken->Event == Kup) {
        haveMark = true;
        if (pElementToken->Duration < TshortestMark) {
          TshortestMark = pElementToken->Duration;
        }
      }
    }
  }
  if (haveMark == true) {
    currentTdit = TshortestMark;
  } else {
    currentTdit = WPM2ms(runningWPM);
  }
  // todo move this!
  if (!haveMark) {
    TshortestDahDit = WPM2ms(runningWPM);
  } else {
  }
}

void keyPress::assignMorseElements(void) {
  keyElementToken_t *pElementToken;
  uint8_t countRingBuffer;
  bool useFarnsworth = getFarnsworthSpacing();

  countRingBuffer = getCountRingBuffer();
  for (int idx = 0; idx < countRingBuffer; idx++) {
    pElementToken = peekRingBuffer(idx);

    if (pElementToken->Event == Kdown) {
      pElementToken->tDitUnits = (float)pElementToken->Duration
                                 / (float)TshortestDahDit;
    } else {
      pElementToken->tDitUnits = (float)pElementToken->Duration
                                 / (float)currentTdit;
    }

    switch (pElementToken->Event) {
      case Kup:
        if (pElementToken->tDitUnits < 4.0) {
          pElementToken->morseElement = morseMark;
        } else if (useFarnsworth == true) {
          if (pElementToken->Duration <= (WPM2ms(5) * 4)) {
            pElementToken->morseElement = morseSpace;
          } else {
            pElementToken->morseElement = morseWordSpace;
          }
        } else {
          if (pElementToken->tDitUnits < 7.0) {
            pElementToken->morseElement = morseSpace;
          } else {
            pElementToken->morseElement = morseWordSpace;
          }
        }
        break;
      case Kdown:
        if (pElementToken->tDitUnits <= 2.0) {
          pElementToken->morseElement = morseDit;
        } else {
          pElementToken->morseElement = morseDah;
        }
        break;
      case Kidle:
      case Kstuck:
        pElementToken->morseElement = morseIdle;
        break;
      default:
        break;
    }
  }
}


bool keyPress::spaceDetect(void) {
  keyElementToken_t *pElementToken;
  uint8_t countRingBuffer;

  countRingBuffer = getCountRingBuffer();
  for (int idx = 0; idx < countRingBuffer; idx++) {
    pElementToken = peekRingBuffer(idx);
    if ((pElementToken->morseElement == morseSpace) || (pElementToken->morseElement == morseWordSpace) || (pElementToken->morseElement == morseIdle)) {
      return true;
    }
  }
  return false;
}

void keyPress::processKeyEntry(void) {
  static uint16_t WPM;
  bool charComplete = false;
  bool sendSpace = false;
  static uint8_t seqIdx = 0;
  uint8_t charIdx;
  keyElementToken_t *pElementToken;
  uint8_t *msg;
  static FwdMorseSeq_t morseSeq = { 1, false, { Cdah, Cdit, Cdah, Cdit } };
  morseCharToken_t morseCharToken;
  uint8_t proIdx;
  char C;
  static bool haveInput = false;

  fillRingBuffer();
  analyzeRingBuffer();
  assignMorseElements();
  WPM = 1200 / TshortestDahDit;
  if (spaceDetect() == true) {
    charIdx = 0;

    while ((isMTRingBuffer == false) && (charComplete == false)) {
      pElementToken = pullRingBuffer();

      if (pElementToken != NULL) {
        printKeyElement(pElementToken);
      } else {
        //	PRINTF("pElementToken is NULL!\r\n");
      }
      morseCharToken.morseCharSeq[charIdx].morseElement = pElementToken->morseElement;
      morseCharToken.morseCharSeq[charIdx++].Duration = pElementToken->Duration;

      switch (pElementToken->morseElement) {
        case morseMark:
          break;
        case morseDit:
          morseSeq.elements[seqIdx++] = Cdit;
          haveInput = true;
          break;
        case morseDah:
          morseSeq.elements[seqIdx++] = Cdah;
          haveInput = true;
          break;
        case morseSpace:
        case morseWordSpace:
        case morseIdle:
          charComplete = true;
          if (pElementToken->morseElement == morseWordSpace) {
            sendSpace = true;
          }
          break;
        default:
          break;
      }

      if (charComplete && haveInput) {
        morseSeq.count = seqIdx;
        seqIdx = 0;
        if (!DecodeMorse(morseSeq, &C)) {
          // look for prosign
          if (lookupProsign(&morseSeq, &proIdx)) {
            morseCharToken.valid = true;
            morseCharToken.prosign = true;
            morseCharToken.morseChar = proIdx;
          } else {
            C = '!';  // Spanish inverted question mark
            morseCharToken.valid = false;
            morseCharToken.prosign = false;
          }
        } else {
          morseCharToken.valid = true;
          morseCharToken.prosign = false;
          morseCharToken.morseChar = C;
        }

        morseCharToken.Tdit = TshortestDahDit;
        morseCharToken.lengthSeq = charIdx;
        morseCharToken.farnsworth = getFarnsworthSpacing();
        calcWPM(&morseCharToken);
        //PRINTF("\r\nFarnsworth = %d TshortestDahDit = %d  TshortestMark = %d  WPM = %d Char = \'%c\'\r\n\n", morseCharToken.farnsworth? 1:0, TshortestDahDit,TshortestMark, WPM, C);
        ReceiveTextChar *RcvTxtChar = new ReceiveTextChar(&morseCharToken);
        msg = RcvTxtChar->getMsg();
        SendCQmessage(Queues, msg);
        delete RcvTxtChar;
        dsplyMorseLED(&morseCharToken);
      }
      if (sendSpace) {
        //PRINTF("\r\n<SPACE>\r\n");
      }
    }
  }
}

void keyPress::calcWPM(morseCharToken_t *pMorseCharToken) {
  uint32_t totalDuration = 0;
  uint32_t numberDits = 0;

  if (pMorseCharToken->valid == true) {
    for (int idx = 0; idx < pMorseCharToken->lengthSeq; idx++) {
      morseCharSeq_t *pMorseCharSeq = &pMorseCharToken->morseCharSeq[idx];
      switch (pMorseCharSeq->morseElement) {
        case morseDit:
          totalDuration += pMorseCharSeq->Duration;
          numberDits += 1;
          break;
        case morseDah:
          totalDuration += pMorseCharSeq->Duration;
          numberDits += 3;
          break;
        case morseMark:
          totalDuration += pMorseCharSeq->Duration;
          numberDits += 1;
          break;
        case morseSpace:
        case morseWordSpace:
        case morseIdle:
          break;

        default:
          break;
      }
    }

    WPM = MS2wpm(totalDuration / numberDits);
    if (getFarnsworthSpacing() == true) {
      idleTimeout = WPM2ms(5) * 10;
    } else {
      idleTimeout = WPM2ms(WPM) * 10;
    }
    runningWPM += WPM;
    runningWPM /= 2;
    runningWPM = std::min(runningWPM, maximumWPM);
  }
}

void keyPress::dsplyMorseLED(morseCharToken_t *pMorseCharToken) {
  uint8_t elementIdx = 0;
  if (pMorseCharToken->valid == true) {
    scrollLEDmatrix();
    for (int idx = 0; idx < pMorseCharToken->lengthSeq; idx++) {
      morseCharSeq_t *pMorseCharSeq = &pMorseCharToken->morseCharSeq[idx];
      switch (pMorseCharSeq->morseElement) {
        case morseDit:
          setLEDmatrix(7, elementIdx++, 1);
          break;
        case morseDah:
          setLEDmatrix(7, elementIdx++, 1);
          setLEDmatrix(7, elementIdx++, 1);
          setLEDmatrix(7, elementIdx++, 1);
          break;
        case morseMark:
          setLEDmatrix(7, elementIdx++, 0);
          break;
        case morseSpace:
        case morseWordSpace:
          setLEDmatrix(7, elementIdx++, 0);
          setLEDmatrix(7, elementIdx++, 0);
          setLEDmatrix(7, elementIdx++, 0);
          setLEDmatrix(7, elementIdx++, 0);
          setLEDmatrix(7, elementIdx++, 0);
          break;
        case morseIdle:
          break;
        default:
          break;
      }
    }
    setLEDmatrix(0, 11, false);
    dsplyLEDmatrix();
  }
}