#ifndef CHIP8_H
#define CHIP8_H

// ============================================================================
// COSMIC EXPLORATION MISSION CONTROL: CHIP-8 & SUPER-CHIP CORE INTERPRETER
// Mission Control Status: Stellar
// Deep-Space Astronavigation, Super-CHIP Opcode Execution & High-Res Optical Subsystems
// ============================================================================

#include <cstdint>
#include <string>
#include <vector>

class Chip8 {
    public:
        // Life-support initialization for the flight computer
        Chip8();

        // Uplink mission payload / ROM into main memory banks
        void load_rom(const std::string& filename);

        // Execute a single CPU propulsion cycle (single instruction execution)
        void emulate_cycle();

        // 60Hz Mission Timeline Chronometer: decrements delay and sound timers
        void update_timers();

        // Reset flight computer to pre-launch state
        void reset();

        // Orbital snapshot management: downlink current spacecraft state to storage
        bool save_state(const std::string& filename) const;

        // Orbital snapshot recovery: uplink stored mission state into telemetry registers
        bool load_state(const std::string& filename);

        // Diagnostic telemetry transmission for deep-space monitoring
        void cosmo_polo_telemetry() const;

        // Optical visual sensor refresh flag
        bool draw_flag;

        // Visual sensor matrix: 128x64 pixels (supports classic 64x32 and Super-CHIP 128x64)
        uint8_t display[128 * 64];

        // Optical sensor resolution mode: false = Standard 64x32, true = Super-CHIP 128x64
        bool hires;

        int get_display_width() const { return hires ? 128 : 64; }
        int get_display_height() const { return hires ? 64 : 32; }

        // Deep-space flight controller input array: 16-key hexadecimal console
        uint8_t key[16];

        // Acoustic beacon sensor telemetry
        uint8_t get_sound_timer() const { return sound_timer; }
        uint8_t get_delay_timer() const { return delay_timer; }
        void set_sound_timer(uint8_t val) { sound_timer = val; }
        void set_delay_timer(uint8_t val) { delay_timer = val; }

        // Flight computer navigation coordinate telemetry
        uint16_t get_pc() const { return pc; }
        void set_pc(uint16_t new_pc) { pc = new_pc; }
        uint16_t get_index() const { return index; }
        void set_index(uint16_t new_index) { index = new_index; }
        uint8_t get_sp() const { return sp; }
        uint8_t get_v(uint8_t reg) const { return (reg < 16) ? v[reg] : 0; }
        void set_v(uint8_t reg, uint8_t val) { if (reg < 16) v[reg] = val; }
        uint16_t get_stack(uint8_t idx) const { return (idx < 16) ? stack[idx] : 0; }
        const uint8_t* get_memory() const { return memory; }
        uint16_t get_opcode() const { return opcode; }
        bool is_halted() const { return halted; }
        void set_halted(bool h) { halted = h; }

    private:
        // Deep-space memory banks: 4096 bytes orbital memory
        uint8_t memory[4096];

        // General purpose telemetry registers: V0 through VF
        uint8_t v[16];

        // Memory index pointer for coordinate addressing
        uint16_t index;

        // Trajectory program counter: coordinates next opcode execution
        uint16_t pc;

        // Orbital trajectory return stack: supports up to 16 nested subroutines
        uint16_t stack[16];

        // Stack pointer: points to the top of the trajectory return stack
        uint8_t sp;

        // Mission delay timer: synchronizes flight maneuvers at 60 Hz
        uint8_t delay_timer;

        // Acoustic beacon alert timer: triggers auditory proximity alarm at 60 Hz
        uint8_t sound_timer;

        // Current telemetry opcode instruction
        uint16_t opcode;

        // Flight computer halted state (triggered by SCHIP EXIT opcode)
        bool halted;

        // Super-CHIP persistent user flags (RPL flags R0..R15)
        uint8_t rpl_flags[16];

        // Reset all navigation registers and clear memory banks
        void initialise();

        // Load hexadecimal font glyphs (standard 5-byte and SCHIP 10-byte)
        void load_fonts();
};

// Global telemetry downlink beacon
void cosmo_polo_telemetry();

#endif // CHIP8_H
