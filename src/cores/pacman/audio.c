#include "cores/pacman/audio.h"
#include "utils/sound.h"
#include <string.h>

static int pacman_generate_sample(pacman_t* p)
{
    if (!p->SOUND_ENABLED)
        return 0;

    int out = 0;
    u32 freq;
    u8  waveform, idx, samp, vol;

    /* Voice 1 — 5-nibble frequency, dedicated volume register */
    freq = 0;
    for (int i = 0; i < 5; i++)
        freq |= (u32)(p->VOICE1_FREQ[i] & 0xF) << (i * 4);
    p->voiceAccumulator[0] = (p->voiceAccumulator[0] + freq) & 0xFFFFF;
    waveform = p->SOUND_VOICE1[5] & 0x7;
    idx  = (u8)(p->voiceAccumulator[0] >> 15);
    samp = p->audioROM[32 * waveform + idx] & 0xF;
    vol  = p->VOICE1_VOLUME & 0xF;
    out += sound_set_channel_sample(samp * vol, 0);

    /* Voice 2 — 4-nibble frequency (shifted <<4), volume in VOICE2_FREQ_VOL[4] */
    freq = 0;
    for (int i = 0; i < 4; i++)
        freq |= (u32)(p->VOICE2_FREQ_VOL[i] & 0xF) << (i * 4 + 4);
    p->voiceAccumulator[1] = (p->voiceAccumulator[1] + freq) & 0xFFFFF;
    waveform = p->SOUND_VOICE2[4] & 0x7;
    idx  = (u8)(p->voiceAccumulator[1] >> 15);
    samp = p->audioROM[32 * waveform + idx] & 0xF;
    vol  = p->VOICE2_FREQ_VOL[4] & 0xF;
    out += sound_set_channel_sample(samp * vol, 1);

    /* Voice 3 */
    freq = 0;
    for (int i = 0; i < 4; i++)
        freq |= (u32)(p->VOICE3_FREQ_VOL[i] & 0xF) << (i * 4 + 4);
    p->voiceAccumulator[2] = (p->voiceAccumulator[2] + freq) & 0xFFFFF;
    waveform = p->SOUND_VOICE3[4] & 0x7;
    idx  = (u8)(p->voiceAccumulator[2] >> 15);
    samp = p->audioROM[32 * waveform + idx] & 0xF;
    vol  = p->VOICE3_FREQ_VOL[4] & 0xF;
    out += sound_set_channel_sample(samp * vol, 2);

    return out;
}

static void pacman_get_sample(void* userdata, void* sample_data)
{
    pacman_t* p = (pacman_t*)userdata;
    i16 sample;

    pacman_generate_sample(p); /* discard first WSG tick */
    sample = (i16)(pacman_generate_sample(p) * PACMAN_AUDIO_GAIN);
    memcpy(sample_data, &sample, sizeof(sample));
}

void PACMAN_push_sample(pacman_t* p, int cycles)
{
    i16 sample;
    sound_push_sample(cycles, sizeof(sample), p, &sample, pacman_get_sample);
}
