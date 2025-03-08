#include "chip8.h"
#include <fstream>
#include <iostream>


Chip8::Chip8(std::string fname) {
    running_flag = load_ROM(fname);
    load_instructions();
    memcpy(memory, FONTSET, sizeof(uint8_t) * FONTSET_SIZE);
}


bool Chip8::load_ROM(const std::string &fname) {
    std::ifstream file(fname, std::ios::binary | std::ios::ate);
    if (!file) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to open input file.");
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > MEMORY_SIZE - PROGRAM_START) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Input file is too big.\n");
        return false;
    }

    file.read(reinterpret_cast<char *>(&memory[PROGRAM_START]), size);

    return true;
}


uint16_t Chip8::fetch() {
    if (PC > MEMORY_SIZE) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Reached end of instructions.");
        return 0;
    }

    uint16_t opcode = memory[PC] << 8 | memory[PC + 1];
    PC += 2;
    return opcode;
}


void Chip8::execute_loop() {
    uint16_t opcode = fetch();
    InstructionFunc func = instruction_funcs[(opcode & 0xF000) >> 12];
    if (!func) {
        if (exit_on_unknown) {
            running_flag = false;
        }
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unknown opcode: %04X", opcode);
    } else {
        (this->*func)(opcode);
    }

    if (stepping) {
        execute_next = false;
    }
}


void Chip8::opcode_00E0(uint16_t opcode) {
    if (debug) {
        SDL_Log("Called %04X: Clear display", opcode);
    }
    memset(display, 0, sizeof(uint8_t) * LOGICAL_WIDTH * LOGICAL_HEIGHT);
    draw_flag = true;
}


void Chip8::opcode_1NNN(uint16_t opcode) {
    uint16_t NNN = opcode & 0x0FFF;

    if (debug) {
        SDL_Log("Called %04X: Jump to %03X", opcode, NNN);
    }

    PC = NNN;
}

void Chip8::opcode_3XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint16_t NN = opcode & 0x00FF;

    if (debug) {
        SDL_Log("Called %04X: Skip next instruction if V%01X (%02X) == %02X", opcode, X, V[X], NN);
    }

    if (V[X] == NN) {
        PC += 2;
    }
}

void Chip8::opcode_4XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint16_t NN = opcode & 0x00FF;

    if (debug) {
        SDL_Log("Called %04X: Skip next instruction if V%01X (%02X) != %02X", opcode, X, V[X], NN);
    }

    if (V[X] != NN) {
        PC += 2;
    }
}

void Chip8::opcode_5XY0(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;

    if (debug) {
        SDL_Log("Called %04X: Skip next instruction if V%01X (%02X) = V%01X (%02X)", opcode, X, V[X], Y, V[Y]);
    }

    if (V[X] == V[Y]) {
        PC += 2;
    }

}


void Chip8::opcode_6XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint16_t NN = opcode & 0x00FF;

    if (debug) {
        SDL_Log("Called %04X: Set V%01X = %02X", opcode, X, NN);
    }
    V[X] = NN;
}

void Chip8::opcode_7XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint16_t NN = opcode & 0x00FF;

    V[X] += NN;

    if (debug) {
        SDL_Log("Called %04X: Add %02X to V%01X. V%01X is now set to %02X", opcode, NN, X, X, V[X]);
    }
}

void Chip8::opcode_9XY0(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;

    if (debug) {
        SDL_Log("Called %04X: Skip next instruction if V%01X (%02X) != V%01X (%02X)", opcode, X, V[X], Y, V[Y]);
    }

    if (V[X] != V[Y]) {
        PC += 2;
    }

}

void Chip8::opcode_ANNN(uint16_t opcode) {
    uint16_t NNN = opcode & 0x0FFF;
    if (debug) {
        SDL_Log("Called %04X: Set I = %03X", opcode, NNN);
    }
    I = NNN;
}

void Chip8::opcode_DXYN(uint16_t opcode) {
    if (debug) {
        SDL_Log("Called %04X: Draw", opcode);
    }
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    uint8_t N = opcode & 0x000F;

    uint8_t x = V[X] % LOGICAL_WIDTH;
    uint8_t y = V[Y] % LOGICAL_HEIGHT;
    uint8_t pixel;

    V[0xF] = 0;

    //for all N rows:
    for (uint8_t y_coord = 0; y_coord < N; ++y_coord) {
        //get the pixel data for that row
        pixel = memory[I + y_coord];
        //for each bit
        for (uint8_t x_coord = 0; x_coord < 8; ++x_coord) {
            //if the pixel in the sprite is on
            if ((pixel & (0x80 >> x_coord)) != 0) {
                //do not clip if we go over, instead skip to the next row
                if (x + x_coord >= LOGICAL_WIDTH || y + y_coord >= LOGICAL_HEIGHT) {
                    continue;
                }

                //set the pixel value in display by XORing it with the pixel value (1)
                //if this causes a pixel to be erased, set VF = 1
                if (display[x + x_coord + ((y + y_coord) * LOGICAL_WIDTH)]) {
                    V[0xF] = 1;
                }

                display[x + x_coord + ((y + y_coord) * LOGICAL_WIDTH)] ^= 1;
            }
        }
    }

    draw_flag = true;
}


void Chip8::load_instructions() {
    instruction_funcs[0x0] = &Chip8::opcode_00E0;
    instruction_funcs[0x1] = &Chip8::opcode_1NNN;
    instruction_funcs[0x3] = &Chip8::opcode_3XNN;
    instruction_funcs[0x4] = &Chip8::opcode_4XNN;
    instruction_funcs[0x5] = &Chip8::opcode_5XY0;
    instruction_funcs[0x6] = &Chip8::opcode_6XNN;
    instruction_funcs[0x7] = &Chip8::opcode_7XNN;
    instruction_funcs[0x9] = &Chip8::opcode_9XY0;
    instruction_funcs[0xA] = &Chip8::opcode_ANNN;
    instruction_funcs[0xD] = &Chip8::opcode_DXYN;
}

void Chip8::draw(Screen &screen) {
    if (draw_flag) {
        screen.draw(display);
        draw_flag = false;
    }
}

void Chip8::update_inputs() {
    SDL_Event e;
    memcpy(prev_keyboard, keyboard, sizeof(uint8_t) * KEY_COUNT);
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running_flag = false;
            break;
        } else if (e.type == SDL_KEYDOWN) {
            for (int i = 0; i < KEY_COUNT; i++) {
                if (e.key.keysym.scancode == KEYMAP[i]) {
                    keyboard[i] = true;
                }
            }
        } else if (e.type == SDL_KEYUP) {
            if (e.key.keysym.scancode == EXIT_BUTTON) {
                running_flag = false;
                break;
            }

            if (e.key.keysym.scancode == PAUSE_BUTTON) {
                stepping = !stepping;
                if (stepping) {
                    execute_next = false;
                }
            }

            if (e.key.keysym.scancode == STEP_BUTTON) {
                execute_next = true;
            }


            for (int i = 0; i < KEY_COUNT; i++) {
                if (e.key.keysym.scancode == KEYMAP[i]) {
                    keyboard[i] = false;
                }
            }
        }
    }
}
