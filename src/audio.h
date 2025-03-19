#ifndef CHIP8_AUDIO_H
#define CHIP8_AUDIO_H

#include <SDL3/SDL.h>


const int SAMPLE_RATE = 44100;
const int AMPLITUDE = 28000;
const int FREQUENCY = 440;

class Audio {
public:
    bool is_beeping = false;
    int phase = 0;

    void init_audio();
private:
    SDL_AudioStream *audio_stream;

};

void callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);


#endif //CHIP8_AUDIO_H
