/*
 * msggenerator.cpp
 *
 *  Created on: Apr 19, 2023
 *      Author: luke
 */
#include <string.h>
#include <math.h>
#include <Serial.h>
#include "Qcontainer.h"
#include "msggenerator.h"
/*
msggenerator::msggenerator() {
	// TODO Auto-generated constructor stub

}

msggenerator::~msggenerator() {
	// TODO Auto-generated destructor stub
}
*/

bool g_farnsworthSpacing = false;

uint16_t MsgUtil:: CheckSum(uint8_t * Msg, uint8_t Length){
	uint16_t calcChecksum = 0;
	uint8_t idx;

	for (idx=0; idx < Length-2; idx++){
		calcChecksum += Msg[idx];
	}
	Msg[Length-2] = u16tomsb(calcChecksum);
	Msg[Length-1] = u16tolsb(calcChecksum);

	return calcChecksum;
}

uint8_t MsgUtil::initMsg(uint8_t * Msg, uint8_t Length, CmmdCode::t_cmmdCodeEnum CC){
	uint8_t idx;
	const uint8_t next = 4;

	Msg[0] = 'C';
	Msg[1] = 'Q';
	Msg[2] = Length;
	Msg[3] = CC;

	for (idx = next; idx < Length; idx ++){
		Msg[idx] = 0xFE;
	}

	return next;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*###########################################################################*/
addTone::addTone(uint8_t _Handle, float _Ampl, float _Freq, float _Phase, SerialWaveType::t_codeEnum _Waveform){

    Handle = _Handle;
    Ampl = _Ampl;
    Freq = _Freq;
    Phase = _Phase;
    Waveform = _Waveform;
    Length = 20;

    MsgIdx = initMsg(msg, Length, CmmdCode::t_cmmdCodeEnum::cmmdAddTone);


    msg[MsgIdx++] = Handle;
    msg[MsgIdx++] = Waveform;

    Encode32Float(msg, MsgIdx, Ampl);
    Encode32Float(msg, MsgIdx+4, Freq);
    Encode32Float(msg, MsgIdx+8, Phase);


    CheckSum(msg, Length);
}

addTone::addTone(uint8_t * _Msg){
    Length = 20;
	memmove (msg, _Msg, Length);
}

float addTone::getAmpl(void){
	return Decode32Float(msg, 6);
}

float addTone::getFreq(void){
	return Decode32Float(msg, 10);
}

float addTone::getPhase(void){
	return Decode32Float(msg, 14);
}

uint8_t  addTone::getIndex(void){
	return msg[4];
}

uint8_t  addTone::getHandle(void){
	return msg[4];
}

SerialWaveType::t_codeEnum addTone::getWaveFormType(void){
	return (SerialWaveType::t_codeEnum)msg[5];
}

addTone::~addTone() {
	Serial.print("~addTone destructor.\r\n");
}

/*###########################################################################*/
enableTone::enableTone(uint8_t _Handle, bool _All){
    Length = 8;
    Handle = _Handle;
    All = _All;

	MsgIdx = initMsg(msg, Length, CmmdCode::t_cmmdCodeEnum::cmmdEnableTone);

	msg[MsgIdx++] = Handle;
	msg[MsgIdx++] = All;

    CheckSum(msg, Length);
}

bool enableTone::getAll(void){
  return All;
}

uint8_t  enableTone::getHandle(void){
  return Handle;
}

enableTone::enableTone(uint8_t * Msg){
	Length = 8;
	MsgIdx = 0;
  Handle = 0;
  All = false;

	memmove (msg, Msg, Length);

  Handle = msg[4];
  All = (msg[5] != 0);
}


enableTone::~enableTone() {
	Serial.print("~enableTone destructor.\r\n");
}
/*###########################################################################*/
disableTone::disableTone(uint8_t _Handle, bool _All){
    Length = 8;
    Handle = _Handle;
    All = _All;

	MsgIdx = initMsg(msg, Length, CmmdCode::t_cmmdCodeEnum::cmmdEnableTone);

	msg[MsgIdx++] = Handle;
	msg[MsgIdx++] = All;

    CheckSum(msg, Length);
}

disableTone::disableTone(uint8_t * Msg){
	Length = 8;
	MsgIdx = 0;
  Handle = 0;
  All = false;

	memmove (msg, Msg, Length);
  
  Handle = msg[4];
  All = (msg[5] != 0);
}

uint8_t  disableTone::getHandle(void){
	return Handle;
}

bool  disableTone::getAll(void){
	return All;
}

disableTone::~disableTone() {
	Serial.print("~disableTone destructor.\r\n");
}

/*###########################################################################*/
deleteTone::deleteTone(uint8_t _Handle){
    Length = 7;
    Handle = _Handle;

	MsgIdx = initMsg(msg, Length, CmmdCode::t_cmmdCodeEnum::cmmdDelTone);

	msg[MsgIdx++] = Handle;

    CheckSum(msg, Length);
}

deleteTone::deleteTone(uint8_t * Msg){
	Length = 7;
	memmove (msg, Msg, Length);
}

uint8_t  deleteTone::getHandle(void){
	return msg[4];
}

deleteTone::~deleteTone() {
	Serial.print("~deleteTone destructor.\r\n");
}


/*###########################################################################*/
ReqConfig::ReqConfig(uint8_t * Msg){
	memmove (msg, Msg, Length);
}

/*###########################################################################*/
SendConfig::SendConfig(uint8_t _NumTones){

	CommonModeAmplitude = 0.0f;
	Index = 0;

	NumTones = _NumTones;

	ToneList = new Tone_t[NumTones];

	for (uint8_t K=0; K < NumTones; K++){
		Tone_t * T = &ToneList[K];
		T->Index = 0;
		T->Waveform = SerialWaveType::t_codeEnum::Invalid;
		T->Amplitude = 0.0f;
		T->freq = 0.0f;
		T->phase = 0.0f;
    T->nSamples = 0;
    T->status = 0x00;
		T->defined = false;
		T->enabled = false;

		Length = 7;// C+Q+length+CmmdCode+ToneListLength+Checksum-MSB+Checksum-LSB
		Length += 4; // Common Mode Amplitude
		Length += 4; // Nyquist Frequency
		SingleToneLength  = 1; // ToneListIndex
		SingleToneLength += 1; // ToneListHandle
		SingleToneLength += 1; // WaveformType
		SingleToneLength += 4; // Amplitude
		SingleToneLength += 4; // Frequency
		SingleToneLength += 4; // Phase
		SingleToneLength += 2; // nSamples
		SingleToneLength += 1; // Status
		SingleToneLength += 1; // Defined
		SingleToneLength += 1; // Enabled

		Length += SingleToneLength*NumTones;

		msg = new uint8_t[Length];

		MsgIdx = initMsg(msg, Length, CmmdCode::t_cmmdCodeEnum::cmmdSendConfig);
		msg[MsgIdx++] = NumTones;
		msg[MsgIdx++] = 0; // CommonModeAmplitude MMSB
		msg[MsgIdx++] = 0; // CommonModeAmplitude MLSB
		msg[MsgIdx++] = 0; // CommonModeAmplitude LMSB
		msg[MsgIdx++] = 0; // CommonModeAmplitude LLSB
		msg[MsgIdx++] = 0; // Nyquist Frequency MMSB
		msg[MsgIdx++] = 0; // Nyquist Frequency MLSB
		msg[MsgIdx++] = 0; // Nyquist Frequency LMSB
		msg[MsgIdx++] = 0; // Nyquist Frequency LLSB
		ToneListStart = MsgIdx;
	}

}
void  SendConfig::FinalizeMsg(void){
    CheckSum(msg, Length);
}

uint8_t SendConfig::AddCommonModeA(float A){
	uint8_t MsgBaseIdx = 5;
	return Encode32Float(msg, MsgBaseIdx, A);
}

uint8_t SendConfig::AddNyquistFrequency(float NyquistF){
	uint8_t MsgBaseIdx = 9;
	return Encode32Float(msg, MsgBaseIdx, NyquistF);
}

uint8_t SendConfig::AddListEntry(uint8_t Idx, uint8_t ToneIndex, uint8_t Handle, SerialWaveType::t_codeEnum _Waveform,
		                         float A, float F, float Ph, uint16_t nSamples, uint8_t Status, bool Defined, bool Enabled){

	uint8_t MsgBaseIdx = ToneListStart + (SingleToneLength * Idx);

	msg[MsgBaseIdx++] = ToneIndex;
	msg[MsgBaseIdx++] = Handle;
	msg[MsgBaseIdx++] = SerialWaveType::getWaveformCode(_Waveform);

	MsgBaseIdx = Encode32Float(msg, MsgBaseIdx, A);
	MsgBaseIdx = Encode32Float(msg, MsgBaseIdx, F);
	MsgBaseIdx = Encode32Float(msg, MsgBaseIdx, Ph);
	MsgBaseIdx = Encode16(msg, MsgBaseIdx, nSamples);


    msg[MsgBaseIdx++] = Status;
    msg[MsgBaseIdx++] = Defined;
    msg[MsgBaseIdx++] = Enabled;

    return MsgBaseIdx;
}

uint8_t * SendConfig::getMsg(void){
	return msg;
}




SendConfig::SendConfig(uint8_t * _Msg){
	uint8_t length = _Msg[2];
	msg = new uint8_t[length];
	memmove (msg, _Msg, length);
}

SendConfig::~SendConfig(){
	delete ToneList;
	delete msg;
	Serial.print("~SendConfig destructor.\r\n");
}


/*###########################################################################*/
ReturnTableSize::ReturnTableSize(uint8_t size, bool status){
    Length = 8;

 	  MsgIdx = initMsg(msg, Length, CmmdCode::t_cmmdCodeEnum::cmmdReturnTableSize);

	  msg[MsgIdx++] = size;
	  msg[MsgIdx++] = status ? 1 : 0;

    CheckSum(msg, Length);

  }
  
  ReturnTableSize::ReturnTableSize(uint8_t *_Msg){
    
  }

uint8_t * ReturnTableSize::getMsg(void){
	return msg;
}
/*###########################################################################*/
ReturnHandle::ReturnHandle(uint8_t handle, bool status){
    Length = 8;

 	  MsgIdx = initMsg(msg, Length, CmmdCode::t_cmmdCodeEnum::cmmdReturnHandle);

	  msg[MsgIdx++] = handle;
	  msg[MsgIdx++] = status ? 1 : 0;

    CheckSum(msg, Length);

  }
  
ReturnHandle::ReturnHandle(uint8_t *_Msg){
    
  }

uint8_t * ReturnHandle::getMsg(void){
	return msg;
}
/*###########################################################################*/
RespondCommand::RespondCommand(uint8_t handle, uint8_t status){
    Length = 8;

 	  MsgIdx = initMsg(msg, Length, CmmdCode::t_cmmdCodeEnum::cmmdRespondCommand);

	  msg[MsgIdx++] = handle;
	  msg[MsgIdx++] = status;

    CheckSum(msg, Length);

  }
  
RespondCommand::RespondCommand(uint8_t *_Msg){
    
  }

uint8_t * RespondCommand::getMsg(void){
	return msg;
}
/*###########################################################################*/
RestartAnnounced::RestartAnnounced(uint8_t numTones){
    Length = 7;
 	  MsgIdx = initMsg(msg, Length, CmmdCode::t_cmmdCodeEnum::cmmdRestartAnnounced);

	  msg[MsgIdx++] = numTones;

    CheckSum(msg, Length);

  }
  
RestartAnnounced::RestartAnnounced(uint8_t *_Msg){
    
  }

uint8_t * RestartAnnounced::getMsg(void){
	return msg;
}
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
ReceiveTextChar::ReceiveTextChar(morseCharToken_t * morseCharToken) {
	uint8_t msgIdx;

	Length = 6; //C+Q+len+CC+chksum-h+chksum-l
	Length += 4; //char+valid+prosign+lengthSeq;
	Length += 3*morseCharToken->lengthSeq; // [element+duration(16)] * seqLength;
    Length += 2; // Tdit(16)
    Length += 1; // Farnsworth Spacing

	msg = new uint8_t[Length];
    msgIdx = initMsg(msg, Length, CmmdCode::cmmdReceiveTextChar);
    msg[4] = morseCharToken->morseChar; //actual char, or prosign index
    msg[5] = morseCharToken->valid ? 1:0;
    msg[6] = morseCharToken->prosign ? 1:0;
    msg[7] = morseCharToken->lengthSeq;
    msg[8] = morseCharToken->farnsworth ? 1:0;
    Encode16(msg, 9, (uint16_t)morseCharToken->Tdit);

    msgIdx = 11;
    for (int idx = 0; idx < morseCharToken->lengthSeq; idx++){
        msg[msgIdx] = morseCharToken->morseCharSeq[idx].morseElement;
        Encode16(msg, msgIdx+1, (uint16_t)morseCharToken->morseCharSeq[idx].Duration);
        msgIdx+= 3;
    }
    CheckSum(msg, Length);
}
uint8_t * ReceiveTextChar::getMsg(void){
	return msg;
}

ReceiveTextChar::~ReceiveTextChar() {
	delete msg;
	//PRINTF("~SendMorseMsg destructor.\r\n");
}
/* ===========================================================================*/
SendMorseMsg::SendMorseMsg(uint8_t * _Msg){
	Length = _Msg[2];
	msg = new uint8_t[Length];
	memmove (msg, _Msg, Length);
	textLength = msg[4];
	textMsg = new uint8_t[textLength+1];
	memmove (textMsg, &msg[5], textLength);
	textMsg[textLength] = (char)0x00;
}

uint8_t SendMorseMsg::getTextLength(void){
	return textLength;
}
char * SendMorseMsg::getTextMsg(void){
	return (char *) textMsg;
}
SendMorseMsg::~SendMorseMsg() {
    delete msg;
    delete textMsg;
	//PRINTF("~SendMorseMsg destructor.\r\n");
}
/* ===========================================================================*/

PlayMorseMsg::PlayMorseMsg(uint8_t * _Msg){
	Length = 7;
	memmove (msg, _Msg, Length);
	WPM = msg[4];
}

PlayMorseMsg::~PlayMorseMsg() {
	//PRINTF("~PlayMorseMsg destructor.\r\n");
}

uint8_t PlayMorseMsg::getWPM(void) {
	return WPM;
}

/* ===========================================================================*/
SendSidetone::SendSidetone(uint8_t * _Msg){
	Length = 10;
	memmove (msg, _Msg, Length);
	Sidetone = Decode32Float(msg,4);
}

SendSidetone::~SendSidetone() {
	//PRINTF("~SendSidetone destructor.\r\n");
}

float SendSidetone::getSidetone(void) {
	return Sidetone;
}

/* ===========================================================================*/
SendFarnsworth::SendFarnsworth(uint8_t * _Msg){
	Length = 7;
	memmove (msg, _Msg, Length);
	enableFarnsworth = msg[4] == 0x01;
}

SendFarnsworth::~SendFarnsworth() {
	//PRINTF("~SendFarnsworth destructor.\r\n");
}

bool SendFarnsworth::getFarnsworth(void) {
	return enableFarnsworth;
}
