# CHIP-8 Interpreter 

An interpreter for the [CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) programming language, utilizing SDL3 for audio, input and graphics. Currently, only support for the original COSMAC VIP CHIP-8 is planned, but other variants may be added in the future after the base is done.

# Usage

Make using cmake and run with `.\CHIP8 [options] rom.ch8`. Valid options are:
- `-d` enables debug output
- `-i` <ipf> sets the instruction count per frame to <ipf>. Default value is 11. 
- `-ignore` if set, unknown instructions will be ignored. Otherwise, unknown instructions will cause the interpreter to quit.
- `-inc-i-on-index` if set, I will be incremented when performing FX55 or FX65. Otherwise, a temporary indexing variable will be used, and I will be unchanged.   

When running, press escape to exit. Pressing space will pause execution, and pressing the right arrow key will then allow for running one instruction at a time.z 

# Resources Used
- [High-level guide to making a CHIP-8 Emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/) - Gives an explaination of the memory layout and other expected hardware specifications. 
- [Timendus' test ROM](https://github.com/Timendus/chip8-test-suite) - Includes tests for every opcode and platform-specific quirks
- [EmuDev Discord](https://discord.gg/dkmJAes) - The `#chip8` channel contains many resources, including a link to the COSMAC VIP manual which goes in depth into each opcode. 
- [ROM list](https://github.com/kripod/chip8-roms/) - Contains a list of CHIP-8 / SuperChip / MegaChip8 ROMs

