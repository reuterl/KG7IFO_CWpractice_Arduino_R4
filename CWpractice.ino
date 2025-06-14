
#include "AudioToneGen.h"
#include "FspTimer.h"
#include <SoftwareSerial.h>
#include "TriodeGirlDAC.h"
#include "machinedetat.h"
#include "SerialWaveType.h"
#include "ProcessCommands.h"
#include "Qcontainer.h"

#include "MorseChar.h"
#include "keyPress.h"

timerHook pTimerHook = NULL;

TriodeGirlDAC TgirlDAC;

// Debug output
SoftwareSerial SwSerial(2, 3);
char sSprintf[256];

machinedetat* MDE;
AudioToneGen* ATGen;
Qcontainer Queues;
keyPress* keyPressHandler;
MorseChar* morseChar;

uint8_t numTones = 8;  // Size of tone table

FspTimer NyquistTimer;
uint8_t gpioOut = 0;
const float nyquistFrequency = 44000.0f;

/*---------------------------------------------------------------------------*/
void NyquistCallback(timer_callback_args_t __attribute((unused)) * pArgs) {
  int32_t valueDAC;

  valueDAC = 0x00000FFF & ATGen->mixer();
  TgirlDAC.writeDAC(valueDAC);

  if (pTimerHook != NULL){
    pTimerHook();
  }
}

/*---------------------------------------------------------------------------*/
bool beginNyquistTime(float rate) {
  uint8_t timer_type = GPT_TIMER;

  int8_t tindex = FspTimer::get_available_timer(timer_type);
  if (tindex < 0) {
    tindex = FspTimer::get_available_timer(timer_type), true;
  }
  if (tindex < 0) {
    return false;
  }
  sprintf(sSprintf, "timer tindex = %d\n", tindex);
  Serial.print(sSprintf);

  FspTimer::force_use_of_pwm_reserved_timer();

  if (!NyquistTimer.begin(TIMER_MODE_PERIODIC, timer_type, tindex, rate, 0.0f, NyquistCallback)) {
    return false;
  }

  if (!NyquistTimer.setup_overflow_irq()) {
    return false;
  }

  if (!NyquistTimer.open()) {
    return false;
  }

  if (!NyquistTimer.start()) {
    return false;
  }

  return true;
}


/*---------------------------------------------------------------------------*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  // Initialize serial communication at a baud rate of 115200
  SwSerial.begin(9600);
  
  Serial.println("KG7IFO CW Practice Trainer/Fist Analyzer.  Arduino Uno R3 WiFi version. Dim brightness, please.");
  
  TgirlDAC.setup_dac();

  MDE = new machinedetat(&Queues);
  ATGen = new AudioToneGen(numTones, nyquistFrequency);

  ATGen->setCommModeAmpl(0.1);  // reduce clipping on audio amp

  ATGen->setTone(0, 1.0f, 440.0f, 0.0f, SerialWaveType::t_codeEnum::Sine);
  ATGen->setHandle(0, ATGen->generateHandle());

  ATGen->setTone(1, 1.0f, 350.0f, 0.0f, SerialWaveType::t_codeEnum::Sine);
  ATGen->setHandle(1, ATGen->generateHandle());

  ATGen->setTone(2, 1.0f, 700.0f, 0.0f, SerialWaveType::t_codeEnum::Sine);
  ATGen->setHandle(2, ATGen->generateHandle());
  ATGen->Sync();

  if (beginNyquistTime(nyquistFrequency)) {
    Serial.println("Nyquist Timer setup complete!");
  }

  morseChar = new MorseChar(ATGen);
  keyPressHandler = new keyPress(ATGen, &Queues);

  InitProcessCQ(ATGen, morseChar, &Queues);

  ProcessRestartAnnounce(numTones);
}

/*---------------------------------------------------------------------------*/
void loop() {
  uint8_t inputByte;
  // put your main code here, to run repeatedly:
  while (SwSerial.available()) {
    inputByte = SwSerial.read();
    MDE->msgParser(inputByte);
  }

  ProcessReplayMessages();

  if (morseChar->soundMorseMsg() == false) {
  } else {
    keyPressHandler->getElement();
    keyPressHandler->processKeyEntry();
  }
}
