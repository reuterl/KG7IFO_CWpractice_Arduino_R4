/*
 * AudioToneGen.h
 *
 *  Created on: Jun 24, 2023
 *      Author: luke
 */

#ifndef AUDIOTONEGEN_H_
#define AUDIOTONEGEN_H_
#include <string>
#include <stdint.h>
#include <math.h>
#include "SerialWaveType.h"
#include "BuzzWave.h"


class AudioToneGen : public SerialWaveType {
private:
  uint8_t handle = 1;
  bool sparkGap;
public:
  AudioToneGen(uint8_t nTones, float nyquist);
  virtual ~AudioToneGen();

  void Sync(void);

  void setCommModeAmpl(float Amplitude);
  float getCommModeAmpl(void);


  bool setTone(uint8_t idx, float A, float F, float Ph, SerialWaveType::t_codeEnum _Waveform);
  bool setSound(uint8_t idx, const st_SoundWaveInfo * SoundWaveInfo);
  
  // Don't really belong here, but I didn't want them global.
  // in common with processcommands and keyPress
  bool getSparkGap(void);
  void setSparkGap(bool enable);
  //
  void clearTone(uint8_t idx);
  uint16_t mixer(void);
  float getNyquist(void);
  uint8_t getNumTones(void);
  uint16_t getNsamples(uint8_t idx);

  bool getEnabled(uint8_t idx);
  void setEnabled(uint8_t idx, bool enabled);

  uint8_t getStatus(uint8_t idx);
  void setStatus(uint8_t idx, uint8_t status);

  bool getDefined(uint8_t idx);
  void setDefined(uint8_t idx, bool defined);

  uint8_t generateHandle(void);
  uint8_t getHandle(uint8_t idx);
  void setHandle(uint8_t idx, uint8_t handle);

  float getAmplitude(uint8_t idx);
  float getFreq(uint8_t idx);
  bool setFreq(uint8_t idx, float freq);
  float getPhase(uint8_t idx);

  SerialWaveType::t_codeEnum getWaveformType(uint8_t idx);

  uint8_t findAvailable(void);
  uint8_t findHandle(uint8_t handle);

  inline uint8_t getNumDefined(void){
    uint8_t numDefined = 0;
    for (uint8_t i = 0; i < getNumTones(); i++){
      if (getDefined(i)){
        numDefined++;
      }
    }
    return numDefined;
  }

  private:

    uint8_t numTones;
    float Fnyquist;
    const float twoPi = (2.0f * M_PI);
    float stepSize;
    float CommModeAmpl;
    const float DtoRadians = M_PI / 180.0f;
    bool generateWaveform(uint8_t idx);

    typedef struct Tone_s {
      SerialWaveType::t_codeEnum WaveformType;
      float Amplitude;
      float freq;
      float phase;
      bool defined;
      bool enabled;
      bool synch;
      uint16_t *dacSample;
      uint32_t nSamples;
      uint16_t sampleIdx;
      uint8_t handle;
      uint8_t status;
    } Tone_t;


    uint16_t getToneSample(uint8_t idx);
    void generateWaveformSine(Tone_t * tone);
    void generateWaveformSquare(Tone_t * tone);
    void generateWaveformSawtooth(Tone_t * tone);
    void generateWaveformTriangle(Tone_t * tone);


    Tone_t *toneList;
  };

#endif /* AUDIOTONEGEN_H_ */
