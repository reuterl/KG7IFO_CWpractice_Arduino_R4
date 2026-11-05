
#include "compileoptions.h"
#include "AudioToneGen.h"
#include "FspTimer.h"
#include <SoftwareSerial.h>
#include "TriodeGirlDAC.h"
#include "machinedetat.h"
#include "SerialWaveType.h"
#include "ProcessCommands.h"
#include "Qcontainer.h"
#include "CWcommUDP.h"

#include "MorseChar.h"
#include "keyPress.h"
#include "BuzzWave.h"
#include "mySPRINTF.h"
#include "susan_ra4m1_minima_register_defines.h"


extern const uint16_t buzzWaveform[];
//const uint16_t * pBuzz[] = buzzWaveform[0];
extern uint32_t buzzSampleCount;

timerHook pTimerHook = NULL;
timerHook keyMonTimerHook = NULL;

TriodeGirlDAC TgirlDAC;

// Debug output
//SoftwareSerial SwSerial(2, 3);
SoftwareSerial SwSerial(3, 4);
char sSprintf[256];  // used worldwide by the SPRINTF #define (in (mySPRINTF.h)

machinedetat* MDE;
AudioToneGen* ATGen;
Qcontainer Queues;
keyPress* keyPressHandler;
MorseChar* morseChar;
#if COMM_PATH_WIFI == 1
CWcommUDP* commUDP;
#endif

uint8_t numTones = 8;  // Size of tone table

FspTimer* NyquistTimer;

uint8_t gpioOut = 0;
const float nyquistFrequency = 44100.0f;

/*---------------------------------------------------------------------------*/
const uint8_t keyDiscretePin = 2;  // P104, Uno WiFi
void KeyEdgeDetect_ISR() {
  static bool inDebounce = false;
  static uint32_t now, tLastEvent = 0;
  static PinStatus pin;
  static st_keyEvt keyEvent;
  static bool ignore;
  const uint8_t debounceInterval = 20;  // mS
  
  now = millis(); // capture event time.

  if (inDebounce) {
    ignore = ((now - tLastEvent) < debounceInterval);
  } else {
    ignore = false;
  }

  if (!ignore) {
    pin = digitalRead(keyDiscretePin);
    keyEvent.evt = (pin == HIGH) ? EdgeUp : EdgeDn;
    keyEvent.evtTime = now;
    Queues.EdgeDetectStream.push(keyEvent);
    inDebounce = true;
    tLastEvent = now;
  }
}

/*---------------------------------------------------------------------------*/
void NyquistCallback(timer_callback_args_t __attribute((unused)) * pArgs) {
  int32_t valueDAC;

  valueDAC = 0x00000FFF & ATGen->mixer();
  TgirlDAC.writeDAC(valueDAC);
  // used by LED matrix
  if (pTimerHook != NULL) {
    pTimerHook();
  }
}

/*---------------------------------------------------------------------------*/
FspTimer* createTimer(float rate, GPTimerCbk_f cbk) {
  uint8_t timer_type = GPT_TIMER;

  FspTimer* newTimer = new FspTimer;
  if (newTimer == NULL) {
    SPRINTF("Could not create new FspTimer object.\n");
    return NULL;
  }

  newTimer->force_use_of_pwm_reserved_timer();

  int8_t tindex = newTimer->get_available_timer(timer_type);
  if (tindex < 0) {
    tindex = newTimer->get_available_timer(timer_type), true;
  }

  if (tindex < 0) {
    SPRINTF("!!Could not get available timer index.\n");
    delete newTimer;
    return NULL;
  }

  sprintf(sSprintf, "timer tindex = %d\n", tindex);
  Serial.print(sSprintf);


  if (!newTimer->begin(TIMER_MODE_PERIODIC, timer_type, tindex, rate, 0.0f, cbk)) {
    return NULL;
  }

  if (!newTimer->setup_overflow_irq()) {
    return NULL;
  }

  if (!newTimer->open()) {
    return NULL;
  }

  if (!newTimer->start()) {
    return NULL;
  }
  return newTimer;
}

/*---------------------------------------------------------------------------*/
/*===========================================================================*/
/*---------------------------------------------------------------------------*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  // Initialize serial communication at a baud rate of 115200

#if COMM_PATH_SERIAL == 1
  char commType[] = "Serial";
#elif COMM_PATH_WIFI == 1
  char commType[] = "WiFi";
#else
  char commType[] = "!INVALID!";
#endif
  Serial.print("WZ7CHR CW Practice Trainer/Fist Analyzer.  Arduino Uno R3 WiFi version. UI Communications: ");
  Serial.print(commType);
  Serial.println(". Dim brightness, please.\n");

#if COMM_PATH_SERIAL == 1
  SwSerial.begin(9600);
#endif

  MDE = new machinedetat(&Queues);

#if COMM_PATH_WIFI == 1
  commUDP = new CWcommUDP(2390, MDE);
  commUDP->setupUDP();
#endif


  TgirlDAC.setup_dac();

  // Setup interrupt pin.
  //ISR will detect rising and falling edges and capture event time.
  // CHANGE status triggers on rise/fall.  ISR determines which.
  pinMode(keyDiscretePin, INPUT);
  pinMode(keyDiscretePin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(keyDiscretePin), KeyEdgeDetect_ISR, CHANGE);
  // Interrupt Control Unit  IRQ ctrl register, set digital filter bit 'on' to suppress
  // bounce while in ISR.
  // set FCLKSEL[1:0] = 3, PCLKB/64 RA4M1 Datasheet (13.2.1)
  *ICU_IRQCR01 |= 0xB0;
  SPRINTF("A(IRQCR01) = 0x%08X IRQCR01 = 0x%02x\n",  ICU_IRQCR01, *ICU_IRQCR01);

  // setup side-tone generator
  ATGen = new AudioToneGen(numTones, nyquistFrequency);

  ATGen->setCommModeAmpl(0.1);  // reduce clipping on audio amp

  ATGen->setTone(0, 1.0f, 440.0f, 0.0f, SerialWaveType::t_codeEnum::Sine);
  ATGen->setHandle(0, ATGen->generateHandle());

  ATGen->setTone(1, 1.0f, 350.0f, 0.0f, SerialWaveType::t_codeEnum::Sine);
  ATGen->setHandle(1, ATGen->generateHandle());

  ATGen->setTone(2, 1.0f, 700.0f, 0.0f, SerialWaveType::t_codeEnum::Sine);
  ATGen->setHandle(2, ATGen->generateHandle());

  ATGen->setSound(3, &buzzWave);
  ATGen->setHandle(3, ATGen->generateHandle());

  ATGen->Sync();

  NyquistTimer = createTimer(nyquistFrequency, NyquistCallback);
  if (NyquistTimer != NULL) {
    SPRINTF("NyquistCallback timer setup complete.\n");
  } else {
    SPRINTF("!!Could not allocate NyquistCallback timer.\n");
  }

  morseChar = new MorseChar(ATGen);
  keyPressHandler = new keyPress(ATGen, &Queues);

#if COMM_PATH_SERIAL == 1
  InitProcessCQ(ATGen, morseChar, &Queues);
#endif
#if COMM_PATH_WIFI == 1
  InitProcessCQ(ATGen, morseChar, &Queues, commUDP);
#endif

  ProcessRestartAnnounce(numTones);
}

/*---------------------------------------------------------------------------*/
void loop() {
  uint8_t inputByte;
  // put your main code here, to run repeatedly:
#if COMM_PATH_SERIAL == 1
  while (SwSerial.available()) {
    inputByte = SwSerial.read();
    MDE->msgParser(inputByte);
  }
#endif
#if COMM_PATH_WIFI == 1
  commUDP->udpRcvStream();
#endif
#if COMM_PATH_SERIAL == 1
  ProcessReplyMessages();
#endif

  if (morseChar->soundMorseMsg() == false) {
  } else {
    keyPressHandler->getElement();
    keyPressHandler->processKeyEntry();
  }
}
