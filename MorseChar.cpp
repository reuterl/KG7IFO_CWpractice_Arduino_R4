/*
 * MorseChar.cpp
 *
 *  Created on: Sep 6, 2023
 *      Author: luke
 */

#include "MorseChar.h"
#include <ctype.h>
#include <string.h>

MorseChar::MorseChar(AudioToneGen *atGen) {
	ATGen = atGen;
	soundChrState = startChr_st;
	soundMsgState = startMorseMsg_st;
	for (uint16_t K = 0; K < sizeof(morseMsg); K++) {
		morseMsg[K] = '\xEE';
	}
	morseMsgValid = false;
	enableMorseMsg = false;
	WPM = 5;
}

MorseChar::~MorseChar() {
	// TODO Auto-generated destructor stub
}

char *MorseChar::DisplayMorse(RevMorseSeq_t Seq) {
	int K = Seq.count;
	int L = 0;
	while (K) {
		MorseString[--K] = Seq.elements[L++] ? '-' : '.';
	}
	MorseString[Seq.count] = '\0';
	return (char *)&MorseString;
}

bool MorseChar::EncodeMorse(char C, RevMorseSeq_t *morseSeq) {
	char asciiChar = toupper(C);
	if (asciiChar == 0x20) {
		morseSeq->isSpace = true;
		return true;
	} else {
		morseSeq->isSpace = false;
		for (int K = 0; K < symbolTreeSize; K++) {
			const symbolTree_t *Entry = &symbolTree[K];
			if (Entry->asciiChar == asciiChar) {
				morseSeq->count = 0;
				while (Entry->previous != 99) {
					morseSeq->elements[morseSeq->count++] = Entry->endElement;
					Entry = &symbolTree[Entry->previous];
				}
				return true;
			}
		}
	}

	return false;
}

bool MorseChar::soundMorseChr(FwdMorseSeq_t *morseSeq, uint8_t WPM) {
	static uint8_t ptr = 0;
	static uint16_t Tdit = 0;
	static uint16_t elemTime = 0;
	static uint16_t duration = 0;
	static bool returnStatus = false;
	static bool useFarnsworth = false;

	if (abortPlayMorseMsg == true) {
		soundChrState = exitChr_st;
	}

	switch (soundChrState) {
		case startChr_st:
			ptr = morseSeq->count - 1;
			Tdit = 1200 / WPM;
			returnStatus = false;
			soundChrState = start_elem_st;
			break;

		case start_elem_st:
			useFarnsworth = getFarnsworthSpacing();
			if (morseSeq->isSpace == true) {
				if (useFarnsworth == true) {
					elemTime = WPM2ms(5) * 4;
				} else {
					elemTime = Tdit * 4;  // word space = 7 dits, 3 already provided between charsgetElapsed
				}
				startInterval(3);
				soundChrState = soundWordSpc_st;
			} else {
				elemTime = morseSeq->elements[ptr] ? (Tdit * 3) : (Tdit);
				if (ATGen->getSparkGap()) {
					ATGen->setEnabled(3, true);
				} else {
					ATGen->setEnabled(2, true);
				}
				startInterval(3);
				soundChrState = soundElem_st;
			}
			break;

		case soundWordSpc_st:
			duration = getElapsed(3);
			if (duration < elemTime) {
				soundChrState = soundWordSpc_st;
			} else {
				soundChrState = exitChr_st;
			}
			break;

		case soundElem_st:
			duration = getElapsed(3);
			if (duration < elemTime) {
				soundChrState = soundElem_st;
			} else {
				if (ATGen->getSparkGap()) {
					ATGen->setEnabled(3, false);
				} else {
					ATGen->setEnabled(2, false);
				}
				soundChrState = setupIntra_st;
			}
			break;

		case setupIntra_st:
			elemTime = Tdit;
			startInterval(3);
			soundChrState = soundIntra_st;
			break;

		case soundIntra_st:
			duration = getElapsed(3);
			if (duration < elemTime) {
				soundChrState = soundIntra_st;
			} else {
				soundChrState = nextElem_st;
			}
			break;

		case nextElem_st:
			if (ptr == 0) {
				soundChrState = setupChrSpc_st;
			} else {
				ptr--;
				soundChrState = start_elem_st;
			}
			break;

		case setupChrSpc_st:
			if (useFarnsworth == true) {
				elemTime = WPM2ms(5) * 3;
			} else {
				elemTime = Tdit * 3;
			}
			startInterval(3);
			soundChrState = soundChrSpc_st;
			break;

		case soundChrSpc_st:
			duration = getElapsed(3);
			if (duration < elemTime) {
				soundChrState = soundChrSpc_st;
			} else {
				soundChrState = exitChr_st;
			}
			break;

		case exitChr_st:
			returnStatus = true;
			soundChrState = startChr_st;
			break;

		default:
			break;
	}

	return returnStatus;
}

bool MorseChar::setMorseMsg(char *Msg) {
	morseMsgValid = false;
	enableMorseMsg = false;
	morseMsg[0] = '\0';  // init to  NULL string.

	if (validateMorseMsg(Msg) == true) {
		strcpy(morseMsg, Msg);
		parseProsign();
		sizeMorseMsg = strlen(morseMsg);
		morseMsgValid = true;
		SPRINTF("setMorseMsg = \"%s\"\n", morseMsg);
		return true;
	} else {
		return false;
	}
}


void MorseChar::parseProsign(void) {
	uint8_t ptr = 0;
	uint8_t tgt = 0;
	char C;
	char *proPtr = NULL;
	proSignTbl_t *entryProSignTbl = NULL;

	do {
		C = morseMsg[ptr];
		if (C == '\\') {
			proPtr = &morseMsg[ptr + 1];
			entryProSignTbl = findProSign(proPtr);
			if (entryProSignTbl != NULL) {
				morseMsg[ptr + 1] = entryProSignTbl->byteCode;
				tgt = ptr + 2;  // position after \[prosign code]
				ptr += strlen(entryProSignTbl->escapeSeq) + 1;
			} else {
				morseMsg[ptr++] = '?';  // replace with question, leave bogus code.
				tgt = ptr;
			}
		} else {
			ptr++;
			morseMsg[tgt++] = C;
		}
	} while (morseMsg[ptr] != 0x00);
	morseMsg[tgt] = 0x00;
}

bool MorseChar::soundMorseMsg(void) {
	static uint8_t ptr;
	static RevMorseSeq_t morseSeq;
	static bool returnCode = false;
	char C;

	switch (soundMsgState) {
		case startMorseMsg_st:
			if ((enableMorseMsg == true) && (morseMsgValid == true)) {
				ptr = 0;
				returnCode = false;
				soundMsgState = startSoundMorseMsg_st;
				abortPlayMorseMsg = false;
			} else {
				soundMsgState = startMorseMsg_st;
				returnCode = true;
			}
			break;

		case startSoundMorseMsg_st:
			soundMsgState = soundMorseMsg_st;
			C = morseMsg[ptr];
			if (C == '\\') {
				uint8_t idx = morseMsg[ptr + 1] - 1;
				proSignTbl_t *entryProSignTbl = &proSignTbl[idx];
				morseSeq = entryProSignTbl->morseSeq;
				ptr += 1;
			} else {
				EncodeMorse(C, &morseSeq);
			}
			break;

		case soundMorseMsg_st:
			if (soundMorseChr(&morseSeq, WPM)) {
				soundMsgState = nextMorseMsgChr_st;
			} else {
				soundMsgState = soundMorseMsg_st;
			}
			break;

		case nextMorseMsgChr_st:
			ptr++;
			if (morseMsg[ptr] == '\x00') {
				returnCode = true;
				enableMorseMsg = false;
				soundMsgState = startMorseMsg_st;
			} else {
				soundMsgState = startSoundMorseMsg_st;
			}
			break;
		default:
			break;
	}
	return returnCode;
}
