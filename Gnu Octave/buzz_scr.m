#
# Get sample vector from audacity .wav file
#
buzz=audioread(".\\documents\\ringtones\\buzz_wave.wav");

# Samples are pos/negative values around zero. Find lowest peak,
# and offset that to zero. Move entire vector up by same amount.
buzz_pos = buzz-(min(buzz));
#
# scale LSB to 12 bit (4096 step) DAC output.
#
buzz_dac = buzz_pos*(4096/max(buzz_pos));
#
# and convert to integer DAC values.
#
buzz_bits = int16(buzz_dac);
plot(buzz_bits);


fid=fopen("buzz.txt", "w");
fprintf(fid, "#include \"BuzzWave.h\"\nconst uint16_t buzzWaveform[] = {\n");
fprintf(fid,"0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, \n",int16(buzz_bits));
fprintf(fid, "};\n\nconst uint16_t buzzSampleCount = sizeof(buzzWaveform)/sizeof(uint16_t);\n");
fprintf(fid,"\nconst st_SoundWaveInfo buzzWave = {\n  buzzWaveform,\n  &buzzSampleCount\n};\n");

fprintf(fid,"\n\n#ifndef BUZZWAVE_H_\n\n");
fprintf(fid,"#define BUZZWAVE_H_\n");

fprintf(fid,"#include <arduino.h>\n\n");

fprintf(fid,"typedef struct SoundWaveInfo {\n");
fprintf(fid,"  const uint16_t * pSoundWave;\n");
fprintf(fid,"  const uint32_t * length;\n");
fprintf(fid,"} st_SoundWaveInfo;\n\n");

fprintf(fid,"extern const st_SoundWaveInfo buzzWave;\n\n");

fprintf(fid,"#endif\n\n");
fclose(fid);