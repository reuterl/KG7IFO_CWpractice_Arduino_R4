#include <stdint.h>
#include "ProcessCommands.h"
#include "msggenerator.h"
#include <stdio.h>
#include <SoftwareSerial.h>

static AudioToneGen *ATGen;
static MorseChar *morseChar;
static keyPress *keyPressHandler;
static Qcontainer *Queues;
static CWcommUDP *commUDP;

extern SoftwareSerial SwSerial;
MsgUtil msgUtil;

extern char sSprintf[];
#define SPRINTF(FMT, args...) \
  sprintf(sSprintf, FMT, args); \
  Serial.print(sSprintf);

/*---------------------------------------------------------------------------*/
void ProcessRestartAnnounce(uint8_t numTones) {
  RestartAnnounced *RA = new RestartAnnounced(numTones);
  SendCQmessage(Queues, RA->getMsg());
  delete RA;
}
/*---------------------------------------------------------------------------*/
#if COMM_PATH_SERIAL == 1
void ProcessReplyMessages(void) {
  while (!Queues->XmitStream.empty()) {
    uint8_t byte = Queues->XmitStream.front();
    Queues->XmitStream.pop();
    SwSerial.write(byte);
  }
}
#endif
/*---------------------------------------------------------------------------*/
void ProcessReplySendConfig(void) {
  uint8_t *msg;
  uint8_t i;
  uint8_t toneKount = 0;
  uint8_t addListIdx = 0;

  // Count up defined tones
  for (i = 0; i < ATGen->getNumTones(); i++) {
    ATGen->getDefined(i) ? toneKount++ : toneKount;
  }

  SendConfig *SC = new SendConfig(toneKount);

  Serial.println("ProcessReplySendConfig()");

  SC->AddCommonModeA(ATGen->getCommModeAmpl());
  SC->AddNyquistFrequency(ATGen->getNyquist());

  for (uint8_t J = 0; J < ATGen->getNumTones(); J++) {
    //sprintf(sSprintf,"toneList(%02d)\n",J);
    //Serial.print(sSprintf);
    if (ATGen->getDefined(J)) {
      //Serial.println("Defined");
      SC->AddListEntry(addListIdx++,
                       J,
                       ATGen->getHandle(J),
                       ATGen->getWaveformType(J),
                       ATGen->getAmplitude(J),
                       ATGen->getFreq(J),
                       ATGen->getPhase(J),
                       ATGen->getNsamples(J),
                       ATGen->getStatus(J),
                       ATGen->getDefined(J),
                       ATGen->getEnabled(J));
    }
  }

  SC->FinalizeMsg();
  msg = SC->getMsg();
  SendCQmessage(Queues, msg);
  delete SC;
  Serial.println("end ProcessReplySendConfig()");
}
/*---------------------------------------------------------------------------*/
void ProcessDelTone(uint8_t *CmmdMsg) {
  uint8_t *msg;
  uint8_t Handle;
  uint8_t updateIndex;

  Serial.println("ProcessDelTone");

  msgUtil.dsplMsg(CmmdMsg);

  deleteTone *DT = new deleteTone(CmmdMsg);
  Handle = DT->getHandle();
  updateIndex = ATGen->findHandle(Handle);

  sprintf(sSprintf, "Delete Tone handle %02d from table index %d\n", Handle, updateIndex);
  Serial.print(sSprintf);
  ATGen->clearTone(updateIndex);
  delete DT;

  SendConfig *SC = new SendConfig(1);
  SC->AddCommonModeA(ATGen->getCommModeAmpl());
  SC->AddNyquistFrequency(ATGen->getNyquist());

  SC->AddListEntry(0,
                   updateIndex,
                   ATGen->getHandle(updateIndex),
                   ATGen->getWaveformType(updateIndex),
                   ATGen->getAmplitude(updateIndex),
                   ATGen->getFreq(updateIndex),
                   ATGen->getPhase(updateIndex),
                   ATGen->getNsamples(updateIndex),
                   ATGen->getStatus(updateIndex),
                   ATGen->getDefined(updateIndex),
                   ATGen->getEnabled(updateIndex));


  SC->FinalizeMsg();
  msg = SC->getMsg();
  SendCQmessage(Queues, msg);
  delete SC;
}
/*---------------------------------------------------------------------------*/
void ProcessAddTone(uint8_t *CmmdMsg) {
  uint8_t *msg;
  addTone *AddTone = new addTone(CmmdMsg);
  float ampl = AddTone->getAmpl();
  float freq = AddTone->getFreq();
  float phase = AddTone->getPhase();
  uint8_t Handle = AddTone->getHandle();
  uint8_t Index;

  SerialWaveType::t_codeEnum waveform = AddTone->getWaveFormType();

  Index = ATGen->findHandle(Handle);
  if (!ATGen->setTone(Index, ampl, freq, phase, waveform)) {
    // Could not create/edit the tone. Probably out of memory
    /*
    RespondCommand *RC = new RespondCommand(Handle, 0xE0);
    msg = RC->getMsg();
    SendCQmessage(Queues, msg);
    delete RC;
    return;
    */
  }
  ATGen->Sync();
  sprintf(sSprintf, "Table Location(%d): ProcessAddTone(%02d, %2.3f, %2.3f, %2.3f, 0x%02X\n", Index, Handle, ampl, freq, phase, waveform);
  Serial.print(sSprintf);
  delete AddTone;

  SendConfig *SC = new SendConfig(1);
  SC->AddCommonModeA(ATGen->getCommModeAmpl());
  SC->AddNyquistFrequency(ATGen->getNyquist());
  SerialWaveType *SWT = new SerialWaveType();
  SC->AddListEntry(0,
                   Index,
                   ATGen->getHandle(Index),
                   SWT->getWaveformEnum(ATGen->getWaveformType(Index)),
                   ATGen->getAmplitude(Index),
                   ATGen->getFreq(Index),
                   ATGen->getPhase(Index),
                   ATGen->getNsamples(Index),
                   ATGen->getStatus(Index),
                   ATGen->getDefined(Index),
                   ATGen->getEnabled(Index));
  sprintf(sSprintf, "Returned waveform type: 0x%02X\n", ATGen->getWaveformType(Index));
  Serial.print(sSprintf);
  delete SWT;
  // Send command response. Status = 00, indicates hunky-dorey
  /*
  RespondCommand * RC = new RespondCommand(Handle, 0x00);
  msg = RC->getMsg();
  SendCQmessage(Queues, msg);
  delete RC;
  */

  SC->FinalizeMsg();
  msg = SC->getMsg();
  SendCQmessage(Queues, msg);
  delete SC;
  return;
}
/*---------------------------------------------------------------------------*/
void AcheiveEnableDisable(uint8_t Handle, bool Enable, bool All) {
  uint8_t Index, Kount, SCsize, nDefined;
  uint8_t *msg;

  sprintf(sSprintf, "Enable/Disable, Handel=%02d  Enable=%d  ALL=%d\n", Handle, Enable, All);
  Serial.print(sSprintf);

  if (!All) {
    Index = ATGen->findHandle(Handle);
    ATGen->setEnabled(Index, Enable);
    Kount = Index + 1;
    nDefined = 1;
    SCsize = 1;
  } else {
    nDefined = 0;
    for (Index = 0; Index < ATGen->getNumTones(); Index++) {
      if (ATGen->getDefined(Index)) {
        nDefined++;
        ATGen->setEnabled(Index, Enable);
      }
    }
    Index = 0;
    Kount = ATGen->getNumTones();
    SCsize = ATGen->getNumTones();
  }

  SendConfig *SC = new SendConfig(nDefined);

  SC->AddCommonModeA(ATGen->getCommModeAmpl());
  SC->AddNyquistFrequency(ATGen->getNyquist());
  uint8_t scIndex = 0;
  for (Index; Index < Kount; Index++) {
    if (ATGen->getDefined(Index)) {
      SC->AddListEntry(scIndex++,
                       Index,
                       ATGen->getHandle(Index),
                       ATGen->getWaveformType(Index),
                       ATGen->getAmplitude(Index),
                       ATGen->getFreq(Index),
                       ATGen->getPhase(Index),
                       ATGen->getNsamples(Index),
                       ATGen->getStatus(Index),
                       ATGen->getDefined(Index),
                       ATGen->getEnabled(Index));
    }
  }
  SC->FinalizeMsg();
  msg = SC->getMsg();
  SendCQmessage(Queues, msg);
  delete SC;
  return;
}
/*---------------------------------------------------------------------------*/
void ProcessEnableTone(uint8_t *CmmdMsg) {
  uint8_t *msg;
  enableTone *EnableTone = new enableTone(CmmdMsg);
  uint8_t Handle = EnableTone->getHandle();
  bool All = EnableTone->getAll();

  delete EnableTone;

  AcheiveEnableDisable(Handle, true, All);
}

/*---------------------------------------------------------------------------*/
void ProcessDisableTone(uint8_t *CmmdMsg) {
  uint8_t *msg;
  disableTone *DisableTone = new disableTone(CmmdMsg);
  uint8_t Handle = DisableTone->getHandle();
  bool All = DisableTone->getAll();

  delete DisableTone;

  AcheiveEnableDisable(Handle, false, All);
}

/*---------------------------------------------------------------------------*/
void ProcessReqConfig(uint8_t *CmmdMsg) {
  ProcessReplySendConfig();
}

void ProcessSendMorseMsg(uint8_t *CmmdMsg) {
  SendMorseMsg *SMM = new SendMorseMsg(CmmdMsg);
  morseChar->setMorseMsg((char *)SMM->getTextMsg());
  delete SMM;
}

void ProcessPlayMorseMsg(uint8_t *CmmdMsg) {
  PlayMorseMsg *PMM = new PlayMorseMsg(CmmdMsg);
  morseChar->setWPM(PMM->getWPM());
  morseChar->setSendMorse();
  delete PMM;
}

void ProcessStopMorseMsg(uint8_t *CmmdMsg) {
  morseChar->stopPlayMorseMsg();
}

void ProcessSendSidetone(uint8_t *CmmdMsg) {
  SendSidetone *SST = new SendSidetone(CmmdMsg);
  ATGen->setFreq(2, SST->getSidetone());
  delete SST;
}

void ProcessSendFarnsworth(uint8_t *CmmdMsg) {
  SendFarnsworth *SFW = new SendFarnsworth(CmmdMsg);
  morseChar->setFarnsworthSpacing(SFW->getFarnsworth());
  delete SFW;
}
/*---------------------------------------------------------------------------*/
void ProcessReqTableSize(uint8_t *CmmdMsg) {
  uint8_t *msg;
  uint8_t tableSize = ATGen->getNumTones();
  ReturnTableSize *RTS = new ReturnTableSize(tableSize, true);
  msg = RTS->getMsg();
  SendCQmessage(Queues, msg);
  delete RTS;
}

/*---------------------------------------------------------------------------*/
void ProcessReqHandle(uint8_t *CmmdMsg) {
  uint8_t *msg;
  uint8_t available;
  uint8_t handle;
  ReturnHandle *RTH;

  available = ATGen->findAvailable();
  if (available != 0xFF) {
    ATGen->setDefined(available, true);
    handle = ATGen->generateHandle();
    ATGen->setHandle(available, handle);
    ATGen->setEnabled(available, false);  // Init to disabled
    sprintf(sSprintf, "ProcessReqHandle found handle %02d at table index %d\n", handle, available);
    Serial.print(sSprintf);
    RTH = new ReturnHandle(handle, true);
  } else {
    RTH = new ReturnHandle(0xFF, false);
  }
  msg = RTH->getMsg();
  SendCQmessage(Queues, msg);
  delete RTH;
}

/*---------------------------------------------------------------------------*/
static bool pingTrigger = true;
static uint8_t pingCount = 1;
static uint8_t startPingCount = 1;
void setPingCount(uint8_t count) {
  pingCount = count;
  startPingCount = count;
}
uint8_t getPingCount(void) {
  return pingCount;
}
void setPingTrigger(bool triggerState) {
  pingTrigger = triggerState;
  if (pingTrigger){
    pingCount = startPingCount;
  }
}
bool getPingTrigger(void) {
  return pingTrigger;
}
void ProcessPing(uint8_t *CmmdMsg) {
  if (pingTrigger) {
    ping *PING = new ping();
    PING->rcvPing(CmmdMsg);
    SendCQmessage(Queues, PING->echoPing());
    pingCount--;
    if (pingCount == 0) {
      pingTrigger = false;
      pingCount = startPingCount;
    }
    delete PING;
  }
}

void ProcessCmmdCode(uint8_t *CmmdMsg) {
  uint8_t CmmdCode;

  CmmdCode = CmmdMsg[3];

  switch (CmmdCode) {
    case 0xC0:
      ProcessAddTone(CmmdMsg);
      break;
    case 0xC1:
      ProcessEnableTone(CmmdMsg);
      break;
    case 0xC2:
      ProcessDisableTone(CmmdMsg);
      break;
    case 0xC3:
      ProcessReqConfig(CmmdMsg);
      break;
    case 0xD0:
      ProcessSendMorseMsg(CmmdMsg);
      break;
    case 0xD1:
      ProcessPlayMorseMsg(CmmdMsg);
      break;
    case 0xD2:
      ProcessSendSidetone(CmmdMsg);
      break;
    case 0xD3:
      ProcessSendFarnsworth(CmmdMsg);
      break;
    case 0xD4:
      ProcessStopMorseMsg(CmmdMsg);
      break;
    case 0xAA:
      ProcessPing(CmmdMsg);
      break;
    default:
      //PRINTF("\r\nBad Command Code 0x%02X\r\n", CmmdCode);
      break;
  }
}

void ProcessCQcmmd(uint8_t *Cmmd) {
  ProcessCmmdCode(Cmmd);
  //ProcessReplySendConfig();
}
/*---------------------------------------------------------------------------*/

#if COMM_PATH_SERIAL == 1
void InitProcessCQ(AudioToneGen *_ATGen,
                   MorseChar *_morsechar,
                   Qcontainer *_Queues) {
  Queues = _Queues;
  morseChar = _morsechar;
  ATGen = _ATGen;
}
#endif
#if COMM_PATH_WIFI == 1
void InitProcessCQ(AudioToneGen *_ATGen,
                   MorseChar *_morsechar,
                   Qcontainer *_Queues,
                   CWcommUDP *_commUDP) {
  Queues = _Queues;
  morseChar = _morsechar;
  ATGen = _ATGen;
  commUDP = _commUDP;
}
#endif
/*---------------------------------------------------------------------------*/
void ProcessCommands(AudioToneGen *_ATGen, Qcontainer *_Queues) {
  uint8_t *MSG;
  uint8_t CmmdMsg[128];
  uint8_t length;

  //Queues = _Queues;

  ATGen = _ATGen;

  if (!Queues->MsgQ.empty()) {
    MSG = (uint8_t *)Queues->MsgQ.front();
    Queues->MsgQ.pop();
    length = MSG[2];
    if (length > sizeof(CmmdMsg)) {
      Serial.print("\r\nMessage too long, length = %d\r\n");  //, length);
      delete MSG;
      return;
    }
    memmove(CmmdMsg, MSG, length);
    SendCQmessage(Queues, CmmdMsg);
  }
}

/*---------------------------------------------------------------------------*/
uint8_t SendCQmessage(Qcontainer *Queues, uint8_t *Msg) {
  uint8_t Length = Msg[2];

  Serial.println("SendCQmessage()");
  msgUtil.dsplMsg(Msg);
#if COMM_PATH_SERIAL == 1
  for (uint8_t L = 0; L < Length; L++) {
    //sprintf(sSprintf, "XmitStream.push(0x%02X)\n", Msg[L]);
    //Serial.print(sSprintf);
    Queues->XmitStream.push((uint8_t)Msg[L]);
  }
#endif

#if COMM_PATH_WIFI == 1
  commUDP->writePacket(Msg, Length);
#endif

  return Length;
}
