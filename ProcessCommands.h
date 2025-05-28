/*
 * ProcessCommands.h
 *
 *  Created on: May 3, 2023
 *      Author: luke
 */

#ifndef PROCESSCOMMANDS_H_
#define PROCESSCOMMANDS_H_

#include "Qcontainer.h"
#include "AudioToneGen.h"
#include <SerialWaveType.h>
#include "msggenerator.h"
#include "MorseChar.h"
#include "keyPress.h"
/*---------------------------------------------------------------------------*/
void InitProcessCQ(AudioToneGen * _ATGen, MorseChar * _morsechar, Qcontainer * _Queues);
uint8_t SendCQmessage(Qcontainer *Queues, uint8_t *Msg);

void ProcessCommands (AudioToneGen * _ATGen, Qcontainer * _Queues);
void ProcessReplySendConfig(void);

void ProcessCQcmmd(uint8_t * Cmmd);
void InitProcessCQ(AudioToneGen * _ATGen, Qcontainer * _Queues);
void ProcessReplayMessages(void);
void ProcessRestartAnnounce(uint8_t numTones);


#endif /* PROCESSCOMMANDS_H_ */
