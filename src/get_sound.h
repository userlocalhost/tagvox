#ifndef __GET_SOUND__
#define __GET_SOUND__

#include "common.h"

/*
 * 16 (bits) * 44100 (Hz) * 5 (sec) = 3528000 (bits)
 *                                  = 441000  (bytes)
 */
#define BUFSIZE 44100
#define SAMPLING_RATE 44100
#define QUANTIZATION_BIT 16

#define QUANTIZED_BIT 16

#define WARMUP_STATE 1
#define VIEW_RANGE 3

#define SoundOUTPUT "sound.data"

int get_sound(short *, int);
void write_sound_log(short *, int);

#endif
