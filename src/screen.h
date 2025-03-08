#ifndef CHIP8_DISPLAY_H
#define CHIP8_DISPLAY_H

#include <SDL3/SDL.h>

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 320;

class Screen {
public:
    Screen();
    ~Screen();
    void draw(uint8_t *display);

private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

};


#endif //CHIP8_DISPLAY_H
