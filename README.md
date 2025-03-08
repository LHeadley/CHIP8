# CHIP-8 Interpreter 

An interpreter for the [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) programming language, utilizing SDL2 for audio, input and graphics. Currently, only support for the original COSMAC VIP CHIP-8 is planned, but other variants may be added in the future after the base is done.

# Usage

Make using cmake and run with `.\CHIP8 [options] rom.ch8`. Valid options are:
- `-d` enables debug output
- `-i` <ipf> sets the instruction count per frame to <ipf>. Default value is 11. 

When running, press escape to exit. Pressing space will pause execution, and pressing the right arrow key will then allow for running one instruction at a time.z 

# Resources Used
- [High-level guide to making a CHIP-8 Emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/) - Gives an explaination of the memory layout and other expected hardware specifications. 
- [Timendus' test ROM](https://github.com/Timendus/chip8-test-suite) - Includes tests for every opcode and platform-specific quirks
- [EmuDev Discord](https://discord.gg/dkmJAes) - The `#chip8` channel contains many resources, including a link to the COSMAC VIP manual which goes in depth into each opcode. 
- [ROM list](https://github.com/kripod/chip8-roms/) - Contains a list of CHIP-8 / SuperChip / MegaChip8 ROMs

