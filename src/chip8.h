#ifndef CHIP_8_CHIP8_H
#define CHIP_8_CHIP8_H

#include <SDL3/SDL.h>
#include <string>
#include "screen.h"

const int KEY_COUNT = 16;
const int REGISTER_COUNT = 16;
const int MEMORY_SIZE = 4096;
const int STACK_SIZE = 16;

const int LOGICAL_WIDTH = 64;
const int LOGICAL_HEIGHT = 32;

const int EXIT_BUTTON = SDL_SCANCODE_ESCAPE;

const int PAUSE_BUTTON = SDL_SCANCODE_SPACE;
const int STEP_BUTTON = SDL_SCANCODE_RIGHT;

const int PROGRAM_START = 0x200;
const int FONT_START = 0x050;

const int FONTSET_SIZE = 80;

constexpr uint8_t FONTSET[FONTSET_SIZE] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

constexpr uint8_t KEYMAP[KEY_COUNT] = {
        SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
        SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
        SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
        SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V
};

class Chip8 {
public:
    explicit Chip8(std::string fname);

    Chip8(std::string _fname, bool _debug) : Chip8(_fname) {
        debug = _debug;
    }

    Chip8(std::string _fname, bool _debug, bool exit) : Chip8(_fname) {
        debug = _debug;
        exit_on_unknown = exit;
    }

    Chip8(std::string _fname, bool _debug, bool exit, bool increment_I) : Chip8(_fname, _debug, exit) {
        debug = _debug;
        exit_on_unknown = exit;
        increment_I_on_index = increment_I;
    }



    void execute_loop();

    void update_inputs();

    void decrement_timers();

    void draw(Screen &screen);

    bool should_execute_next() const { return execute_next; }

    bool isRunning() const { return running_flag; }

    bool isStepping() const { return stepping; }

private:
    bool debug = true;
    bool stepping = false;
    bool execute_next = false;
    bool exit_on_unknown = true;
    bool increment_I_on_index = false;

    uint8_t memory[MEMORY_SIZE] = {0};

    uint8_t display[LOGICAL_WIDTH * LOGICAL_HEIGHT] = {0};
    uint16_t PC = PROGRAM_START;
    uint16_t I = 0;

    uint16_t stack[STACK_SIZE] = {0};
    uint16_t SP = 0;

    uint8_t delay = 0;
    uint8_t sound = 0;

    uint8_t V[REGISTER_COUNT] = {0};

    bool keyboard[KEY_COUNT] = {false};
    bool prev_keyboard[KEY_COUNT] = {false};

    bool draw_flag = false;

    bool running_flag;

    using InstructionFunc = void (Chip8::*)(uint16_t);
    InstructionFunc instruction_funcs[16] = {nullptr};

    bool load_ROM(const std::string &fname);

    void load_instructions();

    uint16_t fetch();

    void unknown_opcode(uint16_t opcode);

    //00E_ Either 00E0 or 00EE
    void opcode_00E_(uint16_t opcode);

    //00E0 Clear Screen
    void opcode_00E0(uint16_t opcode);

    //00EE Return from subroutine
    void opcode_00EE(uint16_t opcode);

    //1NNN Jump to NNN
    void opcode_1NNN(uint16_t opcode);

    //2NNN Call subroutine at NNN
    void opcode_2NNN(uint16_t opcode);

    //3XNN Skip next instruction if VX = NN
    void opcode_3XNN(uint16_t opcode);

    //4XNN Skip next instruction if VX != NN
    void opcode_4XNN(uint16_t opcode);

    //5XY0 Skip next instruction if VX = VY
    void opcode_5XY0(uint16_t opcode);

    //6XNN Let VX = NN
    void opcode_6XNN(uint16_t opcode);

    //7XNN Add NN to VX
    void opcode_7XNN(uint16_t opcode);

    //8XY_ Logic and Arithmetic Instructions
    void opcode_8XY_(uint16_t opcode);

    //9XY0 Skip next instruction if VX != VY
    void opcode_9XY0(uint16_t opcode);

    //ANNN Let I = NNN
    void opcode_ANNN(uint16_t opcode);

    //BNNN Jump tp NNN + V0
    void opcode_BNNN(uint16_t opcode);

    //CXNN Random
    void opcode_CXNN(uint16_t opcode);

    //DXYN Draw
    void opcode_DXYN(uint16_t opcode);

    void opcode_FX_(uint16_t opcode);

    //FX07 Let VX = delay timer
    void opcode_FX07(uint8_t X);

    //FX0A Wait for key input and put key in VX
    void opcode_FX0A(uint8_t X);

    //FX15 Set delay timer = VX
    void opcode_FX15(uint8_t X);

    //FX18 Set sound timer = VX
    void opcode_FX18(uint8_t X);

    //FX1E Add VX to I
    void opcode_FX1E(uint8_t X);

    //FX29 Set I = address of character in VX
    void opcode_FX29(uint8_t X);

    //FX33 Convert VX to BCD and store starting at memory[I]
    void opcode_FX33(uint8_t X);

    //FX55 Store memory
    void opcode_FX55(uint8_t X);

    //FX65 Load memory
    void opcode_FX65(uint8_t X);
};


#endif //CHIP_8_CHIP8_H
