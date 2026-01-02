#ifndef SERIALWAVETYPE_H_
#define SERIALWAVETYPE_H_
//#include <string.h>
using namespace std;
class SerialWaveType {

public:
typedef enum e_codeEnum {Sine=0xF0, Square=0xF1, Sawtooth=0xF2, Triangle=0xF3, Sound=0xF4, Invalid=0xFF} t_codeEnum;

typedef struct st_typeCode {
  uint8_t code;
  t_codeEnum codeEnum;
  std::string name;
} t_typeCode;


const t_typeCode typeCodeTable[5] = {
  {0xF0, Sine, "Sine"},
  {0xF1, Square, "Square"},
  {0xF2, Sawtooth, "Sawtooth"},
  {0xF3, Triangle, "Triangle"},
  {0xFF, Invalid, "Invalid"},
 };

const uint8_t sizetypeCodeTable = (sizeof(typeCodeTable)/sizeof(t_typeCode)) -1;
public:

inline std::string getWaveformName(uint8_t code){
    for(int I=0; I < sizetypeCodeTable; I++){
      if (typeCodeTable[I].code == code){
        return typeCodeTable[I].name;
      }
    }
    return typeCodeTable[sizetypeCodeTable-1].name;
}

inline t_codeEnum getWaveformEnum(uint8_t code){
    for(int I=0; I < sizetypeCodeTable; I++){
      if (typeCodeTable[I].code == code){
        return typeCodeTable[I].codeEnum;
      }
    }
    return typeCodeTable[sizetypeCodeTable-1].codeEnum;
}

inline std::string getWaveformName(t_codeEnum  codeEnum){
    for(int I=0; I < sizetypeCodeTable; I++){
      if (typeCodeTable[I].codeEnum == codeEnum){
        return typeCodeTable[I].name;
      }
    }
    return typeCodeTable[sizetypeCodeTable-1].name;
}

inline uint8_t  getWaveformCode(std::string name){
    for(int I=0; I < sizetypeCodeTable; I++){
      if (typeCodeTable[I].name == name){
        return typeCodeTable[I].code;
      }
    }
    return typeCodeTable[sizetypeCodeTable-1].code;
}

inline uint8_t  getWaveformCode(t_codeEnum codeEnum){
    for(int I=0; I < sizetypeCodeTable; I++){
      if (typeCodeTable[I].codeEnum == codeEnum){
        return typeCodeTable[I].code;
      }
    }
    return typeCodeTable[sizetypeCodeTable-1].code;
}


};

#endif