/*
 * MorseChar.h
 *
 *  Created on: Sep 6, 2023
 *      Author: luke
 */

#ifndef MORSECHAR_H_
#define MORSECHAR_H_

#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include "AudioToneGen.h"
#include "MorseSymbol.h"

#define SPRINTF(FMT, args...) \
	sprintf(sSprintf, FMT, args); \
	Serial.print(sSprintf);
extern char sSprintf[];

class MorseChar : public MorseSymbolDefn {
public:
	MorseChar(AudioToneGen* atGen);
	virtual ~MorseChar();


	bool EncodeMorse(char C, RevMorseSeq_t* morseSeq);
	char* DisplayMorse(RevMorseSeq_t Seq);
	bool soundMorseChr(FwdMorseSeq_t* morseSeq, uint8_t WPM);
	bool setMorseMsg(char* Msg);
	bool soundMorseMsg(void);

	inline void setWPM(uint8_t wpm) {
		WPM = wpm;
	}

	inline uint8_t getWPM(void) {
		return WPM;
	}

	inline void setSendMorse(void) {
		if (sizeMorseMsg != 0) {
			enableMorseMsg = true;
		}
	}

	inline void clrSendMorse(void) {
		enableMorseMsg = false;
	}

	inline void stopPlayMorseMsg(void) {
		abortPlayMorseMsg = true;
	}

private:

	bool abortPlayMorseMsg;
	uint8_t WPM;

	char morseMsg[256];
	uint8_t sizeMorseMsg;

	bool morseMsgValid;
	bool enableMorseMsg;

	AudioToneGen* ATGen;

	typedef enum { startChr_st,
		             start_elem_st,
		             soundElem_st,
		             soundWordSpc_st,
		             setupIntra_st,
		             soundIntra_st,
		             nextElem_st,
		             setupChrSpc_st,
		             soundChrSpc_st,
		             exitChr_st } soundChar_st;

	soundChar_st soundChrState;

	typedef enum { startMorseMsg_st,
		             startSoundMorseMsg_st,
		             soundMorseMsg_st,
		             nextMorseMsgChr_st } soundMsg_e;

	soundMsg_e soundMsgState;


	void parseProsign(void);


	inline bool validateMorseMsg(char* Msg) {
		for (uint16_t K = 0; K < sizeof(morseMsg); K++) {
			if (Msg[K] == '\x00') {
				return true;
			}
			if (!isascii(Msg[K])) {
				return false;
			}
		}
		return false;
	}
};

#endif /* MORSECHAR_H_ */
