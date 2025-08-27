#include "cores/pv1000/pv1000.h"
#include "cores/pv1000/psg.h"

#include <math.h>

static void send_samples(psg_t* psg, Uint8 * stream, int len){
    for(int i = 0; i < len; i++){
        Uint8 samples[3] = {0, 0, 0};
        for(int j = 0; j < 3; j++){
            if(!psg->square_freq[j])
                continue;
            samples[j] = sin(psg->time_elapsed*2*M_PI*psg->square_freq[j]) > 0; 
        }
        if(psg->mixer){
            samples[0] = samples[0] ^ samples[1];
            samples[1] = samples[1] ^ samples[2];
        }
        stream[i] = samples[0] + (samples[1] << 1) + (samples[2] << 2);
        stream[i] = ((int8_t)stream[i]) * HOST_GAIN;
        psg->time_elapsed += psg->updateRate;
    }
}

static void send_silence(psg_t* psg, Uint8 * stream, int len){
    for(int i = 0; i < len; i++)
        stream[i] = 0;
    psg->time_elapsed = 0;
}

void pv1000_psg_callback(void *userdata, Uint8 * stream, int len){
    pv1000_t* pv1000 = userdata;
    psg_t* psg = &pv1000->psg;

    if(psg->enabled)
        send_samples(psg, stream, len);
    else
        send_silence(psg, stream, len);
}