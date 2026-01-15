#ifndef CMMDCODE_H_
#define CMMDCODE_H_
#include <string>

class CmmdCode {
  public:
  typedef enum e_cmmdCodeEnum {
    cmmdReqTableSize = 0xB0,
    cmmdReqHandle = 0xB1,
    cmmdReturnTableSize = 0xB2,
    cmmdReturnHandle = 0xB3,

    cmmdAddTone = 0xC0,
    cmmdEnableTone = 0xC1,
    cmmdDisableTone = 0xC2,
    cmmdReqConfig = 0xC3,
    cmmdDelTone = 0xC4,

    cmmdSendMorseMsg = 0xD0,
    cmmdPlayMorseMsg = 0xD1,
    cmmdSendSidetone = 0xD2,
    cmmdSendFarnsworth = 0xD3,
    
    cmmdReceiveTextChar = 0xF0,
    cmmdRunningWPM = 0xF1,

    cmmdSendConfig = 0xA0,
    cmmdRespondCommand = 0xA1,
    cmmdRestartAnnounced = 0xA2,
    cmmdPing = 0xAA
  } t_cmmdCodeEnum;

  typedef struct st_CmmdCodeElement {
    uint8_t code;
    t_cmmdCodeEnum cmmdCodeEnum;
    string name;
  } t_cmmdCodeElement;

  const t_cmmdCodeElement cmmdCodeTable[19] = {
    { 0xB0, cmmdReqTableSize, "cmmdReqTableSize" },
    { 0xB1, cmmdReqHandle, "cmmdReqHandle" },
    { 0xB2, cmmdReturnTableSize, "cmmdReturnTableSize" },
    { t_cmmdCodeEnum::cmmdReturnHandle , cmmdReturnHandle, "cmmdReturnHandle" },

    { 0xC0, cmmdAddTone, "cmmdAddTone" },
    { 0xC1, cmmdEnableTone, "cmmdEnableTone" },
    { 0xC2, cmmdDisableTone, "cmmdDisableTone" },
    { 0xC3, cmmdReqConfig, "cmmdReqConfig" },
    { 0xC4, cmmdDelTone, "cmmdDelTone" },

    { 0xD0, cmmdSendMorseMsg, "cmmdSendMorseMsg" },
    { 0xD1, cmmdPlayMorseMsg, "cmmdPlayMorseMsg" },
    { 0xD2, cmmdSendSidetone, "cmmdSendSidetone" },
    { 0xD3, cmmdSendFarnsworth, "cmmdSendFarnsworth" },

    { 0xF0, cmmdReceiveTextChar, "cmmdReceiveTextChar" },
    { 0xF1, cmmdRunningWPM, "cmmdRunningWpm" },

    { 0xA0, cmmdSendConfig, "cmmdSendConfig" },
    { 0xA1, cmmdRespondCommand, "cmmdRespondCommand" },
    { 0xA2, cmmdRestartAnnounced, "cmmdRestartAnnounced"},
    { 0xAA, cmmdPing, "cmmdPing"}
  };

  const uint8_t sizeCmmdCodeTable = (sizeof(cmmdCodeTable) / sizeof(t_cmmdCodeElement));

  inline std::string getCmmdCodeName(uint8_t code) {
    for (int I = 0; I < sizeCmmdCodeTable; I++) {
      if (cmmdCodeTable[I].code == code) {
        return cmmdCodeTable[I].name;
      }
    }
    return "Invalid";
  }

  inline std::string getCmmdCodeName(t_cmmdCodeEnum codeEnum) {
    for (int I = 0; I < sizeCmmdCodeTable; I++) {
      if (cmmdCodeTable[I].cmmdCodeEnum == codeEnum) {
        return cmmdCodeTable[I].name;
      }
    }
    return "Invalid";
  }

  inline uint8_t getWaveformCode(std::string name) {
    for (int I = 0; I < sizeCmmdCodeTable; I++) {
      if (cmmdCodeTable[I].name == name) {
        return cmmdCodeTable[I].code;
      }
    }
    return 0xFF;
  }

  inline uint8_t getWaveformCode(t_cmmdCodeEnum codeEnum) {
    for (int I = 0; I < sizeCmmdCodeTable; I++) {
      if (cmmdCodeTable[I].cmmdCodeEnum == codeEnum) {
        return cmmdCodeTable[I].code;
      }
    }
    return 0xFF;
  }
};

#endif