#include "screen.h"
#include "chip8.h"

Screen::Screen() {
    window = SDL_CreateWindow("CHIP-8",
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, nullptr);

    //setting the logical size lets us just treat it as a 64 x 32 display, and it will automatically scale it up
    SDL_SetRenderLogicalPresentation(renderer, LOGICAL_WIDTH, LOGICAL_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                LOGICAL_WIDTH,
                                LOGICAL_HEIGHT);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
}

Screen::~Screen() {
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_Quit();

}

void Screen::draw(uint8_t *display) {
    uint32_t screen[LOGICAL_WIDTH * LOGICAL_HEIGHT] = {0};
    for (int i = 0; i < LOGICAL_WIDTH; i++) {
        for (int j = 0; j < LOGICAL_HEIGHT; j++) {
            if (display[i + j * LOGICAL_WIDTH] == 1) {
                screen[i + j * LOGICAL_WIDTH] = UINT32_MAX;
            }
        }
    }

    SDL_UpdateTexture(texture, nullptr, screen, LOGICAL_WIDTH * sizeof(uint32_t));
    SDL_FRect position;
    position.x = 0;
    position.y = 0;

    position.w = LOGICAL_WIDTH;
    position.h = LOGICAL_HEIGHT;
    SDL_RenderTexture(renderer, texture, nullptr, &position);
    SDL_RenderPresent(renderer);
}