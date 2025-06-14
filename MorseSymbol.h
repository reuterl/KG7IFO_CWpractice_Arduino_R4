/*
 * MorseSymbol.h
 *
 *  Created on: Sep 25, 2023
 *      Author: luke
 */

#ifndef MORSESYMBOL_H_
#define MORSESYMBOL_H_

#include <arduino.h>
#include <stdint.h>
#include <ctype.h>

class MorseSymbolDefn {

	/************************ Timers ***************/
private:
	uint64_t startTime[5];
public:
	inline void startInterval(uint8_t timerHandle) {
		startTime[timerHandle] = millis();
	}

	inline uint32_t getElapsed(uint8_t timerHandle) {
		return millis() - startTime[timerHandle];
	}

public:
	bool farnsworthSpacing;
	MorseSymbolDefn();
	virtual ~MorseSymbolDefn();

	void setFarnsworthSpacing(bool farnsworth);
	bool getFarnsworthSpacing();


	typedef enum morseElement_e { morseDit = 101,
		                            morseDah,
		                            morseMark,
		                            morseSpace,
		                            morseWordSpace,
		                            morseIdle,
		                            morseUnknown } morseElement_t;


	const uint8_t Cdit = 0;
	const uint8_t Cdah = 1;

	typedef struct morseSeq_st {
		uint8_t count;
		bool isSpace;
		uint8_t elements[9];
	} morseSeq_t;

	typedef morseSeq_t FwdMorseSeq_t;
	typedef morseSeq_t RevMorseSeq_t;

	typedef struct morseCharSeq_st {
		morseElement_e morseElement;
		uint32_t Duration;
	} morseCharSeq_t;

	typedef struct morseCharToken_st {
		uint16_t Tdit;
		char morseChar;
		bool valid;
		bool prosign;
		bool farnsworth;
		uint8_t lengthSeq;
		morseCharSeq_t morseCharSeq[18];  // longest = \SOS
	} morseCharToken_t;

	typedef struct symbolTree_st {
		char asciiChar;
		uint8_t ditdah[2];
		uint8_t previous;
		uint8_t endElement;
	} symbolTree_t;

	char MorseString[10];

	const symbolTree_t symbolTree[66] = {
		{ '-', { 1, 2 }, 99, 99 },         //  0
		{ 'E', { 3, 4 }, 0, Cdit },        //  1
		{ 'T', { 38, 39 }, 0, Cdah },      //  2
		{ 'I', { 5, 6 }, 1, Cdit },        //  3
		{ 'A', { 21, 22 }, 1, Cdah },      //  4
		{ 'S', { 7, 8 }, 3, Cdit },        //  5
		{ 'U', { 14, 15 }, 3, Cdah },      //  6
		{ 'H', { 9, 10 }, 5, Cdit },       //  7
		{ 'V', { 11, 12 }, 5, Cdah },      //  8
		{ '5', { 0, 0 }, 7, Cdit },        //  9
		{ '4', { 0, 0 }, 7, Cdah },        // 10
		{ '\x8A', { 0, 0 }, 8, Cdah },     // 11
		{ '3', { 0, 0 }, 8, Cdah },        // 12
		{ '$', { 0, 0 }, 11, Cdit },       // 13
		{ 'F', { 0, 0 }, 11, Cdit },       // 14
		{ '\xDC', { 17, 18 }, 6, Cdah },   // 15
		{ '\xC9', { 0, 0 }, 14, Cdit },    // 16
		{ '\xD0', { 19, 20 }, 15, Cdit },  // 17
		{ '2', { 0, 0 }, 15, Cdah },       // 18
		{ '?', { 0, 0 }, 17, Cdit },       // 19
		{ '_', { 0, 0 }, 17, Cdah },       // 20
		{ 'R', { 23, 24 }, 4, Cdit },      // 21
		{ 'W', { 30, 31 }, 4, Cdah },      // 22
		{ 'L', { 25, 26 }, 21, Cdit },     // 23
		{ 'a', { 28, 0 }, 21, Cdah },      // 24
		{ '&', { 0, 0 }, 23, Cdit },       // 25
		{ 'e', { 27, 0 }, 23, Cdah },      // 26
		{ '"', { 0, 0 }, 26, Cdit },       // 27
		{ '+', { 0, 29 }, 24, Cdit },      // 28
		{ '.', { 0, 0 }, 28, Cdah },       // 29
		{ 'P', { 32, 33 }, 22, Cdit },     // 30
		{ 'J', { 35, 36 }, 22, Cdah },     // 31
		{ 'p', { 0, 0 }, 30, Cdit },       // 32
		{ 'a', { 0, 0 }, 30, Cdah },       // 33
		{ '@', { 0, 0 }, 33, Cdit },       // 34
		{ '\x86', { 0, 0 }, 31, Cdah },    // 35
		{ '1', { 0, 0 }, 31, Cdah },       // 36
		{ '\'', { 0, 0 }, 36, Cdit },      // 37

		{ 'N', { 40, 41 }, 2, Cdit },  // 38
		{ 'M', { 42, 43 }, 2, Cdah },  // 39

		{ 'D', { 44, 45 }, 38, Cdit },  // 40
		{ 'K', { 46, 47 }, 38, Cdah },  // 41
		{ 'G', { 48, 49 }, 39, Cdit },  // 42
		{ 'O', { 50, 51 }, 39, Cdah },  // 43
		{ 'B', { 52, 53 }, 40, Cdit },  // 44
		{ 'X', { 54, 0 }, 40, Cdah },   // 45
		{ 'C', { 55, 0 }, 41, Cdit },   // 46
		{ 'Y', { 56, 0 }, 41, Cdah },   // 47
		{ 'Z', { 57, 0 }, 42, Cdit },   // 48
		{ 'Q', { 58, 59 }, 42, Cdah },  // 49

		{ '\xD6', { 60, 0 }, 43, Cdit },   // 50
		{ '\xD7', { 61, 62 }, 43, Cdah },  // 51
		{ '6', { 0, 65 }, 44, Cdit },      // 52
		{ '=', { 0, 0 }, 44, Cdah },       // 53
		{ '/', { 0, 0 }, 45, Cdit },       // 54
		{ '\xC7', { 0, 0 }, 46, Cdit },    // 55
		{ '(', { 0, 64 }, 47, Cdit },      // 56
		{ '7', { 0, 0 }, 48, Cdit },       // 57
		{ '\xee', { 0, 0 }, 49, Cdit },    // 58
		{ '\xD1', { 0, 0 }, 49, Cdah },    // 59
		{ '8', { 63, 0 }, 50, Cdit },      // 60
		{ '9', { 0, 0 }, 51, Cdit },       // 61
		{ '0', { 0, 0 }, 51, Cdah },       // 62
		{ ':', { 0, 0 }, 60, Cdit },       // 63
		{ ')', { 0, 0 }, 56, Cdah },       // 64
		{ '-', { 0, 0 }, 52, Cdah },       // 65
	};

	const int symbolTreeSize = sizeof(symbolTree) / sizeof(symbolTree_st);
	typedef enum proSignCode_e {
		proAA = 1,
		proAR,
		proAS,
		proBT,
		proCQ,
		proCT,
		proEE,
		proIMI,
		proKN,
		proSK,
		proSN,
		proSOS
	} proSignCode_t;

	typedef struct proSignTbl_st {
		char escapeSeq[4];
		uint8_t lengthEscapeSeq;
		uint8_t byteCode;
		RevMorseSeq_t morseSeq;
	} proSignTbl_t;
	static const uint8_t sizeProSignTbl = 12;
	proSignTbl_t proSignTbl[sizeProSignTbl] = {
		{ "AA", 2, proAA, { 4, false, Cdah, Cdit, Cdah, Cdit } },
		{ "AR", 2, proAR, { 5, false, Cdit, Cdah, Cdit, Cdah, Cdit } },
		{ "AS", 2, proAS, { 5, false, Cdit, Cdit, Cdit, Cdah, Cdit } },
		{ "BT", 2, proBT, { 5, false, Cdah, Cdit, Cdit, Cdit, Cdah } },
		{ "CQ", 2, proCQ, { 8, false, Cdah, Cdit, Cdah, Cdah, Cdit, Cdah, Cdit, Cdah } },
		{ "CT", 2, proCT, { 5, false, Cdah, Cdit, Cdah, Cdit, Cdah } },
		{ "EE", 2, proEE, { 8, false, Cdit, Cdit, Cdit, Cdit, Cdit, Cdit, Cdit, Cdit } },
		{ "IMI", 3, proIMI, { 6, false, Cdit, Cdit, Cdah, Cdah, Cdit, Cdit } },
		{ "KN", 2, proKN, { 5, false, Cdit, Cdah, Cdah, Cdit, Cdah } },
		{ "SK", 2, proSK, { 6, false, Cdah, Cdit, Cdah, Cdit, Cdit, Cdit } },
		{ "SN", 2, proSN, { 5, false, Cdit, Cdah, Cdit, Cdit, Cdit } },
		{ "SOS", 3, proSOS, { 9, false, Cdit, Cdit, Cdit, Cdah, Cdah, Cdah, Cdit, Cdit, Cdit } }
	};
	bool lookupProsign(morseSeq_t* morseSeq, uint8_t* idx);
	bool prosignSeqCompare(morseSeq_t* morseSeq, proSignTbl_t* proSignTblEntry);

	bool compareProSeq(char* escapeSeq, char* morseMsg);
	proSignTbl_t* findProSign(char* proSeq);
	bool DecodeMorse(FwdMorseSeq_t morseSeq, char* C);

	inline uint16_t WPM2ms(uint16_t WPM) {
		return 1200 / WPM;
	}

	inline uint16_t MS2wpm(uint16_t MS) {
		return 1200 / MS;
	}
};

#endif /* MORSESYMBOL_H_ */
