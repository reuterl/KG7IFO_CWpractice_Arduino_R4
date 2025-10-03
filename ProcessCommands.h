/*
 * ProcessCommands.h
 *
 *  Created on: May 3, 2023
 *      Author: luke
 */

#ifndef PROCESSCOMMANDS_H_
#define PROCESSCOMMANDS_H_

#include "compileoptions.h"
#include "Qcontainer.h"
#include "AudioToneGen.h"
#include "SerialWaveType.h"
#include "msggenerator.h"
#include "MorseChar.h"
#include "keyPress.h"
#include "CWcommUDP.h"

#define SPRINTF(FMT, args...) \
  sprintf(sSprintf, FMT, args); \
  Serial.print(sSprintf);

/*---------------------------------------------------------------------------*/
uint8_t SendCQmessage(Qcontainer *Queues, uint8_t *Msg);

void ProcessCommands(AudioToneGen *_ATGen, Qcontainer *_Queues);
void ProcessReplySendConfig(void);

void ProcessCQcmmd(uint8_t *Cmmd);
#if COMM_PATH_SERIAL == 1
void InitProcessCQ(AudioToneGen *_ATGen, MorseChar *_morsechar, Qcontainer *_Queues);
#endif
#if COMM_PATH_WIFI == 1
void InitProcessCQ(AudioToneGen *_ATGen, MorseChar *_morsechar, Qcontainer *_Queues, CWcommUDP *_commUDP);
#endif
void ProcessReplyMessages(void);
void ProcessRestartAnnounce(uint8_t numTones);

void setPingCount(uint8_t count);
uint8_t getPingCount(void);
void setPingTrigger(bool triggerState);
bool getPingTrigger(void);

#endif /* PROCESSCOMMANDS_H_ */
