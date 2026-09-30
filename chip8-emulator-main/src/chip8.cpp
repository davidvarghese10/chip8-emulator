// ============================================================================
// COSMIC EXPLORATION MISSION CONTROL: CHIP-8 & SUPER-CHIP FLIGHT COMPUTER
// Mission Control Status: Stellar
// Trajectory Calculation, Super-CHIP Opcode Execution & Telemetry Systems
// ============================================================================

#include "chip8.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cstring>
#include <random>

// Hexadecimal astral font glyph matrix: 80 bytes in memory banks 0x000 - 0x04F
static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0 - Celestial Orbit Zero
    0x20, 0x60, 0x20, 0x20, 0x70, // 1 - Rocket Probe One
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2 - Binary Star Two
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3 - Apollo Three
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4 - Quadrant Four
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5 - Stellar Sector Five
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6 - Sextant Six
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7 - Deep-Space Array Seven
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8 - Infinity Octave Eight
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9 - Nebula Nine
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A - Alpha Centauri
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B - Beta Pictoris
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C - Cygnus
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D - Delta Orionis
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E - Epsilon Eridani
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F - Flight Vector F
};

// Super-CHIP 10-byte High-Resolution Font Glyphs (Digits 0-9): 100 bytes at 0x050 - 0x0B3
static const uint8_t schip_fontset[100] = {
    0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C, // 0
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // 1
    0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, // 2
    0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, // 3
    0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, // 4
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, // 5
    0x3E, 0x7C, 0xC0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, // 6
    0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, // 7
    0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, // 8
    0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C  // 9
};

Chip8::Chip8() {
    // Ignite flight systems and calibrate navigation registers
    initialise();
}

void Chip8::reset() {
    initialise();
}

void Chip8::initialise() {
    // Reset trajectory program counter to standard mission entry vector 0x200
    pc = 0x200;
    opcode = 0;
    index = 0;
    sp = 0;
    halted = false;
    hires = false; // Start in standard 64x32 mode

    // Purge optical visual sensor array and flight memory banks
    std::memset(display, 0, sizeof(display));
    std::memset(stack, 0, sizeof(stack));
    std::memset(v, 0, sizeof(v));
    std::memset(memory, 0, sizeof(memory));
    std::memset(key, 0, sizeof(key));
    std::memset(rpl_flags, 0, sizeof(rpl_flags));

    // Uplink celestial glyphs into protected base memory
    load_fonts();

    // Synchronize mission timers
    delay_timer = 0;
    sound_timer = 0;
    draw_flag = false;
}

void Chip8::load_fonts() {
    // Transfer standard 5-byte astral glyphs (0x000 - 0x04F)
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }
    // Transfer Super-CHIP 10-byte high-resolution font glyphs (0x050 - 0x0B3)
    for (int i = 0; i < 100; i++) {
        memory[0x050 + i] = schip_fontset[i];
    }
}

void Chip8::load_rom(const std::string& filename) {
    // Establish telemetry downlink with game payload binary
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "[Mission Control Error] Failed to open payload cartridge: " << filename << std::endl;
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Verify payload fits within available spacecraft memory banks
    if (size > (4096 - 512)) {
        std::cerr << "[Mission Control Error] Payload cartridge exceeds orbital memory capacity!" << std::endl;
        file.close();
        return;
    }

    // Reset state before loading new payload
    initialise();

    // Load binary payload into program execution space starting at 0x200
    file.read(reinterpret_cast<char*>(memory + 512), size);
    file.close();

    std::cout << "[Mission Control] Payload successfully inserted into orbital memory: " << filename << std::endl;
}

void Chip8::emulate_cycle() {
    if (halted) {
        return;
    }

    // Trajectory safety check: ensure PC remains in orbital boundary
    if (pc >= 4095) {
        std::cerr << "[Mission Warning] Trajectory PC out of orbital bounds: 0x" << std::hex << pc << std::dec << std::endl;
        return;
    }

    // Fetch 16-bit guidance instruction from memory
    opcode = (memory[pc] << 8) | memory[pc + 1];

    // Decode high nibble of telemetry instruction
    switch (opcode & 0xF000) {
        case 0x0000:
            // Super-CHIP Scroll Down: 00CN
            if ((opcode & 0xFFF0) == 0x00C0) {
                int lines = opcode & 0x000F;
                int w = get_display_width();
                int h = get_display_height();
                for (int y = h - 1; y >= lines; y--) {
                    std::memcpy(&display[y * w], &display[(y - lines) * w], w);
                }
                std::memset(display, 0, lines * w);
                draw_flag = true;
                pc += 2;
                break;
            }

            switch (opcode & 0x00FF) {
                case 0x00E0: // 00E0: Clear optical display matrix
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;

                case 0x00EE: // 00EE: Return from astronavigation subroutine
                    if (sp > 0) {
                        sp--;
                        pc = stack[sp];
                    } else {
                        std::cerr << "[Mission Warning] Trajectory stack underflow encountered!" << std::endl;
                    }
                    pc += 2;
                    break;

                case 0x00FB: { // 00FB: Super-CHIP Scroll Right 4 Pixels
                    int w = get_display_width();
                    int h = get_display_height();
                    for (int y = 0; y < h; y++) {
                        for (int x = w - 1; x >= 4; x--) {
                            display[x + y * w] = display[(x - 4) + y * w];
                        }
                        for (int x = 0; x < 4; x++) {
                            display[x + y * w] = 0;
                        }
                    }
                    draw_flag = true;
                    pc += 2;
                    break;
                }

                case 0x00FC: { // 00FC: Super-CHIP Scroll Left 4 Pixels
                    int w = get_display_width();
                    int h = get_display_height();
                    for (int y = 0; y < h; y++) {
                        for (int x = 0; x < w - 4; x++) {
                            display[x + y * w] = display[(x + 4) + y * w];
                        }
                        for (int x = w - 4; x < w; x++) {
                            display[x + y * w] = 0;
                        }
                    }
                    draw_flag = true;
                    pc += 2;
                    break;
                }

                case 0x00FD: // 00FD: Super-CHIP EXIT - Halt flight CPU
                    halted = true;
                    std::cout << "[Mission Control] Flight CPU halted by SCHIP EXIT telecommand." << std::endl;
                    pc += 2;
                    break;

                case 0x00FE: // 00FE: Super-CHIP LOW - Set 64x32 Low-Resolution Mode
                    hires = false;
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;

                case 0x00FF: // 00FF: Super-CHIP HIGH - Set 128x64 High-Resolution Mode
                    hires = true;
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;

                default:
                    // 0NNN: Legacy orbital machine call - safe advancement
                    pc += 2;
                    break;
            }
            break;

        case 0x1000: // 1NNN: Orbital Jump to trajectory address NNN
            pc = opcode & 0x0FFF;
            break;

        case 0x2000: // 2NNN: Subroutine Orbital Insertion at address NNN
            if (sp < 16) {
                stack[sp] = pc;
                sp++;
                pc = opcode & 0x0FFF;
            } else {
                std::cerr << "[Mission Warning] Trajectory stack overflow abort!" << std::endl;
                pc += 2;
            }
            break;

        case 0x3000: { // 3XNN: Skip next telemetry vector if V[X] == NN
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (v[x] == nn) pc += 4;
            else pc += 2;
            break;
        }

        case 0x4000: { // 4XNN: Skip next telemetry vector if V[X] != NN
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (v[x] != nn) pc += 4;
            else pc += 2;
            break;
        }

        case 0x5000: { // 5XY0: Skip next telemetry vector if V[X] == V[Y]
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if (v[x] == v[y]) pc += 4;
            else pc += 2;
            break;
        }

        case 0x6000: { // 6XNN: Set telemetry register V[X] = NN
            uint8_t x = (opcode & 0x0F00) >> 8;
            v[x] = opcode & 0x00FF;
            pc += 2;
            break;
        }

        case 0x7000: { // 7XNN: Add delta NN to telemetry register V[X]
            uint8_t x = (opcode & 0x0F00) >> 8;
            v[x] += (opcode & 0x00FF);
            pc += 2;
            break;
        }

        case 0x8000: { // Arithmetic & Logic Propulsion Computer
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            switch (opcode & 0x000F) {
                case 0x0000: // 8XY0: V[X] = V[Y]
                    v[x] = v[y];
                    pc += 2;
                    break;

                case 0x0001: // 8XY1: V[X] = V[X] OR V[Y]
                    v[x] |= v[y];
                    pc += 2;
                    break;

                case 0x0002: // 8XY2: V[X] = V[X] AND V[Y]
                    v[x] &= v[y];
                    pc += 2;
                    break;

                case 0x0003: // 8XY3: V[X] = V[X] XOR V[Y]
                    v[x] ^= v[y];
                    pc += 2;
                    break;

                case 0x0004: { // 8XY4: V[X] += V[Y], V[F] = Orbital Carry Flag
                    uint16_t sum = v[x] + v[y];
                    uint8_t carry = (sum > 0xFF) ? 1 : 0;
                    v[x] = sum & 0xFF;
                    v[0xF] = carry;
                    pc += 2;
                    break;
                }

                case 0x0005: { // 8XY5: V[X] -= V[Y], V[F] = NOT Borrow Flag (1 if V[X] >= V[Y])
                    uint8_t not_borrow = (v[x] >= v[y]) ? 1 : 0;
                    v[x] = v[x] - v[y];
                    v[0xF] = not_borrow;
                    pc += 2;
                    break;
                }

                case 0x0006: { // 8XY6: V[X] >>= 1, V[F] = Bit Shift LSB
                    uint8_t lsb = v[x] & 0x1;
                    v[x] >>= 1;
                    v[0xF] = lsb;
                    pc += 2;
                    break;
                }

                case 0x0007: { // 8XY7: V[X] = V[Y] - V[X], V[F] = NOT Borrow Flag (1 if V[Y] >= V[X])
                    uint8_t not_borrow = (v[y] >= v[x]) ? 1 : 0;
                    v[x] = v[y] - v[x];
                    v[0xF] = not_borrow;
                    pc += 2;
                    break;
                }

                case 0x000E: { // 8XYE: V[X] <<= 1, V[F] = Bit Shift MSB
                    uint8_t msb = (v[x] >> 7) & 0x1;
                    v[x] <<= 1;
                    v[0xF] = msb;
                    pc += 2;
                    break;
                }

                default:
                    std::cerr << "[Mission Warning] Unknown 8-series propulsion opcode: 0x" << std::hex << opcode << std::dec << std::endl;
                    pc += 2;
                    break;
            }
            break;
        }

        case 0x9000: { // 9XY0: Skip next telemetry vector if V[X] != V[Y]
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if (v[x] != v[y]) pc += 4;
            else pc += 2;
            break;
        }

        case 0xA000: // ANNN: Set orbital memory index pointer to NNN
            index = opcode & 0x0FFF;
            pc += 2;
            break;

        case 0xB000: // BNNN: Jump to address NNN + V[0]
            pc = (opcode & 0x0FFF) + v[0];
            break;

        case 0xC000: { // CXNN: Generate random cosmic telemetry byte AND NN
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dist(0, 255);
            uint8_t x = (opcode & 0x0F00) >> 8;
            v[x] = dist(gen) & (opcode & 0x00FF);
            pc += 2;
            break;
        }

        case 0xD000: { // DXYN: Render celestial sprite at coordinate (V[X], V[Y])
            int w = get_display_width();
            int h = get_display_height();
            uint8_t x = v[(opcode & 0x0F00) >> 8] % w;
            uint8_t y = v[(opcode & 0x00F0) >> 4] % h;
            uint8_t height = opcode & 0x000F;

            v[0xF] = 0; // Reset collision sensor

            if (height == 0) {
                // Super-CHIP 16x16 Sprite Drawing
                for (int y_line = 0; y_line < 16; y_line++) {
                    int screen_y = y + y_line;
                    if (screen_y >= h) break;

                    uint8_t byte0 = memory[(index + y_line * 2) & 0x0FFF];
                    uint8_t byte1 = memory[(index + y_line * 2 + 1) & 0x0FFF];
                    uint16_t row_pixels = (byte0 << 8) | byte1;

                    for (int x_line = 0; x_line < 16; x_line++) {
                        if ((row_pixels & (0x8000 >> x_line)) != 0) {
                            int screen_x = x + x_line;
                            if (screen_x >= w) continue;

                            int screen_index = screen_x + (screen_y * w);
                            if (display[screen_index] == 1) {
                                v[0xF] = 1; // Collision detected
                            }
                            display[screen_index] ^= 1;
                        }
                    }
                }
            } else {
                // Standard 8xN Sprite Drawing
                for (int y_line = 0; y_line < height; y_line++) {
                    int screen_y = y + y_line;
                    if (screen_y >= h) break;

                    uint8_t pixel = memory[(index + y_line) & 0x0FFF];

                    for (int x_line = 0; x_line < 8; x_line++) {
                        if ((pixel & (0x80 >> x_line)) != 0) {
                            int screen_x = x + x_line;
                            if (screen_x >= w) continue;

                            int screen_index = screen_x + (screen_y * w);
                            if (display[screen_index] == 1) {
                                v[0xF] = 1;
                            }
                            display[screen_index] ^= 1;
                        }
                    }
                }
            }
            draw_flag = true;
            pc += 2;
            break;
        }

        case 0xE000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t key_index = v[x] & 0x0F;

            switch (opcode & 0x00FF) {
                case 0x009E: // EX9E: Skip next instruction if key in V[X] is pressed
                    if (key[key_index] != 0) pc += 4;
                    else pc += 2;
                    break;

                case 0x00A1: // EXA1: Skip next instruction if key in V[X] is not pressed
                    if (key[key_index] == 0) pc += 4;
                    else pc += 2;
                    break;

                default:
                    std::cerr << "[Mission Warning] Unknown flight control key opcode: 0x" << std::hex << opcode << std::dec << std::endl;
                    pc += 2;
                    break;
            }
            break;
        }

        case 0xF000: {
            uint8_t x = (opcode & 0x0F00) >> 8;

            switch (opcode & 0x00FF) {
                case 0x0007: // FX07: V[X] = mission delay timer
                    v[x] = delay_timer;
                    pc += 2;
                    break;

                case 0x000A: { // FX0A: Await flight console key signal; store in V[X]
                    bool key_pressed = false;
                    for (int i = 0; i < 16; i++) {
                        if (key[i] != 0) {
                            v[x] = i;
                            key_pressed = true;
                            break;
                        }
                    }
                    if (!key_pressed) {
                        return; // Remain in holding orbit: stall PC until telemetry signal is received
                    }
                    pc += 2;
                    break;
                }

                case 0x0015: // FX15: Set mission delay timer = V[X]
                    delay_timer = v[x];
                    pc += 2;
                    break;

                case 0x0018: // FX18: Set acoustic beacon alarm timer = V[X]
                    sound_timer = v[x];
                    pc += 2;
                    break;

                case 0x001E: // FX1E: Index pointer += V[X]
                    index = (index + v[x]) & 0x0FFF;
                    pc += 2;
                    break;

                case 0x0029: // FX29: Point index to astral font glyph for digit V[X] (5-byte)
                    index = (v[x] & 0x0F) * 5;
                    pc += 2;
                    break;

                case 0x0030: // FX30: Super-CHIP Point index to 10-byte font glyph for digit V[X]
                    index = 0x050 + (v[x] & 0x0F) * 10;
                    pc += 2;
                    break;

                case 0x0033: { // FX33: Store Binary Coded Decimal of V[X] into memory at Index
                    uint8_t value = v[x];
                    memory[index & 0x0FFF] = value / 100;
                    memory[(index + 1) & 0x0FFF] = (value / 10) % 10;
                    memory[(index + 2) & 0x0FFF] = value % 10;
                    pc += 2;
                    break;
                }

                case 0x0055: { // FX55: Dump registers V[0] through V[X] inclusive into orbital memory
                    for (int i = 0; i <= x; i++) {
                        memory[(index + i) & 0x0FFF] = v[i];
                    }
                    pc += 2;
                    break;
                }

                case 0x0065: { // FX65: Restore registers V[0] through V[X] inclusive from orbital memory
                    for (int i = 0; i <= x; i++) {
                        v[i] = memory[(index + i) & 0x0FFF];
                    }
                    pc += 2;
                    break;
                }

                case 0x0075: { // FX75: Super-CHIP Store V[0] through V[X] in persistent RPL flags
                    int limit = (x > 15) ? 15 : x;
                    for (int i = 0; i <= limit; i++) {
                        rpl_flags[i] = v[i];
                    }
                    pc += 2;
                    break;
                }

                case 0x0085: { // FX85: Super-CHIP Restore V[0] through V[X] from persistent RPL flags
                    int limit = (x > 15) ? 15 : x;
                    for (int i = 0; i <= limit; i++) {
                        v[i] = rpl_flags[i];
                    }
                    pc += 2;
                    break;
                }

                default:
                    std::cerr << "[Mission Warning] Unknown telemetry utility opcode: 0x" << std::hex << opcode << std::dec << std::endl;
                    pc += 2;
                    break;
            }
            break;
        }

        default:
            std::cerr << "[Mission Warning] Unrecognized flight instruction: 0x" << std::hex << opcode << std::dec << std::endl;
            pc += 2;
            break;
    }
}

void Chip8::update_timers() {
    if (delay_timer > 0) {
        delay_timer--;
    }
    if (sound_timer > 0) {
        sound_timer--;
    }
}

// Savestate Management: Orbital Snapshot Downlink
bool Chip8::save_state(const std::string& filename) const {
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[Mission Control Error] Unable to open orbital snapshot downlink file: " << filename << std::endl;
        return false;
    }

    const char magic[16] = "COSMO_CHIP8_SAV";
    uint32_t version = 2; // Version 2 includes Super-CHIP high-res and RPL flags

    out.write(magic, 16);
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(memory), sizeof(memory));
    out.write(reinterpret_cast<const char*>(v), sizeof(v));
    out.write(reinterpret_cast<const char*>(&index), sizeof(index));
    out.write(reinterpret_cast<const char*>(&pc), sizeof(pc));
    out.write(reinterpret_cast<const char*>(stack), sizeof(stack));
    out.write(reinterpret_cast<const char*>(&sp), sizeof(sp));
    out.write(reinterpret_cast<const char*>(&delay_timer), sizeof(delay_timer));
    out.write(reinterpret_cast<const char*>(&sound_timer), sizeof(sound_timer));
    out.write(reinterpret_cast<const char*>(&opcode), sizeof(opcode));
    out.write(reinterpret_cast<const char*>(display), sizeof(display));
    
    uint8_t df = draw_flag ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&df), sizeof(df));
    uint8_t hr = hires ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&hr), sizeof(hr));
    uint8_t ht = halted ? 1 : 0;
    out.write(reinterpret_cast<const char*>(&ht), sizeof(ht));
    out.write(reinterpret_cast<const char*>(rpl_flags), sizeof(rpl_flags));

    out.close();
    std::cout << "[Mission Control] Orbital snapshot stored successfully to " << filename << std::endl;
    return true;
}

// Savestate Management: Orbital Snapshot Uplink Recovery
bool Chip8::load_state(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[Mission Control Error] Unable to open orbital snapshot uplink file: " << filename << std::endl;
        return false;
    }

    char magic[16];
    uint32_t version = 0;

    in.read(magic, 16);
    if (std::strncmp(magic, "COSMO_CHIP8_SAV", 15) != 0) {
        std::cerr << "[Mission Control Error] Invalid orbital snapshot signature!" << std::endl;
        in.close();
        return false;
    }

    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version != 1 && version != 2) {
        std::cerr << "[Mission Control Error] Unsupported orbital snapshot format version: " << version << std::endl;
        in.close();
        return false;
    }

    in.read(reinterpret_cast<char*>(memory), sizeof(memory));
    in.read(reinterpret_cast<char*>(v), sizeof(v));
    in.read(reinterpret_cast<char*>(&index), sizeof(index));
    in.read(reinterpret_cast<char*>(&pc), sizeof(pc));
    in.read(reinterpret_cast<char*>(stack), sizeof(stack));
    in.read(reinterpret_cast<char*>(&sp), sizeof(sp));
    in.read(reinterpret_cast<char*>(&delay_timer), sizeof(delay_timer));
    in.read(reinterpret_cast<char*>(&sound_timer), sizeof(sound_timer));
    in.read(reinterpret_cast<char*>(&opcode), sizeof(opcode));

    if (version == 1) {
        // Version 1 had 64x32 display
        uint8_t v1_disp[64 * 32];
        in.read(reinterpret_cast<char*>(v1_disp), sizeof(v1_disp));
        std::memset(display, 0, sizeof(display));
        std::memcpy(display, v1_disp, sizeof(v1_disp));
        hires = false;
        halted = false;
        std::memset(rpl_flags, 0, sizeof(rpl_flags));
        uint8_t df = 0;
        in.read(reinterpret_cast<char*>(&df), sizeof(df));
    } else if (version == 2) {
        in.read(reinterpret_cast<char*>(display), sizeof(display));
        uint8_t df = 0, hr = 0, ht = 0;
        in.read(reinterpret_cast<char*>(&df), sizeof(df));
        in.read(reinterpret_cast<char*>(&hr), sizeof(hr));
        in.read(reinterpret_cast<char*>(&ht), sizeof(ht));
        in.read(reinterpret_cast<char*>(rpl_flags), sizeof(rpl_flags));
        hires = (hr != 0);
        halted = (ht != 0);
    }

    draw_flag = true;
    in.close();
    std::cout << "[Mission Control] Orbital snapshot restored from " << filename << std::endl;
    return true;
}

// Telemetry Subsystem: Diagnostic readout
void Chip8::cosmo_polo_telemetry() const {
    std::cout << "============================================================" << std::endl;
    std::cout << "[Telemetry Beacon: Cosmo-Polo Alpha-1]" << std::endl;
    std::cout << "Mission Control Status: Stellar" << std::endl;
    std::cout << "Telemetry Trajectory PC: 0x" << std::hex << pc << " | Index Register: 0x" << index << std::dec << std::endl;
    std::cout << "Astronavigation Stack Pointer: " << static_cast<int>(sp) << " | Opcode: 0x" << std::hex << opcode << std::dec << std::endl;
    std::cout << "Optical Resolution: " << (hires ? "Super-CHIP 128x64" : "Standard 64x32") << std::endl;
    std::cout << "Mission Timers - Delay: " << static_cast<int>(delay_timer) << " | Sound Beacon: " << static_cast<int>(sound_timer) << std::endl;
    std::cout << "Registers [V0-VF]:";
    for (int i = 0; i < 16; i++) {
        std::cout << " V" << std::hex << i << "=" << static_cast<int>(v[i]);
    }
    std::cout << std::dec << std::endl;
    std::cout << "============================================================" << std::endl;
}

// Standalone global telemetry downlink function
void cosmo_polo_telemetry() {
    std::cout << "[Global Deep-Space Telemetry Relay]" << std::endl;
    std::cout << "Mission Control Status: Stellar" << std::endl;
}
