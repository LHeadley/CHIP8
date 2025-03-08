#include <iostream>
#include <SDL.h>
#include <getopt.h>
#include "chip8.h"

int main(int argc, char *argv[]) {
    char c;
    int ipf = 11;
    bool debug = false;
    bool exit_on_unknown = true;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    const struct option longopts[] = {
            {"exit-on-unknown", required_argument, nullptr, 'e'},
            {"debug", no_argument, nullptr, 'd'},
            {"ipf", required_argument, nullptr, 'i'},
            {nullptr, 0, nullptr, 0}
    };

    int index;

    while ((c = getopt_long(argc, argv, "e:di:", longopts, &index)) != -1) {
        switch (c) {
            case 'e':
                exit_on_unknown = optarg[0] == '1';
                break;
            case 'd':
                debug = true;
                break;
            case 'i':
                ipf = atoi(optarg);
                break;
            default:
                abort();
        }
    }

    if (argc - optind != 1) {
        SDL_Log("Usage: ./chip8 input.ch8");
        return 0;
    }
    Chip8 chip8(argv[optind++], debug, exit_on_unknown);
    if (!chip8.isRunning()) {
        return 0;
    }
    Screen screen;


    while (chip8.isRunning()) {
        chip8.update_inputs();
        if (!chip8.isStepping()) {
            for (int i = 0; i < ipf; ++i) {
                chip8.execute_loop();
            }
        } else if (chip8.should_execute_next()) {
            chip8.execute_loop();
        }

        chip8.draw(screen);
    }


    return 0;
}
