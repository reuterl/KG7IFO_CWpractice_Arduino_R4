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
  countStuck = 0;
  stuckLatched = false;
  stuckDetected = false;

  WPM = 5;

  // Tdit units
  thresholdTimeMark = 3.5;
  thresholdTimeWordSpace = 7.0;
  thresholdTimeDit = 2.6;

  timingMorseSpace = 3;
  timingMorseWordSpace = 5;
  Queues = _Queues;
  initialRingBuffer();
}

keyPress::~keyPress() {
  // TODO Auto-generated destructor stub
  Serial.println("KeyPress: Destructor");
} /*
 Low-level inteface to key GPIO.
 */


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
/*
  Debounce key strokes. procede from initial state, KBD_Debounce_Start,
  to key up/down state (always down??). stay in debounce until succesive
  iterations in same state reach debounceInterval. Then proceede to up/down
  state and stay until key moves again.

  set externally viewable keyState_st to up, down, TBD to indicate final
  position of key.

 */
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
          keyState_st = keyStateTBD;
        } else {
          keyDebounce_st = KDB_Down;
          keyState_st = keyStateDown;
        }
      } else {
        keyDebounce_st = KDB_Debounce_Dn;  // stay in state throught debounceInterval
      }
      break;

    case KDB_Debounce_Up:
      if (senseKeyPos() == keyUp) {
        if (getElapsed(2) < debounceInterval) {
          keyDebounce_st = KDB_Debounce_Up;
          keyState_st = keyStateTBD;
        } else {
          keyDebounce_st = KDB_Up;
          keyState_st = keyStateUp;
        }
      } else {
        keyDebounce_st = KDB_Debounce_Up;  // stay in state throught debounceInterval
      }
      break;

    case KDB_Down:
      Duration = getElapsed(2);
      if (senseKeyPos() == keyDown) {
        keyDebounce_st = KDB_Down;
      } else {
        startInterval(2);
        keyDebounce_st = KDB_Debounce_Up;
        keyState_st = keyStateTBD;
        lastDuration = Duration;
      }
      break;

    case KDB_Up:
      Duration = getElapsed(2);
      if (senseKeyPos() == keyUp) {
        keyDebounce_st = KDB_Up;
      } else {
        startInterval(2);
        keyDebounce_st = KDB_Debounce_Dn;
        keyState_st = keyStateTBD;
        lastDuration = Duration;
      }
      break;

    default:
      break;
  }
}
/*
  Sound off on key down.  Enable hard-coded tone #2 when key is being pressed.
  Key logic determines when key has been hald down too long resulting in a stuck
  key. Hard codeed tones 0 and 1, 350Hz and 440Hz, sound a dial tone.  When Spark gap
  is .TRUE.  enable hard coded sound #3, whic is an electric buzz sound extracted
  from a .WAV file.
   */
void keyPress::setSideTone(void) {
  switch (Element_st) {
    case elementStateDown:
      ATGen->setEnabled(0, false);
      ATGen->setEnabled(1, false);
      if (ATGen->getSparkGap()) {
        ATGen->setEnabled(3, true);
      } else {
        ATGen->setEnabled(2, true);
      }
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
      ATGen->setEnabled(3, false);
      break;
  }
}

// Accesor for debounced keyState_st
keyPress::keyState_t keyPress::getKeyState(void) {
  return keyState_st;
}

uint32_t keyPress::getDuration(void) {
  return lastDuration;
}
/*
 Wrapper for key element queue
  */
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

/*
  Convert stream of keyState tokens (Up, Dn) to stream of timed elements.
  Start interval timer(0) at each edge: idle-> down, down->up, up->down.
  Up intervals become marks, which later will be interpreted as character
  spaces, or word spaces.  Down intervals will become dits or dahs.  here,
  we push them into the elemntStream queue, which will later be buffered
  and parsed to determine word per minute speed and guess at the element
  types: mark (element space), character space, or word space, and downs 
  resolve to dits and dahs.  
  Special casees are idle: no key activity for a wile, and stuck key: 
  holding key down for long interval (stuckTimeout).
   */
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
            //Serial.println("[[Kidle]]");
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
        stuckDetected = false;  // clear edge detect on falling edge
      } else {
        // on stuck key rising edge, count occurance.
        // on two occurances raise stuckDetect.
        if (stuckDetected == false) {
          SPRINTF("countStuck = %d\n", countStuck);
          if (countStuck <= 1) {
            if (countStuck++ == 1) {
              stuckLatched = true;
            }
          }
          stuckDetected = true;
        }
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

/*
 debug/development print utility.
 */
void keyPress::printKeyElement(keyElementToken_t *KET) {
  char *newLine;
  char noExtra[] = "";
  char extra[] = "\n\n";

  if (KET != NULL) {
    newLine = noExtra;
    switch (KET->Event) {
      default:
      case keyPress::Kup:
        Serial.print("\r\nUP");
        break;
      case keyPress::Kdown:
        Serial.print("\r\nDOWN");
        break;
      case keyPress::Kidle:
        Serial.print("\r\nIDLE");
        newLine = extra;
        break;
      case keyPress::Kstuck:
        Serial.print("\r\nSTUCK");
        break;
        Serial.print("\r\n**unknown**");
        break;
    }

    if (KET->tDitUnits == 0) {
      KET->tDitUnits = -0.1;
    }

    SPRINTF(", RBidx = %d    Duration = %dmS  Event = %s  tDitUnits = %f\r\n%s",
            KET->score,
            KET->Duration,
            printStrMorseElement(KET->morseElement),
            KET->tDitUnits,
            newLine);
  }
}

/*
 debug/development print utility.
 */
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

/*
 Each iteration of main loop, pull stream of ups and downs from queue and
 fill ring buffer.  Later in iteration, peek at the queue and guess at
 element type (dit, dah, mark, space) by examining relative timing and trying
 to determin words per minute, WPM, and assigning type by morse code rules:
  dit, mark - 1 tdit.  dah, character space = 3 tdit, etc.
  */
bool keyPress::fillRingBuffer(void) {
  keyElementToken_t elementToken;
  bool pushElement = false;

  while ((isfullRingBuffer != true) && (ElementStream.empty() == false)) {
    elementToken = ElementStream.front();
    ElementStream.pop();
    if ((elementToken.Event == Kidle) || (elementToken.Event == Kstuck)) {
      elementToken.Duration = 0;
      if (elementToken.Event == Kidle) {
        //Serial.println("RB={{Kidle}}");
      }
    } else {
      // no-op
    }
    pushRingBuffer(elementToken);
    pushElement = true;
  }
  return pushElement;
}
/*
 debug/development print utility.
 */

void keyPress::dumpRingBuffer(void) {
  keyElementToken_t *pElementToken;
  char *CP;
  Serial.println("Ring Buffer:");
  if (isMTRingBuffer == true) {
    Serial.println("MT\n");

  } else {
    countRingBuffer = getCountRingBuffer();
    for (int idx = 0; idx < countRingBuffer; idx++) {
      pElementToken = peekRingBuffer(idx);
      SPRINTF("%02d *Event = ", getAbsRingIdx(pElementToken));
      switch (pElementToken->Event) {
        case Kup:
          Serial.print("Kup");
          break;
        case Kdown:
          Serial.print("Kdown");
          break;
        case Kstuck:
          Serial.print("Kstuck");

          break;
        case Kidle:
          Serial.print("Kidle");
          break;
        default:
          Serial.print("default");
          break;
      }

      SPRINTF("  morseElement = %s, ", printStrMorseElement(pElementToken->morseElement));
      SPRINTF("  Duration = %d, ", pElementToken->Duration);
      SPRINTF("  tDitUnits = %3.1f", pElementToken->tDitUnits);
      SPRINTF("  processed = %s\n", pElementToken->processedElement ? ".TRUE." : ".FALSE.")
    }
    Serial.println("");
  }
}
/*
 Estimate tDit by looking at what we've got in the buffer
 and select  first guess from key up/down interval that looks
 like it might be 1 tDit.  Using this, see if the longets down
 is way greater than 3 tDits.  if so, use it to make a tDit 
 based on it being  a dah and tDit is one third of that.  Maybe.
 */
uint16_t keyPress::guessTdit(void) {
  keyElementToken_t *pElementToken;
  uint8_t countRingBuffer;
  uint16_t lgDn = 0,
           lgMark = 0,
           smDn = 0xFFFF,
           smMark = 0xFFFF,
           firstTdit, secondTdit;


  countRingBuffer = getCountRingBuffer();
  for (uint8_t idx = 0; idx < countRingBuffer; idx++) {
    pElementToken = peekRingBuffer(idx);
    if (pElementToken->Event == Kup) {
      lgMark = max(lgMark, pElementToken->Duration);
      smMark = min(smMark, pElementToken->Duration);
    }
    if (pElementToken->Event == Kdown) {
      lgDn = max(lgDn, pElementToken->Duration);
      smDn = min(smDn, pElementToken->Duration);
    }
  }
  // first guess:
  if (smDn != 0) {
    firstTdit = smDn;
  } else {
    firstTdit = smMark;
  }
  //SPRINTF("firstTdit = %d\n", firstTdit);
  // If first try yeilds a dah that's much over 3 tdits,
  // scale tDit to longest dah/3
  if ((lgDn / firstTdit) > 4) {
    secondTdit = lgDn / 3;
  } else {
    secondTdit = firstTdit;
  }
  //SPRINTF("secondTdit = %d\n", secondTdit);

  return secondTdit;
}


bool keyPress::resolveElementType(keyElementToken_t *pElementToken, uint16_t tDit) {
  bool haveSpace = false;

  // Assign keydowns dit or dah status based on thresholds of dits near 1 and dahs near 3
  // Assign keyups mark, space or wordspace based on thresholds of mark near 1 space near three and wordspace over 5
  // keystuck and idle indicate end of sequence


  //sprintf(sSprintf, "Token event = %d\n"), pElementToken->Event;
  //Serial.print(sSprintf);
  //SPRINTF("resolveElementType using tDit = %d\n", tDit);
  pElementToken->tDitUnits = (float)pElementToken->Duration
                             / tDit;
  pElementToken->tDit = tDit;

  switch (pElementToken->Event) {
    case Kup:
      //Serial.println("Kup");
      countStuck = 0;  // needs two consectuive Kstuck. reset otherwise.

      if (pElementToken->tDitUnits < thresholdTimeMark) {
        pElementToken->morseElement = morseMark;
      } else if (getFarnsworthSpacing() == true) {
        if (pElementToken->Duration <= (WPM2ms(5) * 4)) {
          pElementToken->morseElement = morseSpace;
        } else {
          pElementToken->morseElement = morseWordSpace;
        }
      } else {
        if (pElementToken->tDitUnits < thresholdTimeWordSpace) {
          pElementToken->morseElement = morseSpace;
        } else {
          pElementToken->morseElement = morseWordSpace;
        }
      }
      if ((pElementToken->morseElement == morseWordSpace) || (pElementToken->morseElement == morseSpace)) {
        haveSpace = true;
      }
      break;

    case Kdown:
      //Serial.println("Kdown");
      countStuck = 0;  // needs two consectuive Kstuck. reset otherwise.
      if (pElementToken->tDitUnits <= thresholdTimeDit) {
        pElementToken->morseElement = morseDit;
        //sprintf(sSprintf, "morseDit %4.2f\n", pElementToken->tDitUnits);
        //Serial.print(sSprintf);
      } else {
        pElementToken->morseElement = morseDah;
        //SPRINTF("morseDah %4.2f\n", pElementToken->tDitUnits);
      }
      break;
    case Kstuck:
      Serial.println("@Kstuck");
      pElementToken->morseElement = morseStuck;
      break;
    case Kidle:
      Serial.println("@Kidle");
      pElementToken->morseElement = morseIdle;
      haveSpace = true;
      break;
    default:
      break;
  }
  return haveSpace;
}
/*
 Examine ring buffer and attempt to resolve key down intervals as
 dits or dahs, also try to reslove key ups as marks, character 
 spaces of word spaces based on guessed tDit.  
 */
bool keyPress::assignMorseElements(void) {

  keyElementToken_t *pElementToken;
  uint8_t countRingBuffer;
  bool haveSpace = false;
  uint16_t tDit, gTdit;
  bool haveMark = false;
  bool haveIdle = false;
  uint8_t countDowns = 0, countUps = 0;
  uint8_t aye;

  // Don't continuously re-evaluate
  if (unprocessedElementRingBuffer() == false) {
    return haveSpace;
  }

  //dumpRingBuffer();

  // guess at this iteration tDit
  gTdit = guessTdit();

  //SPRINTF("guessTdit = %d, WPM(%d)\n", gTdit, MS2wpm(gTdit));

  for (aye = 0; aye < getCountRingBuffer(); aye++) {
    pElementToken = peekRingBuffer(aye);
    if (pElementToken->Event == Kup) {
      haveMark = true;
      countUps++;
    }
    if (pElementToken->Event == Kdown) {
      countDowns++;
    }
    if (pElementToken->Event == Kidle) {
      haveIdle = true;
    }
  }
  // No mark yet implies Kdown followed by Kidle.  It's a solitary T, or an E
  // have to go with historical WPM rate.
  if (haveMark != true) {
    tDit = WPM2ms(runningWPM);
    //SPRINTF("*Using, running WPM = %d\n", runningWPM);
  } else {
    tDit = gTdit;
  }

  for (aye = 0; aye < getCountRingBuffer(); aye++) {
    //SPRINTF("aye = %d\n", aye);
    pElementToken = peekRingBuffer(aye);

    //printKeyElement(pElementToken);
    pElementToken->processedElement = true;
    switch (pElementToken->Event) {
      case Kup:
      case Kdown:
        haveSpace |= resolveElementType(pElementToken, tDit);
        break;
      case Kidle:
      case Kstuck:
        haveSpace |= resolveElementType(pElementToken, tDit);
        haveSpace = true;
        break;
      default:
        break;
    }
  }
  // If we only have two Kdowns so far and an idle isn't
  // in the works, clear haveSpace untill we can learn more
  // on the next go around.
  if (countDowns < 3) {
    if (haveIdle != true) {
      haveSpace = false;
    }
  }
  //if (haveSpace) {
  //  Serial.println("Have Space.");
  //}
  //Serial.println("<<exit assignMorseElements.>>");
  return haveSpace;
}
// ////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
/*
 Top of sequence parser.  Each main iteration, peek into ring buffer.  Try to determing tdit
 from elements at head of buffer.  if possible, assign dits and dahs according 
 to tdit.  when a character or word space is recognized (or idle) look up dit/dah 
 pattern in morse tree and transmit morse element message to user interface, along with timing.
 */
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
  if ((assignMorseElements() == true)) {
    charIdx = 0;

    while ((isMTRingBuffer == false) && (charComplete == false)) {
      pElementToken = pullRingBuffer();

      if (pElementToken != NULL) {
        //SPRINTF("  morseElement = %s, ", printStrMorseElement(pElementToken->morseElement));
        //SPRINTF("  tDit = %d, ", pElementToken->tDit);
        //SPRINTF("  Duration = %d, ", pElementToken->Duration);
        //SPRINTF("  tDitUnits = %3.1f", pElementToken->tDitUnits);
        //SPRINTF("  processed = %s\n", pElementToken->processedElement?".TRUE.":".FALSE.")

      } else {
        Serial.println("pElementToken is NULL!");
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
        case morseStuck:
          haveInput = false;
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
            C = '!';  // was Spanish inverted question mark (0xBF?)
            morseCharToken.valid = false;
            morseCharToken.prosign = false;
          }
        } else {
          morseCharToken.valid = true;
          morseCharToken.prosign = false;
          morseCharToken.morseChar = C;
        }

        morseCharToken.Tdit = pElementToken->tDit;
        morseCharToken.lengthSeq = charIdx;
        morseCharToken.farnsworth = getFarnsworthSpacing();
        calcWPM(&morseCharToken);
        //PRINTF("\r\nFarnsworth = %d TshortestDahDit = %d  TshortestMark = %d  WPM = %d Char = \'%c\'\r\n\n", morseCharToken.farnsworth? 1:0, TshortestDahDit,TshortestMark, WPM, C);
        ReceiveTextChar *RcvTxtChar = new ReceiveTextChar(&morseCharToken);
        msg = RcvTxtChar->getMsg();
        SendCQmessage(Queues, msg);
        delete RcvTxtChar;
#ifdef LED_MATRIX
        dsplyMorseLED(&morseCharToken);
#endif
      }
      if (stuckLatched == true) {
        Serial.println("<<STUCK detected.>>");
        stuckLatched = false;
        countStuck = 0;
        //  Reset running WPM to default (5)
        runningWPM = 5;
        uploadRunningWPM *RWPM = new uploadRunningWPM(runningWPM);
        SendCQmessage(Queues, RWPM->getMsg());
        delete RWPM;
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
    //runningWPM = std::min(runningWPM, maximumWPM);
    uploadRunningWPM *RWPM = new uploadRunningWPM(runningWPM);
    SendCQmessage(Queues, RWPM->getMsg());
    delete RWPM;
  }
}
#ifdef LED_MATRIX
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
#endif