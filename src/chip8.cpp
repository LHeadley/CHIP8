#include "chip8.h"
#include <fstream>
#include <iostream>
#include <format>
#include <random>


Chip8::Chip8(std::string fname) {
    audio.init_audio();
    running_flag = load_ROM(fname);
    load_instructions();
    memcpy(memory + FONT_START, FONTSET, sizeof(uint8_t) * FONTSET_SIZE);
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


void Chip8::unknown_opcode(uint16_t opcode) {
    if (exit_on_unknown) {
        running_flag = false;
    }
    std::cerr << std::format("ERROR: Unknown opcode: {:04X}\n", opcode);
}


uint16_t Chip8::fetch() {
    if (PC > MEMORY_SIZE) {
        std::cerr << "ERROR: Reached end of instructions\n";
        running_flag = false;
        return 0;
    }

    uint16_t opcode = memory[PC] << 8 | memory[PC + 1];
    PC += 2;
    return opcode;
}


void Chip8::execute_loop() {
    if (running_flag) {
        uint16_t opcode = fetch();
        InstructionFunc func = instruction_funcs[(opcode & 0xF000) >> 12];
        if (!func) {
            unknown_opcode(opcode);
        } else {
            (this->*func)(opcode);
        }

        if (stepping) {
            execute_next = false;
        }
    }
}


void Chip8::opcode_00E_(uint16_t opcode) {
    uint8_t opt = (opcode & 0x00FF);
    if (opt == 0xE0) {
        opcode_00E0(opcode);
    } else if (opt == 0xEE) {
        opcode_00EE(opcode);
    } else {
        unknown_opcode(opcode);
    }
}


void Chip8::opcode_00E0(uint16_t opcode) {
    if (debug) std::cout << std::format("DEBUG: Called {:04X}: Clear display\n", opcode);

    memset(display, 0, sizeof(uint8_t) * LOGICAL_WIDTH * LOGICAL_HEIGHT);
    draw_flag = true;
}


void Chip8::opcode_00EE(uint16_t opcode) {
    if (debug) std::cout << std::format("DEBUG: Called {:04X}: Return from subroutine\n", opcode);

    if (SP == 0) {
        std::cerr << "ERROR: Attempted stack underflow.\n";
        running_flag = false;
    } else {
        PC = stack[--SP];
    }
}


void Chip8::opcode_1NNN(uint16_t opcode) {
    uint16_t NNN = opcode & 0x0FFF;
    if (debug) std::cout << std::format("DEBUG: Called {:04X}: Jump to {:03X}\n", opcode, NNN);

    PC = NNN;
}

void Chip8::opcode_2NNN(uint16_t opcode) {
    uint16_t NNN = opcode & 0x0FFF;

    if (debug) std::cout << std::format("DEBUG: Called {:04X}: Call subroutine at {:03X}X\n", opcode, NNN);

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
    uint8_t NN = opcode & 0x00FF;

    if (debug) {
        std::cout << std::format("DEBUG: Called {:04X}: Skip next instruction if V{:01X} ({:02X}) == {:02X}\n",
                                 opcode, X, V[X], NN);
    }
    if (V[X] == NN) {
        PC += 2;
    }
}

void Chip8::opcode_4XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t NN = opcode & 0x00FF;
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
    if ((opcode & 0x000F) != 0) {
        unknown_opcode(opcode);
        return;
    }

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
    uint8_t NN = opcode & 0x00FF;
    if (debug) std::cout << std::format("DEBUG: Called {:04X}: Set V{:01X} = {:02X}\n", opcode, X, NN);

    V[X] = NN;
}

void Chip8::opcode_7XNN(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t NN = opcode & 0x00FF;

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
            V[0xF] = 0;
            break;
        case 0x2: //Set VX = VX & VY
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} &= V{:01X}\n", opcode, X, Y);
            V[X] &= V[Y];
            V[0xF] = 0;
            break;
        case 0x3: //Set VX = VX ^ VY
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} ^= V{:01X}\n", opcode, X, Y);
            V[X] ^= V[Y];
            V[0xF] = 0;
            break;
        case 0x4: //Set VX = VX + VY and set VF = 1 if overflow
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} += V{:01X}\n", opcode, X, Y);
            flag = (V[X] + V[Y]) > 255;
            V[X] += V[Y];
            V[0xF] = flag;
            break;
        case 0x5: //Set VX = VX - VY and set VF = 1 if VX > VY
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} -= V{:01X}\n", opcode, X, Y);
            flag = V[X] >= V[Y];
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
            debug_str = std::format("DEBUG: Called {:04X}: Set V{:01X} = V{:01X} - V{:01X}\n", opcode, X, Y, X);
            flag = V[Y] >= V[X];
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

    if ((opcode & 0x000F) != 0) {
        unknown_opcode(opcode);
        return;
    }

    if (debug)
        std::cout << std::format(
                "DEBUG: Called {:04X}: Skip next instruction if V{:01X} ({:02X}) != V{:01X} ({:02X})\n",
                opcode, X, V[X], Y, V[Y]);

    if (V[X] != V[Y]) {
        PC += 2;
    }
}

void Chip8::opcode_ANNN(uint16_t opcode) {
    uint16_t NNN = opcode & 0x0FFF;
    if (debug) std::cout << std::format("DEBUG: Called {:04X}: Set I = {:03X}X\n", opcode, NNN);

    I = NNN;
}

void Chip8::opcode_BNNN(uint16_t opcode) {
    uint16_t NNN = opcode & 0x0FFF;
    if (debug) std::cout << std::format("DEBUG: Called {:04X} Jump to {:03X} + V0", opcode, NNN);

    PC = NNN + V[0];
}

void Chip8::opcode_CXNN(uint16_t opcode) {
    static std::random_device rd;
    static std::mt19937 gen{rd()};
    static std::uniform_int_distribution<uint8_t> dis;

    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t NN = opcode & 0x00FF;

    if (debug) std::cout << std::format("DEBUG: Called {:04X} V[{:01X}] = RAND & {:02X}\n", opcode, X, NN);
    V[X] = dis(gen) & NN;
}

void Chip8::opcode_DXYN(uint16_t opcode) {
    if (debug) std::cout << std::format("DEBUG: Called {:04X}: Draw\n", opcode);

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

void Chip8::opcode_EX_(uint16_t opcode) {
    uint8_t opt = opcode & 0x00FF;
    uint8_t X = (opcode & 0x0F00) >> 8;
    switch (opt) {
        case 0x9E:
            opcode_EX9E(X);
            break;
        case 0xA1:
            opcode_EXA1(X);
            break;
        default:
            unknown_opcode(opcode);
            break;
    }
}

void Chip8::opcode_EX9E(uint8_t X) {
    if (debug) std::cout << std::format("DEBUG: Called E{:01X}9E Skip if key in V{:01X} is pressed\n", X, X);

    uint8_t key = V[X];
    if (keyboard[key]) {
        PC += 2;
    }
}

void Chip8::opcode_EXA1(uint8_t X) {
    if (debug) std::cout << std::format("DEBUG: Called E{:01X}9E Skip if key in V{:01X} is not pressed\n", X, X);

    uint8_t key = V[X];
    if (!keyboard[key]) {
        PC += 2;
    }
}


void Chip8::opcode_FX_(uint16_t opcode) {
    uint8_t X = (opcode & 0x0F00) >> 8;
    uint8_t opt = (opcode & 0x00FF);

    switch (opt) {
        case 0x07:
            opcode_FX07(X);
            break;
        case 0x0A:
            opcode_FX0A(X);
            break;
        case 0x15:
            opcode_FX15(X);
            break;
        case 0x18:
            opcode_FX18(X);
            break;
        case 0x1E:
            opcode_FX1E(X);
            break;
        case 0x29:
            opcode_FX29(X);
            break;
        case 0x33:
            opcode_FX33(X);
            break;
        case 0x55:
            opcode_FX55(X);
            break;
        case 0x65:
            opcode_FX65(X);
            break;
        default:
            unknown_opcode(opcode);
    }

}

void Chip8::opcode_FX07(uint8_t X) {
    if (debug) std::cout << std::format("DEBUG: Called F{:01X}07: Set V{:01X} = delay\n", X, X);
    V[X] = delay;
}

void Chip8::opcode_FX0A(uint8_t X) {
    if (debug) std::cout << std::format("DEBUG: Called F{:01X}0A: Wait for key press\n", X, X);

    for (int i = 0; i < KEY_COUNT; ++i) {
        if (!keyboard[i] && prev_keyboard[i]) {
            V[X] = i;
            return;
        }
    }

    PC -= 2;
}

void Chip8::opcode_FX15(uint8_t X) {
    if (debug) std::cout << std::format("DEBUG: Called F{:01X}15: Set delay = V{:01X}\n", X, X);
    delay = V[X];
}

void Chip8::opcode_FX18(uint8_t X) {
    if (debug) std::cout << std::format("DEBUG: Called F{:01X}18: Set sound = V{:01X}\n", X, X);
    sound = V[X];
}

void Chip8::opcode_FX1E(uint8_t X) {
    if (debug) std::cout << std::format("DEBUG: Called F{:01X}1E: I += V{:01X}\n", X, X);
    I += V[X];
}

void Chip8::opcode_FX29(uint8_t X) {
    if (debug)
        std::cout << std::format("DEBUG: Called F{:01X}29: Set I = address of font character in V{:01X}\n",
                                 X, X);
    I = FONT_START + ((V[X] & 0x0F)  * 5);
}


void Chip8::opcode_FX33(uint8_t X) {
    if (debug) std::cout << std::format("DEBUG: Called F{:01X}33: Compute BCD of V{:01X}\n", X, X);

    uint8_t val = V[X];
    for (int i = 2; i >= 0; --i) {
        memory[I + i] = val % 10;
        val /= 10;
    }
}

void Chip8::opcode_FX55(uint8_t X) {
    if (debug) std::cout << std::format("DEBUG: Called F{:01X}55: Load registers V0 to V{:01X} into memory[I]\n", X, X);
    memcpy(&memory[I], V, (X + 1) * sizeof(uint8_t));
    if (increment_I_on_index) I += X + 1;
}

void Chip8::opcode_FX65(uint8_t X) {
    if (debug) {
        std::cout << std::format("DEBUG: Called F{:01X}55: Load memory[I] into registers V[0] to V{:01X} \n",
                                 X, X);
    }

    memcpy(V, &memory[I], (X + 1) * sizeof(uint8_t));
    if (increment_I_on_index) I += X + 1;
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
    instruction_funcs[0xB] = &Chip8::opcode_BNNN;
    instruction_funcs[0xC] = &Chip8::opcode_CXNN;
    instruction_funcs[0xD] = &Chip8::opcode_DXYN;
    instruction_funcs[0xE] = &Chip8::opcode_EX_;
    instruction_funcs[0xF] = &Chip8::opcode_FX_;
}

void Chip8::draw(Screen &screen) {
    if (draw_flag) {
        screen.draw(display);
        draw_flag = false;
    }
}

void Chip8::update_inputs() {
    SDL_Event e;
    memcpy(prev_keyboard, keyboard, sizeof(bool) * KEY_COUNT);
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

void Chip8::decrement_timers() {
    if (delay > 0) delay--;
    audio.is_beeping = sound > 0;
    if (sound > 0) {
        sound--;
    }
}
