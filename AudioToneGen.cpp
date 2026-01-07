/*
 * AudioToneGen.cpp
 *
 *  Created on: Jun 24, 2023
 *      Author: luke
 */

#include "AudioToneGen.h"
#include <math.h>
#include <SoftwareSerial.h>

extern char sSprintf[];
#define SPRINTF(FMT, args...) \
  sprintf(sSprintf, FMT, args); \
  Serial.print(sSprintf);

extern SoftwareSerial SwSerial;
extern char sSprintf[];

AudioToneGen::AudioToneGen(uint8_t nTones, float nyquist) {

  numTones = nTones;
  Fnyquist = nyquist;
  stepSize = 1.0f / Fnyquist;
  // Initial common mode amplitude
  CommModeAmpl = 0.3f;
  CommModeAmpl = 1.0f;

  //toneList = new Tone_t[numTones];
  toneList = std::make_unique<Tone_t[]>(numTones);
  // Initialize all entries to undefined.
  for (int I = 0; I < numTones; I++) {
    Tone_t *tone = &toneList[I];
    tone->enabled = false;
    tone->synch = false;
    tone->defined = false;
    tone->nSamples = 0;
    tone->dacSample = NULL;
    tone->Amplitude = 1.0f;
    tone->freq = 0.0f;
    tone->phase = 0.0f;
  }
  sparkGap = false;
}

AudioToneGen::~AudioToneGen() {
  for (int idx = 0; idx < numTones; idx++) {
    clearTone(idx);
  }
  //delete toneList;
}

uint16_t AudioToneGen::getNsamples(uint8_t idx) {
  Tone_t *tone = &toneList[idx];
  return tone->nSamples;
}

void AudioToneGen::setSparkGap(bool enable){
  sparkGap = enable;
}

bool AudioToneGen::getSparkGap(void){
  return sparkGap;
}

void AudioToneGen::clearTone(uint8_t idx) {
  Tone_t *tone = &toneList[idx];

  tone->synch = false;
  tone->Amplitude = 666.0;
  tone->enabled = false;
  tone->defined = false;
  tone->nSamples = 0;
  tone->status = 0;
  if (tone->dacSample != NULL) {
    delete tone->dacSample;
  }
  tone->dacSample = NULL;
}

uint8_t AudioToneGen::findAvailable(void) {
  for (uint8_t I = 0; I < getNumTones(); I++) {
    if (toneList[I].defined == false) {
      return I;
    }
  }
  return 0xFF;
}

uint8_t AudioToneGen::findHandle(uint8_t handle) {
  for (uint8_t I = 0; I < getNumTones(); I++) {
    if (toneList[I].handle == handle) {
      return I;
    }
  }
  return 0xFF;
}

uint8_t AudioToneGen::generateHandle(void) {
  return handle++;
}

bool AudioToneGen::getDefined(uint8_t idx) {
  return toneList[idx].defined;
}

uint8_t AudioToneGen::getHandle(uint8_t idx) {
  return toneList[idx].handle;
}

void AudioToneGen::setDefined(uint8_t idx, bool defined) {
  toneList[idx].defined = defined;
}

void AudioToneGen::setHandle(uint8_t idx, uint8_t handle) {
  toneList[idx].handle = handle;
}

// Available table size.
uint8_t AudioToneGen::getNumTones(void) {
  return numTones;
}

float AudioToneGen::getNyquist(void) {
  return Fnyquist;
}

void AudioToneGen::setCommModeAmpl(float Amplitude) {
  CommModeAmpl = Amplitude;
}

float AudioToneGen::getCommModeAmpl(void) {
  return CommModeAmpl;
}


float AudioToneGen::getAmplitude(uint8_t idx) {
  Tone_t *tone = &toneList[idx];
  if (tone->defined == false) {
    return 0.0f;
  } else {
    return tone->Amplitude;
  }
}

float AudioToneGen::getFreq(uint8_t idx) {
  Tone_t *tone = &toneList[idx];
  if (tone->defined == false) {
    return 0.0f;
  } else {
    return tone->freq;
  }
}

float AudioToneGen::getPhase(uint8_t idx) {
  Tone_t *tone = &toneList[idx];
  if (tone->defined == false) {
    return 0.0f;
  } else {
    return tone->phase;
  }
}

SerialWaveType::t_codeEnum AudioToneGen::getWaveformType(uint8_t idx) {
  Tone_t *tone = &toneList[idx];
  if (tone->defined == false) {
    return SerialWaveType::t_codeEnum::Invalid;
  } else {
    return tone->WaveformType;
  }
}

bool AudioToneGen::generateWaveform(uint8_t idx) {
  uint16_t nSamples;
  Tone_t *tone = &toneList[idx];
  if (tone->freq > 0) {
    nSamples = (uint16_t)(Fnyquist / tone->freq) + 0.5f;
  } else {
    tone->nSamples = 0;
    tone->enabled = false;
    tone->defined = true;
    if (tone->dacSample != NULL) {
      delete tone->dacSample;
      tone->dacSample = NULL;
    }
    return true;
  }

  tone->synch = false;
  //tone->enabled = false;
  tone->defined = true;

  //sprintf(sSprintf, "Sample = %d  tone->nSamples = %d\r\n", nSamples, tone->nSamples);
  //Serial.print(sSprintf);
  if (tone->dacSample != NULL) {
    if (nSamples > tone->nSamples) {
      delete tone->dacSample;
      tone->dacSample = new uint16_t[nSamples];
    }
  } else {
    tone->dacSample = (uint16_t *)malloc(sizeof(uint16_t) * nSamples);
    if (tone->dacSample == NULL) {
      sprintf(sSprintf, "\n\n>>>>>>>>> [%d]tone->dacSample is NULL.  No more memory.\r\n", idx);
      Serial.print(sSprintf);
      tone->status = 0xE0;
      return false;
    }
    /*
    try{
    tone->dacSample = new uint16_t[nSamples];
    }catch (std::bad_alloc const&){
    sprintf(sSprintf, "\n\n>>>>>>>>> [%d]tone->dacSample is NULL.  No more memory.\r\n", idx);
    Serial.print(sSprintf);
    tone->status = 0xE0;
    return false;
    }
    */
  }
  tone->nSamples = nSamples;

  switch (tone->WaveformType) {
    case SerialWaveType::t_codeEnum::Sine:
      generateWaveformSine(tone);
      break;
    case SerialWaveType::t_codeEnum::Sawtooth:
      generateWaveformSawtooth(tone);
      break;
    case SerialWaveType::t_codeEnum::Square:
      generateWaveformSquare(tone);
      break;
    case SerialWaveType::t_codeEnum::Triangle:
      generateWaveformTriangle(tone);
      break;
    case SerialWaveType::t_codeEnum::Invalid:
    default:
      //PRINTF("\r\nInvalid/default waveform type! 0x%02X\r\n", tone->WaveformType);
      break;
  }
  tone->sampleIdx = 0;

  return true;
}

bool AudioToneGen::setTone(uint8_t idx, float A, float F, float Ph, SerialWaveType::t_codeEnum _Waveform) {
  //if (idx >= getNumTones()) {
  //  return false;
  //}
  Tone_t *tone = &toneList[idx];
  tone->defined = true;
  //tone->enabled = false; Don't change, if this is an update
  tone->WaveformType = _Waveform;
  tone->Amplitude = A;
  tone->freq = F;
  tone->phase = Ph;
  bool status = generateWaveform(idx);
  if (!status) {  // Probably out of memory
    return false;
  }
  /*
  Serial.print("\nSamples = ");
  Serial.println(tone->nSamples);
  for (uint8_t q = 0; q < tone->nSamples; q++) {
    uint16_t S = getToneSample(0);
    //Serial.println(S);
  }
  */
  // Serial.println("\nDone");
  return status;
}

bool AudioToneGen::setSound(uint8_t idx, const st_SoundWaveInfo * SoundWaveInfo){
  Tone_t *tone = &toneList[idx];
  tone->defined = true;
  //tone->enabled = false; Don't change, if this is an update
  tone->WaveformType =  SerialWaveType::t_codeEnum::Sound;
  tone->dacSample = (uint16_t *)SoundWaveInfo->pSoundWave;
  tone->nSamples = *SoundWaveInfo->length;
  SPRINTF("Set Recorded Sound: Samples = %d, duration = %5.1f seconds\n", tone->nSamples, (float)tone->nSamples/Fnyquist );
}

bool AudioToneGen::setFreq(uint8_t idx, float freq) {
  Tone_t *tone = &toneList[idx];

  tone->freq = freq;

  return generateWaveform(idx);
}

void AudioToneGen::generateWaveformSine(Tone_t *tone) {
  int S;
  float step = 0;
  float Y;
  float phaseRadians = tone->phase * DtoRadians;  // Convert to radians

  for (S = 0; S < tone->nSamples; S++) {
    Y = CommModeAmpl * (tone->Amplitude * sinf((twoPi * tone->freq * step) + phaseRadians));
    tone->dacSample[S] = 0x0FFF & (uint16_t)(2047.0F * (1.0 + Y));
    //PRINTF("%03d Sin(x) = %1.7f  DAC = %03X (%d)\r\n", N++, Y, tone->dacSample[S], tone->dacSample[S]);
    step += stepSize;
  }
}

void AudioToneGen::generateWaveformSquare(Tone_t *tone) {
  int S;
  float Y;
  uint16_t halfway = tone->nSamples / 2;

  for (S = 0; S < tone->nSamples; S++) {
    if (S < halfway) {
      Y = CommModeAmpl * (+1.0f);
    } else {
      Y = CommModeAmpl * (-1.0f);
    }
    tone->dacSample[S] = 0x0FFF & (uint16_t)(2047.0F * (1.0 + Y));
  }
}
void AudioToneGen::generateWaveformSawtooth(Tone_t *tone) {
  int S;
  float Y = -1.0f;
  float step = 2.0f / (float)tone->nSamples;

  for (S = 0; S < tone->nSamples; S++) {
    Y += step;
    tone->dacSample[S] = 0x0FFF & (uint16_t)(2047.0F * (1.0 + (CommModeAmpl * Y)));
  }
}
void AudioToneGen::generateWaveformTriangle(Tone_t *tone) {
  int S;
  float Y = -1.0f;
  uint16_t halfway = tone->nSamples / 2;
  float step = 4.0f / (float)tone->nSamples;

  for (S = 0; S < tone->nSamples; S++) {
    if (S < halfway) {
      Y += step;
    } else {
      Y -= step;
    }
    tone->dacSample[S] = 0x0FFF & (uint16_t)(2047.0F * (1.0 + (CommModeAmpl * Y)));
  }
}

void AudioToneGen::setEnabled(uint8_t idx, bool enabled) {
  Tone_t *tone = &toneList[idx];

  if (tone->defined == false) {
    return;
  } else {
    tone->enabled = enabled;
  }
}

bool AudioToneGen::getEnabled(uint8_t idx) {
  Tone_t *tone = &toneList[idx];
  if (tone->defined == false) {
    return false;
  } else {
    return tone->enabled;
  }
}

void AudioToneGen::setStatus(uint8_t idx, uint8_t status) {
  Tone_t *tone = &toneList[idx];

  if (tone->defined == false) {
    return;
  } else {
    tone->status = status;
  }
}

uint8_t AudioToneGen::getStatus(uint8_t idx) {
  Tone_t *tone = &toneList[idx];
  if (tone->defined == false) {
    return 0xE1;
  } else {
    return tone->status;
  }
}

void AudioToneGen::Sync(void) {
  for (int idx = 0; idx < numTones; idx++) {
    Tone_t *tone = &toneList[idx];
    if (tone->defined == true) {
      tone->synch = true;
    }
  }
}

uint16_t AudioToneGen::getToneSample(uint8_t idx) {
  Tone_t *tone = &toneList[idx];
  uint16_t sample = 0;

  if (tone->defined == false) {
    return 0;
  } else {
    if (tone->synch == true) {
      tone->synch = false;
      tone->sampleIdx = 0;
    }
    sample = tone->dacSample[tone->sampleIdx++];
    if (tone->sampleIdx >= tone->nSamples) {
      tone->sampleIdx = 0;
    }
  }
  return sample;
}

// Called each Nyquist period (ISR).  Add, average current sample from
// each defined, enabled tone.
uint16_t AudioToneGen::mixer(void) {
  uint16_t mixerProduct = 0;
  uint16_t mixCount = 0;
  uint16_t sample;
  static uint16_t lastSample = 0;

  for (int idx = 0; idx < numTones; idx++) {
    sample = getToneSample(idx);
    if (getEnabled(idx) == true) {
      mixerProduct += sample;
      mixCount++;
    }
  }
  if (mixCount != 0) {
    lastSample = mixerProduct / mixCount;
    return lastSample;
  } else {
    return lastSample;
  }
}
