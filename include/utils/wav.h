#ifndef __WAV_H__
#define __WAV_H__

#include "types.h"
#include "utils/serializer.h"

typedef struct wav_t {
    u8* data;
    size_t size;
    int sample_rate;
    int bytes_per_sample;
    int channels;
} wav_t;

wav_t wav_load(const u8* data, size_t size);
void wav_free(wav_t* wav);

#endif