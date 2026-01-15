/*
 * msggenerator.h
 *
 *  Created on: Apr 19, 2023
 *      Author: luke
 */

#ifndef MSGGENERATOR_H_
#define MSGGENERATOR_H_
#include <Serial.h>
#include <stdint.h>
#include "Qcontainer.h"
#include "AudioToneGen.h"
#include "SerialWaveType.h"
#include "CmmdCode.h"
#include "MorseSymbol.h"

#define SPRINTF(FMT, args...) \
  sprintf(sSprintf, FMT, args); \
  Serial.print(sSprintf);

class MsgUtil {
protected:
  uint16_t CheckSum(uint8_t *Msg, uint8_t Length);
  uint8_t initMsg(uint8_t *Msg, uint8_t Length, CmmdCode::t_cmmdCodeEnum CC);

  inline uint8_t u16tolsb(uint16_t word) {
    return (uint8_t)(word & 0x00FF);
  }

  inline uint8_t u16tomsb(uint16_t word) {
    return (uint8_t)((word & 0xFF00) >> 8);
  }

  inline uint16_t btou16(uint8_t msb, uint8_t lsb) {
    return (uint16_t)(lsb | (msb << 8));
  }

  inline uint16_t FloatToFix(float F) {
    return (uint16_t)((F + 0.05f) * 10.0f);
  }

  inline float FixToFloat(uint16_t F) {
    return ((float)F) / 10.0f;
  }
  /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
  inline uint16_t MsgTo16(uint8_t *msg, uint8_t _Index) {
    return (msg[_Index] << 8) | msg[_Index + 1];
  }

  inline uint32_t MsgTo32(uint8_t *msg, uint8_t _Index) {
    return (MsgTo16(msg, _Index) << 16) | MsgTo16(msg, _Index + 2);
  }
  inline float Decode16Float(uint8_t *msg, uint8_t _Index) {
    return ((float)MsgTo16(msg, _Index)) / 1000.0f;
  }

  inline float Decode32Float(uint8_t *msg, uint8_t _Index) {
    return ((float)MsgTo32(msg, _Index)) / 1000.0f;
  }
  /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
  inline uint8_t Encode16(uint8_t *msg, uint8_t _Index, uint16_t value) {
    msg[_Index] = (value & 0xFF00) >> 8;
    msg[_Index + 1] = value & 0x00FF;
    return _Index + 2;
  }

  inline void Encode32(uint8_t *msg, uint8_t _Index, uint32_t value) {
    Encode16(msg, _Index, (uint16_t)((uint32_t)(value & 0xFFFF0000) >> 16));
    Encode16(msg, _Index + 2, (uint16_t)((uint32_t)(value & 0x0000FFFF)));
  }

  inline uint8_t Encode16Float(uint8_t *msg, uint8_t _Index, float value) {
    Encode16(msg, _Index, (uint16_t)(value * 1000.0f));
    return _Index + 2;
  }

  inline uint8_t Encode32Float(uint8_t *msg, uint8_t _Index, float value) {
    Encode32(msg, _Index, (uint32_t)(value * 1000.0f));
    return _Index + 4;
  }

public:
  inline void dsplMsg(uint8_t *msg) {
    uint8_t i;
    uint8_t lenMsg;
    extern char sSprintf[];
    Serial.print("[ ");
    // C Q {Length}
    lenMsg = msg[2];
    for (i = 0; i < 3; i++) {
      sprintf(sSprintf, "0x%02X ", msg[i]);
      Serial.print(sSprintf);
    }
    Serial.print(" ");
    // {Command Code}
    sprintf(sSprintf, "0x%02X  <", msg[3]);
    Serial.print(sSprintf);

    // Message body
    for (i = 4; i < lenMsg - 2; i++) {
      sprintf(sSprintf, "0x%02X ", msg[i]);
      Serial.print(sSprintf);
    }
    Serial.print(">  ");
    //Checksum
    sprintf(sSprintf, "(0x%02X 0x%02X) ] {", msg[lenMsg - 2], msg[lenMsg - 1]);
    Serial.print(sSprintf);

    // Text view
    for (i = 0; i < lenMsg; i++) {
      char C = msg[i];
      if (isascii(C) && isprint(C)) {
        Serial.print(C);
      } else {
        Serial.print(".");
      }
    }
    Serial.println("}");
  }
};

class enableTone : public MsgUtil, CmmdCode {
public:
  enableTone(uint8_t _Handle, bool _All);
  enableTone(uint8_t *Msg);
  virtual ~enableTone();
  bool getAll(void);
  uint8_t getHandle(void);

private:
  uint8_t Length;
  uint8_t Handle;
  bool All;
  uint8_t MsgIdx;
  uint8_t msg[8];
};

class disableTone : public MsgUtil, CmmdCode {

public:
  disableTone(uint8_t _Handle, bool _All);
  disableTone(uint8_t *Msg);
  virtual ~disableTone();
  uint8_t getHandle(void);
  bool getAll(void);
private:
  uint8_t Length;
  uint8_t Handle;
  bool All;
  uint8_t MsgIdx;
  uint8_t msg[8];
};

class deleteTone : public MsgUtil, CmmdCode {
public:
  deleteTone(uint8_t _Handle);
  deleteTone(uint8_t *Msg);
  virtual ~deleteTone();
  uint8_t getHandle(void);
private:
  uint8_t Length;
  uint8_t Handle;
  uint8_t MsgIdx;
  uint8_t msg[7];
};


class addTone : public MsgUtil, CmmdCode, SerialWaveType {
public:
  addTone(uint8_t _Index, float _Ampl, float _Freq, float _Phase, SerialWaveType::t_codeEnum _Waveform);
  addTone(uint8_t *_Msg);
  float getAmpl(void);
  float getFreq(void);
  float getPhase(void);
  uint8_t getIndex(void);
  uint8_t getHandle(void);
  SerialWaveType::t_codeEnum getWaveFormType(void);

  virtual ~addTone();
private:
  uint8_t msg[19];
  float Ampl;
  float Freq;
  float Phase;
  SerialWaveType::t_codeEnum Waveform;
  uint8_t Length;
  uint8_t Index;
  uint8_t Handle;
  uint8_t MsgIdx;
};


class ReqConfig : public MsgUtil, CmmdCode {
public:
  ReqConfig(uint8_t _NumTones);
  ReqConfig(uint8_t *_Msg);
  virtual ~ReqConfig();
private:
  static const uint8_t Length = 6;
  uint8_t msg[Length];
};

class SendConfig : public MsgUtil, CmmdCode, SerialWaveType {
public:
  SendConfig(uint8_t _NumTones);
  SendConfig(uint8_t *_Msg);
  virtual ~SendConfig();

  //  float getCommonModeAmplitude(uint8_t idx);
  uint8_t getHandle(uint8_t idx);
  float getAmplitude(uint8_t idx);
  float getFreq(uint8_t idx);
  float getPhase(uint8_t idx);
  uint16_t getNsamples(uint8_t idx);
  uint8_t getStatus(uint8_t idx);
  bool getDefined(uint8_t idx);
  bool getEnabled(uint8_t idx);
  uint8_t *getMsg(void);

  void setCommonModeAmplitude(float CmmnModeAmpl);
  void setNyquistFrequency(float CmmnModeAmpl);
  void setHandle(uint8_t handle);
  void setStatus(uint8_t Status);
  void setAmplitude(float A);
  void setFreq(float F);
  void setPhase(float Ph);
  void setNsamples(uint16_t Nsamples);
  void setDefined(bool Defined);
  void setEnabled(bool Enabled);
  uint8_t AddListEntry(uint8_t Idx,        // index in SendConfig
                       uint8_t ToneIndex,  // index in toneTable
                       uint8_t Handle,
                       SerialWaveType::t_codeEnum _Waveform,
                       float A,
                       float F,
                       float Ph,
                       uint16_t nSamples,
                       uint8_t Status,
                       bool Defined,
                       bool Enabled);
  uint8_t AddCommonModeA(float A);
  uint8_t AddNyquistFrequency(float NyquistF);

  void FinalizeMsg(void);

private:

  uint8_t *msg;
  uint8_t Length;
  uint8_t Index;
  uint8_t MsgIdx;
  uint8_t NumTones;
  uint8_t ToneListStart;
  uint8_t SingleToneLength;

  float CommonModeAmplitude;
  float NyquistFrequency;

  typedef struct Tone_s {
    uint8_t Index;
    SerialWaveType::t_codeEnum Waveform;
    uint8_t handle;
    float Amplitude;
    float freq;
    float phase;
    uint16_t nSamples;
    uint8_t status;
    bool defined;
    bool enabled;
  } Tone_t;

  Tone_t *ToneList;
};

class ReturnTableSize : public MsgUtil, CmmdCode, SerialWaveType {
  uint8_t tableSize;
  uint8_t Length;
  uint8_t MsgIdx;
  uint8_t msg[8];

public:
  ReturnTableSize(uint8_t size, bool status);
  ReturnTableSize(uint8_t *_Msg);
  //virtual ~ReturnTableSize();
  uint8_t *getMsg(void);
};

class ReturnHandle : public MsgUtil, CmmdCode, SerialWaveType {
  uint8_t handle;
  uint8_t Length;
  uint8_t MsgIdx;
  uint8_t msg[8];

public:
  ReturnHandle(uint8_t handle, bool status);
  ReturnHandle(uint8_t *_Msg);
  //virtual ~ReturnTableSize();
  uint8_t *getMsg(void);
};

class RespondCommand : public MsgUtil, CmmdCode, SerialWaveType {
  uint8_t handle;
  uint8_t status;
  uint8_t Length;
  uint8_t MsgIdx;
  uint8_t msg[8];

public:
  RespondCommand(uint8_t handle, uint8_t status);
  RespondCommand(uint8_t *_Msg);
  uint8_t *getMsg(void);
};

class RestartAnnounced : public MsgUtil, CmmdCode, SerialWaveType {
  uint8_t numTones;
  uint8_t Length;
  uint8_t MsgIdx;
  uint8_t msg[7];

public:
  RestartAnnounced(uint8_t numTones);
  uint8_t *getMsg(void);
};

class uploadRunningWPM : public MsgUtil, CmmdCode, SerialWaveType {
  uint8_t runningWPM;
  uint8_t Length;
  uint8_t MsgIdx;
  uint8_t msg[7];

public:
  uploadRunningWPM(uint8_t runningWPM);
  uint8_t *getMsg(void);
    virtual ~uploadRunningWPM();

};
/*
class msggenerator {
public:
  msggenerator();
  virtual ~msggenerator();

private:
};
*/
/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
class SendMorseMsg : public MsgUtil, CmmdCode {
public:
  SendMorseMsg(uint8_t *_Msg);
  virtual ~SendMorseMsg();
  uint8_t getTextLength(void);
  char *getTextMsg(void);
private:
  uint8_t *msg;
  uint8_t *textMsg;
  uint8_t Length;
  uint8_t textLength;
};

class PlayMorseMsg : public MsgUtil, CmmdCode {
public:
  PlayMorseMsg(uint8_t *_Msg);
  virtual ~PlayMorseMsg();
  uint8_t getWPM(void);
private:
  uint8_t msg[7];
  uint8_t Length;
  uint8_t WPM;
};

class SendSidetone : public MsgUtil, CmmdCode {
public:
  SendSidetone(uint8_t *_Msg);
  virtual ~SendSidetone();
  float getSidetone(void);
  bool  getSparkGap(void);
private:
  uint8_t Length;
  uint8_t msg[11];
  bool sparkGap;
  float Sidetone;
};

class SendFarnsworth : public MsgUtil, CmmdCode {
public:
  SendFarnsworth(uint8_t *_Msg);
  virtual ~SendFarnsworth();
  bool getFarnsworth(void);
private:
  uint8_t msg[7];
  uint8_t Length;
  bool enableFarnsworth;
};

class ReceiveTextChar : public MsgUtil, CmmdCode, MorseSymbolDefn {
public:
  ReceiveTextChar(morseCharToken_t *morseCharSeq);
  virtual ~ReceiveTextChar();
  uint8_t *getMsg(void);

private:
  uint8_t *msg;
  uint8_t Length;
};

class ping : public MsgUtil, CmmdCode, MorseSymbolDefn {
public:
  ping();
  virtual ~ping();
  void rcvPing(uint8_t *msg);
  uint8_t * echoPing(void);
  void newPing(uint16_t _payload);

private:
  uint8_t *msg;
  uint8_t Length;
  uint16_t payload;
};

#endif /* MSGGENERATOR_H_ */
