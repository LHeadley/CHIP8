#include <iostream>
#include <SDL3/SDL.h>
#include <getopt.h>
#include <thread>
#include "chip8.h"
#include "audio.h"

int main(int argc, char *argv[]) {
    int c;
    int ipf = 11;
    bool debug = false;
    bool exit_on_unknown = true;
    bool increment_I_on_index = false;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO);

    const struct option longopts[] = {
            {"ignore",         no_argument,       nullptr, 'e'},
            {"debug",          no_argument,       nullptr, 'd'},
            {"ipf",            required_argument, nullptr, 'i'},
            {"inc-i-on-index", no_argument,       nullptr, 'c'},
            {nullptr,          0,                 nullptr, 0}
    };

    int index;

    while ((c = getopt_long(argc, argv, "edi:", longopts, &index)) != -1) {
        switch (c) {
            case 'e':
                exit_on_unknown = false;
                break;
            case 'd':
                debug = true;
                break;
            case 'i':
                ipf = atoi(optarg);
                break;
            case 'c':
                increment_I_on_index = true;
                break;
            default:
                abort();
        }
    }

    if (argc - optind != 1) {
        std::cerr << "Usage: ./chip8 [options] input.ch8\n";
        return 0;
    }
    Chip8 chip8(argv[optind++], debug, exit_on_unknown, increment_I_on_index);
    if (!chip8.isRunning()) {
        return 0;
    }
    Screen screen;

    while (chip8.isRunning()) {
        chip8.update_inputs();
        chip8.decrement_timers();

        auto frame_start = std::chrono::high_resolution_clock::now();
        if (!chip8.isStepping()) {
            for (int i = 0; i < ipf && !chip8.is_draw_flag(); ++i) {
                chip8.execute_loop();
            }
        } else if (chip8.should_execute_next()) {
            chip8.execute_loop();
        }

        chip8.draw(screen);

        if (!chip8.isStepping()) {
            std::this_thread::sleep_until(frame_start + std::chrono::nanoseconds (16666667));
        }
    }


    return 0;
}
