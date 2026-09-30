#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

// ============================================================================
// COSMIC EXPLORATION MISSION CONTROL: CHIP-8 & SUPER-CHIP INSTRUCTION DISASSEMBLER
// Mission Control Status: Stellar
// Astronavigation Flight Code Telemetry Decoder
// ============================================================================

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>

inline std::string disassemble_chip8(uint16_t opcode) {
    std::ostringstream ss;
    uint8_t op_high = (opcode & 0xF000) >> 12;
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t n = opcode & 0x000F;
    uint8_t nn = opcode & 0x00FF;
    uint16_t nnn = opcode & 0x0FFF;

    switch (op_high) {
        case 0x0:
            if ((opcode & 0xFFF0) == 0x00C0) {
                ss << "SCD  " << static_cast<int>(n) << " (Scroll Down " << static_cast<int>(n) << " lines)";
            } else {
                switch (nn) {
                    case 0xE0: ss << "CLS  (Clear Display)"; break;
                    case 0xEE: ss << "RET  (Subroutine Return)"; break;
                    case 0xFB: ss << "SCR  (Scroll Right 4px)"; break;
                    case 0xFC: ss << "SCL  (Scroll Left 4px)"; break;
                    case 0xFD: ss << "EXIT (Halt Flight CPU)"; break;
                    case 0xFE: ss << "LOW  (Standard 64x32 Res)"; break;
                    case 0xFF: ss << "HIGH (Extended 128x64 Res)"; break;
                    default:   ss << "SYS  0x" << std::hex << std::uppercase << nnn; break;
                }
            }
            break;
        case 0x1: ss << "JP   0x" << std::hex << std::uppercase << nnn; break;
        case 0x2: ss << "CALL 0x" << std::hex << std::uppercase << nnn; break;
        case 0x3: ss << "SE   V" << std::hex << std::uppercase << static_cast<int>(x) << ", 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(nn); break;
        case 0x4: ss << "SNE  V" << std::hex << std::uppercase << static_cast<int>(x) << ", 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(nn); break;
        case 0x5: ss << "SE   V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y); break;
        case 0x6: ss << "LD   V" << std::hex << std::uppercase << static_cast<int>(x) << ", 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(nn); break;
        case 0x7: ss << "ADD  V" << std::hex << std::uppercase << static_cast<int>(x) << ", 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(nn); break;
        case 0x8:
            switch (n) {
                case 0x0: ss << "LD   V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y); break;
                case 0x1: ss << "OR   V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y); break;
                case 0x2: ss << "AND  V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y); break;
                case 0x3: ss << "XOR  V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y); break;
                case 0x4: ss << "ADD  V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y) << " (Carry)"; break;
                case 0x5: ss << "SUB  V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y) << " (Borrow)"; break;
                case 0x6: ss << "SHR  V" << std::hex << std::uppercase << static_cast<int>(x); break;
                case 0x7: ss << "SUBN V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y); break;
                case 0xE: ss << "SHL  V" << std::hex << std::uppercase << static_cast<int>(x); break;
                default:  ss << "UNK  0x" << std::hex << std::uppercase << opcode; break;
            }
            break;
        case 0x9: ss << "SNE  V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y); break;
        case 0xA: ss << "LD   I, 0x" << std::hex << std::uppercase << nnn; break;
        case 0xB: ss << "JP   V0, 0x" << std::hex << std::uppercase << nnn; break;
        case 0xC: ss << "RND  V" << std::hex << std::uppercase << static_cast<int>(x) << ", 0x" << std::setw(2) << std::setfill('0') << static_cast<int>(nn); break;
        case 0xD:
            if (n == 0) {
                ss << "DRW  V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y) << ", 16x16 (SCHIP)";
            } else {
                ss << "DRW  V" << std::hex << std::uppercase << static_cast<int>(x) << ", V" << static_cast<int>(y) << ", " << static_cast<int>(n);
            }
            break;
        case 0xE:
            switch (nn) {
                case 0x9E: ss << "SKP  V" << std::hex << std::uppercase << static_cast<int>(x) << " (If Key Pressed)"; break;
                case 0xA1: ss << "SKNP V" << std::hex << std::uppercase << static_cast<int>(x) << " (If Key Not Pressed)"; break;
                default:   ss << "UNK  0x" << std::hex << std::uppercase << opcode; break;
            }
            break;
        case 0xF:
            switch (nn) {
                case 0x07: ss << "LD   V" << std::hex << std::uppercase << static_cast<int>(x) << ", DT"; break;
                case 0x0A: ss << "LD   V" << std::hex << std::uppercase << static_cast<int>(x) << ", K (Wait Key)"; break;
                case 0x15: ss << "LD   DT, V" << std::hex << std::uppercase << static_cast<int>(x); break;
                case 0x18: ss << "LD   ST, V" << std::hex << std::uppercase << static_cast<int>(x); break;
                case 0x1E: ss << "ADD  I, V" << std::hex << std::uppercase << static_cast<int>(x); break;
                case 0x29: ss << "LD   F, V" << std::hex << std::uppercase << static_cast<int>(x) << " (Font 5x8)"; break;
                case 0x30: ss << "LD   HF, V" << std::hex << std::uppercase << static_cast<int>(x) << " (SCHIP Font 10x8)"; break;
                case 0x33: ss << "LD   B, V" << std::hex << std::uppercase << static_cast<int>(x) << " (BCD)"; break;
                case 0x55: ss << "LD   [I], V" << std::hex << std::uppercase << static_cast<int>(x); break;
                case 0x65: ss << "LD   V" << std::hex << std::uppercase << static_cast<int>(x) << ", [I]"; break;
                case 0x75: ss << "LD   R, V" << std::hex << std::uppercase << static_cast<int>(x) << " (Store RPL Flags)"; break;
                case 0x85: ss << "LD   V" << std::hex << std::uppercase << static_cast<int>(x) << ", R (Read RPL Flags)"; break;
                default:   ss << "UNK  0x" << std::hex << std::uppercase << opcode; break;
            }
            break;
        default:
            ss << "DW   0x" << std::hex << std::uppercase << opcode;
            break;
    }
    return ss.str();
}

#endif // DISASSEMBLER_H
