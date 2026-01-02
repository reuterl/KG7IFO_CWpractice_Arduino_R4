#ifndef BUZZWAVE_H_
#define BUZZWAVE_H_

#include <arduino.h>

typedef struct SoundWaveInfo {
  const uint16_t * pSoundWave;
  const uint32_t * length;
} st_SoundWaveInfo;

extern const st_SoundWaveInfo buzzWave;

#endif