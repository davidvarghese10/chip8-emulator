# Cosmic Chip-8 & Super-CHIP Flight Workbench

> **Mission Control Status: Stellar**

A full-featured CHIP-8 and Super-CHIP (SCHIP) emulator with Dear ImGui debug workbench, instruction disassembler, sound waveform synthesis, ROM browser, and configurable display palettes.

## Features

### Core Emulation
- Full CHIP-8 instruction set (35 opcodes)
- Super-CHIP (SCHIP) extended opcodes: scroll, hi-res 128x64, 16x16 sprites, RPL flags, EXIT/HALT
- Configurable emulation speed (F1/F2 keys, 1-200 cycles per frame)
- Savestate save/load (F5/F6) with backward-compatible versioned format

### ImGui Debug Workbench
- **Optical Sensor Viewport** - Live display with palette preview, hexpad state indicator
- **Flight Disassembler & Debugger** - Real-time disassembly listing with breakpoints, step/pause/resume
- **Astronavigation Registers & Stack** - V0-VF, PC, I, SP, timers, return stack
- **Acoustic Beacon Synthesizer** - 5 waveform types (square, sine, triangle, sawtooth, noise), frequency/volume control
- **ROM Cartridge Archive** - File browser scanning roms/ directory, custom path loading
- **Spacecraft Memory Hex Viewer** - 4KB hex dump with PC highlighting, jump-to-address

### Display & Audio
- 5 configurable color palettes (F3/Tab to cycle): Classic Green, Amber CRT, Neon, Monochrome, Cosmic Blue
- Waveform synthesizer with real-time parameter adjustment
- Test beacon button for audio preview

## Controls

| Key | Action |
|-----|--------|
| 1-4, Q-R, A-F, Z-V | CHIP-8 hexpad (mapped 0-F) |
| Space | Pause / Resume emulation |
| F1 / - | Decrease speed |
| F2 / = | Increase speed |
| F3 / Tab | Cycle color palette |
| F5 | Save state |
| F6 | Load state |
| F9 | Telemetry beacon |
| F10 | Step single opcode (pauses first) |
| Esc | Exit |

## Building

### Prerequisites
- g++ with C++17 support
- SDL2 development libraries
- MSYS2/UCRT64 recommended on Windows

### Quick Build
`ash
python build.py
`

### Manual Build
`ash
g++ -std=c++17 -O2 -Wall -Isrc -Isrc/imgui -IC:/msys64/ucrt64/include/SDL2 \
    src/main.cpp src/chip8.cpp \
    src/imgui/imgui.cpp src/imgui/imgui_draw.cpp src/imgui/imgui_tables.cpp \
    src/imgui/imgui_widgets.cpp src/imgui/imgui_impl_sdl2.cpp \
    src/imgui/imgui_impl_sdlrenderer2.cpp \
    -o chip8.exe -lmingw32 -lSDL2main -lSDL2 -mconsole
`

### Running
`ash
./chip8.exe path/to/rom.ch8
`
Or launch without arguments and use the ROM Browser to load files.

## Super-CHIP Opcodes

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 00Cn | SCD n | Scroll display down n lines |
| 00FB | SCR | Scroll display right 4 pixels |
| 00FC | SCL | Scroll display left 4 pixels |
| 00FD | EXIT | Halt the interpreter |
| 00FE | LOW | Disable hi-res mode (64x32) |
| 00FF | HIGH | Enable hi-res mode (128x64) |
| Dxy0 | DRW16 | Draw 16x16 sprite |
| Fx30 | FONT16 | Point I at large font for digit Vx |
| Fx75 | SRPL | Store V0-Vx in RPL flags |
| Fx85 | LRPL | Load V0-Vx from RPL flags |

## Project Structure

`
src/
  chip8.h          - Emulator class header (CHIP-8 + SCHIP)
  chip8.cpp        - Emulator implementation
  main.cpp         - SDL2 + ImGui workbench application
  disassembler.h   - Instruction disassembler (inline header)
  imgui/           - Dear ImGui v1.91.8 library + SDL2 backends
build.py           - Python build helper script
Makefile           - GNU Make build file
`

## Acknowledgements

Built with [SDL2](https://libsdl.org/) and [Dear ImGui](https://github.com/ocornut/imgui).
