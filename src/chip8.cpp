#include "chip8.h"
#include <fstream>
#include <iostream>
#include <format>


Chip8::Chip8(std::string fname) {
    running_flag = load_ROM(fname);
    load_instructions();
    memcpy(memory, FONTSET, sizeof(uint8_t) * FONTSET_SIZE);
}


bool Chip8::load_ROM(const std::string &fname) {
    std::ifstream file(fname, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "ERROR: Failed to open input file\n";
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > MEMORY_SIZE - PROGRAM_START) {
        std::cerr << "ERROR: Input file is too big\n";
        return false;
    }

    file.read(reinterpret_cast<char *>(&memory[PROGRAM_START]), size);

    return true;
}


uint16_t Chip8::fetch() {
    if (PC > MEMORY_SIZE) {
        std::cerr << "ERROR: Reached end of instructions\n";
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
        std::cerr << std::format("ERROR: Unknown opcode: {:04X}\n", opcode);
    } else {
        (this->*func)(opcode);
    }

    if (stepping) {
        execute_next = false;
    }
}


void Chip8::opcode_00E_(uint16_t opcode) {
    uint8_t opt = (opcode & 0x000F);
    if (opt == 0x0) {
        opcode_00E0(opcode);
    } else if (opt == 0xE) {
        opcode_00EE(opcode);
    } else {
        if (exit_on_unknown) {
            running_flag = false;
        }
        std::cerr << std::format("ERROR: Unknown opcode: {:04X}\n", opcode);
    }
}


void Chip8::opcode_00E0(uint16_t opcode) {
    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Clear display\n", opcode);
    }
    memset(display, 0, sizeof(uint8_t) * LOGICAL_WIDTH * LOGICAL_HEIGHT);
    draw_flag = true;
}


void Chip8::opcode_00EE(uint16_t opcode) {
    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Return from subroutine\n", opcode);
    }
    if (SP == 0) {
        std::cerr << "ERROR: Attempted stack underflow.\n";
        running_flag = false;
    } else {
        PC = stack[--SP];
    }
}


void Chip8::opcode_1NNN(uint16_t opcode) {
    uint16_t NNN = opcode & 0x0FFF;
    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Jump to {:03X}X\n", opcode, NNN);
    }
    PC = NNN;
}

void Chip8::opcode_2NNN(uint16_t opcode) {
    uint16_t NNN = opcode & 0x0FFF;

    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Call subroutine at {:03X}X\n", opcode, NNN);
    }
    if (SP >= STACK_SIZE) {
        std::cerr << "ERROR: Attempted stack overflow.\n";
        running_flag = false;
    } else {
        stack[SP++] = PC;
        PC = NNN;
    }
}


void Chip8::opcode_3XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint16_t NN = opcode & 0x00FF;
    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Skip next instruction if V{:01X} ({:02X}) == {:02X}",
                                 opcode, X, V[X], NN);
    }
    if (V[X] == NN) {
        PC += 2;
    }
}

void Chip8::opcode_4XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint16_t NN = opcode & 0x00FF;
    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Skip next instruction if V{:01X} ({:02X}) != {:02X}\n",
                                 opcode, X, V[X], NN);
    }
    if (V[X] != NN) {
        PC += 2;
    }
}

void Chip8::opcode_5XY0(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Skip next instruction if V{:01X} ({:02X}) = V{:01X} ({:02X})\n",
                                 opcode, X, V[X], Y, V[Y]);
    }
    if (V[X] == V[Y]) {
        PC += 2;
    }
}


void Chip8::opcode_6XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint16_t NN = opcode & 0x00FF;
    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Set V{:01X} = {:02X}\n", opcode, X, NN);
    }
    V[X] = NN;
}

void Chip8::opcode_7XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint16_t NN = opcode & 0x00FF;

    V[X] += NN;

    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Add {:02X} to V{:01X}. V{:01X} is now set to {:02X}\n",
                                 opcode, NN, X, X, V[X]);
    }
}

void Chip8::opcode_8XY_(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    uint8_t opt = opcode & 0x000F;
    std::string debug_str;
    bool flag = false;
    switch (opt) {
        case 0x0: //Set VX = VY
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} = V{:01X}\n", opcode, X, Y);
            V[X] = V[Y];
            break;
        case 0x1: //Set VX = VX | VY
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} |= V{:01X}\n", opcode, X, Y);
            V[X] |= V[Y];
            break;
        case 0x2: //Set VX = VX & VY
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} &= V{:01X}\n", opcode, X, Y);
            V[X] &= V[Y];
            break;
        case 0x3: //Set VX = VX ^ VY
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} ^= V{:01X}\n", opcode, X, Y);
            V[X] ^= V[Y];
            break;
        case 0x4: //Set VX = VX + VY and set VF = 1 if overflow
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} += V{:01X}\n", opcode, X, Y);
            flag = (V[X] + V[Y]) > 255;
            V[X] += V[Y];
            V[0xF] = flag;
            break;
        case 0x5: //Set VX = VX - VY and set VF = 1 if VX > VY
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} -= V{:01X}\n", opcode, X, Y);
            flag = V[X] > V[Y];
            V[X] -= V[Y];
            V[0xF] = flag;
            break;
        case 0x6: //Set VX = VY, shift VX 1 bit right and set VF = the shifted out bit
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} = V{:01X} >> 1,\n", opcode, X, Y);
            V[X] = V[Y];
            flag = V[X] & 1;
            V[X] >>= 1;
            V[0xF] = flag;
            break;
        case 0x7: //Set VX = VY - VX and set VF = 1 if VY > VX
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} |= V{:01X}\n", opcode, X, Y);
            flag = V[Y] > V[X];
            V[X] = V[Y] - V[X];
            V[0xF] = flag;
            break;
        case 0xE: //Set VX = VY, shift VX 1 bit left and set VF = the shifted out bit
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} = V{:01X} << 1\n", opcode, X, Y);
            V[X] = V[Y];
            flag = (V[X] & 10000000) >> 7;
            V[X] <<= 1;
            V[0xF] = flag;
            break;
        default:
            if (exit_on_unknown) {
                running_flag = false;
            }
            std::cerr << std::format("ERROR: Unknown opcode: {:04X}\n", opcode);
    }
    if (debug && !debug_str.empty()) {
        std::cout << debug_str;
    }
}

void Chip8::opcode_9XY0(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t Y = (opcode & 0x00F0) >> 4;
    if (debug) {
        std::cout
                << std::format("DEBUG: Called {:04X}: Skip next instruction if V{:01X} ({:02X}) != V{:01X} ({:02X})\n",
                               opcode, X, V[X], Y, V[Y]);
    }
    if (V[X] != V[Y]) {
        PC += 2;
    }
}

void Chip8::opcode_ANNN(uint16_t opcode) {
    uint16_t NNN = opcode & 0x0FFF;
    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Set I = {:03X}X\n", opcode, NNN);
    }
    I = NNN;
}

void Chip8::opcode_DXYN(uint16_t opcode) {
    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Draw\n", opcode);
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


void Chip8::opcode_FX29(uint16_t opcode) {

}


void Chip8::load_instructions() {
    instruction_funcs[0x0] = &Chip8::opcode_00E_;
    instruction_funcs[0x1] = &Chip8::opcode_1NNN;
    instruction_funcs[0x2] = &Chip8::opcode_2NNN;
    instruction_funcs[0x3] = &Chip8::opcode_3XNN;
    instruction_funcs[0x4] = &Chip8::opcode_4XNN;
    instruction_funcs[0x5] = &Chip8::opcode_5XY0;
    instruction_funcs[0x6] = &Chip8::opcode_6XNN;
    instruction_funcs[0x7] = &Chip8::opcode_7XNN;
    instruction_funcs[0x8] = &Chip8::opcode_8XY_;
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
        if (e.type == SDL_EVENT_QUIT) {
            running_flag = false;
            break;
        } else if (e.type == SDL_EVENT_KEY_DOWN) {
            for (int i = 0; i < KEY_COUNT; i++) {
                if (e.key.scancode == KEYMAP[i]) {
                    keyboard[i] = true;
                }
            }
        } else if (e.type == SDL_EVENT_KEY_UP) {
            if (e.key.scancode == EXIT_BUTTON) {
                running_flag = false;
                break;
            }

            if (e.key.scancode == PAUSE_BUTTON) {
                stepping = !stepping;
                if (stepping) {
                    execute_next = false;
                }
            }

            if (e.key.scancode == STEP_BUTTON) {
                execute_next = true;
            }


            for (int i = 0; i < KEY_COUNT; i++) {
                if (e.key.scancode == KEYMAP[i]) {
                    keyboard[i] = false;
                }
            }
        }
    }
}