/*
 * MorseSymbol.cpp
 *
 *  Created on: Sep 25, 2023
 *      Author: luke
 */

#include "MorseSymbol.h"
#include <string.h>

MorseSymbolDefn::MorseSymbolDefn() {
//	farnsworthSpacing = false;
}

MorseSymbolDefn::~MorseSymbolDefn() {
	// TODO Auto-generated destructor stub
}

bool MorseSymbolDefn::compareProSeq(char * escapeSeq, char * morseMsg){
	uint16_t lengthEscapeSeq = strlen(escapeSeq);

	for (uint8_t L = 0; L < lengthEscapeSeq; L++){
		if (escapeSeq[L] != toupper(morseMsg[L])){
			return false;
		}
	}
	return true;
}

bool MorseSymbolDefn::prosignSeqCompare(morseSeq_t *  morseSeq, proSignTbl_t * proSignTblEntry){
	uint8_t revIdx = proSignTblEntry->morseSeq.count-1;
	for (int V = 0; V < morseSeq->count; V++){
		if (morseSeq->elements[V] != proSignTblEntry->morseSeq.elements[revIdx--]){
			return false;
		}
	}
	return true;
}

bool MorseSymbolDefn::lookupProsign(morseSeq_t *  morseSeq, uint8_t * idx){
	for(int W = 0; W < sizeProSignTbl; W++){
		if (morseSeq->count == proSignTbl[W].morseSeq.count){
			if (prosignSeqCompare(morseSeq, &proSignTbl[W]) == true){
				*idx = W;
				return true;
			}
		}
	}
	return false;
}

MorseSymbolDefn::proSignTbl_t * MorseSymbolDefn::findProSign(char *proSeq) {
	uint16_t tblSize = sizeof(proSignTbl) / sizeof(proSignTbl_t);
	for (uint8_t K = 0; K < tblSize; K++) {
		if (compareProSeq(proSignTbl[K].escapeSeq, proSeq) == true) {
			return &proSignTbl[K];
		}
	}
	return NULL;
}

bool MorseSymbolDefn::DecodeMorse(FwdMorseSeq_t morseSeq, char * C){
	const symbolTree_t * ST = &symbolTree[0];
	uint8_t next;
	for (int idx = 0; idx < morseSeq.count; idx++){
		next = ST->ditdah[morseSeq.elements[idx]];
		if (next == 0){
			return false;
		}
		else{
			ST = &symbolTree[next];
		}
	}
	*C = ST->asciiChar;
	return true;
}

void MorseSymbolDefn::setFarnsworthSpacing(bool farnsworth){
	farnsworthSpacing = farnsworth;
}

bool MorseSymbolDefn::getFarnsworthSpacing(){
	return farnsworthSpacing;
}

