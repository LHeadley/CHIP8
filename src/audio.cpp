#include "audio.h"
#include <iostream>

void Audio::init_audio() {
    SDL_AudioSpec spec;
    spec.channels = 1;
    spec.freq = SAMPLE_RATE;
    spec.format = SDL_AUDIO_S16LE;

    audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    
    SDL_SetAudioStreamGetCallback(audio_stream, callback, this);
    SDL_ResumeAudioStreamDevice(audio_stream);
}

void callback(void* userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    Audio *audio = (Audio *) userdata;
    int len = additional_amount / sizeof(int16_t);
    int16_t buff[len];
    for(int i = 0; i < len; i++) {
        if(audio->is_beeping)
            buff[i] = (audio->phase < (SAMPLE_RATE / FREQUENCY) / 2) ? AMPLITUDE : -AMPLITUDE;
        else
            buff[i] = 0;
        
        audio->phase = (audio->phase + 1) % (SAMPLE_RATE / FREQUENCY);
    }

    SDL_PutAudioStreamData(stream, buff, sizeof(buff));
}