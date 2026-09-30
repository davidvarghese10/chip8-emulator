// ============================================================================
// COSMIC EXPLORATION MISSION CONTROL: CHIP-8 & SUPER-CHIP FLIGHT WORKBENCH
// Mission Control Status: Stellar
// Integrated Disassembler, ImGui Overlay, Waveform Synth, ROM Browser, SCHIP
// ============================================================================
#define SDL_MAIN_HANDLED
#include "chip8.h"
#include "disassembler.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
namespace fs = std::filesystem;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
// === Palette & Color Structures ===
struct ColorRGB { uint8_t r, g, b; };
struct DisplayPalette { const char* name; ColorRGB bg; ColorRGB fg; };
static const DisplayPalette PALETTES[] = {
    {"Classic Green Screen", {10,28,10},  {50,255,50}},
    {"Amber CRT",            {32,16,0},   {255,176,0}},
    {"Neon High-Contrast",   {16,0,36},   {0,255,255}},
    {"Classic Monochrome",   {0,0,0},     {255,255,255}},
    {"Stellar Cosmic Blue",  {8,14,28},   {100,210,255}}
};
static const int PALETTE_COUNT = 5;

// === Waveform Synthesizer ===
enum WaveformType { WAVE_SQUARE=0, WAVE_SINE, WAVE_TRIANGLE, WAVE_SAWTOOTH, WAVE_NOISE };
struct AudioSettings {
    bool beeping = false;
    WaveformType wave_type = WAVE_SQUARE;
    float frequency = 440.0f;
    float volume = 0.35f;
    bool muted = false;
};
void audio_callback(void* userdata, uint8_t* stream, int len) {
    AudioSettings* a = reinterpret_cast<AudioSettings*>(userdata);
    int16_t* buf = reinterpret_cast<int16_t*>(stream);
    int samples = len / 2;
    static double phase = 0.0;
    static uint32_t lfsr = 0xACE1u;
    if (!a->beeping || a->muted || a->volume <= 0.0f) {
        std::memset(stream, 0, len); phase = 0.0; return;
    }
    double inc = a->frequency / 44100.0;
    int16_t amp = static_cast<int16_t>(30000.0f * a->volume);
    for (int i = 0; i < samples; i++) {
        int16_t s = 0;
        switch (a->wave_type) {
            case WAVE_SQUARE:   s = (phase < 0.5) ? amp : -amp; break;
            case WAVE_SINE:     s = static_cast<int16_t>(amp * std::sin(phase * 2.0 * M_PI)); break;
            case WAVE_TRIANGLE: s = (phase < 0.5) ? static_cast<int16_t>(-amp + 4.0*amp*phase)
                                                   : static_cast<int16_t>(amp - 4.0*amp*(phase-0.5)); break;
            case WAVE_SAWTOOTH: s = static_cast<int16_t>(-amp + 2.0*amp*phase); break;
            case WAVE_NOISE:
                lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u);
                s = static_cast<int16_t>((lfsr % (uint32_t)(2*amp+1)) - amp); break;
        }
        buf[i] = s;
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
    }
}
// === Keymap & ImGui Theme ===
static const int KEYMAP[16] = {
    SDLK_x, SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e, SDLK_a,
    SDLK_s, SDLK_d, SDLK_z, SDLK_c,
    SDLK_4, SDLK_r, SDLK_f, SDLK_v
};
void apply_space_imgui_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;
    style.WindowRounding = 6.0f; style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;  style.GrabRounding = 3.0f;
    c[ImGuiCol_Text]             = ImVec4(0.92f,0.95f,0.98f,1.0f);
    c[ImGuiCol_WindowBg]         = ImVec4(0.08f,0.10f,0.14f,0.96f);
    c[ImGuiCol_ChildBg]          = ImVec4(0.06f,0.08f,0.11f,1.0f);
    c[ImGuiCol_PopupBg]          = ImVec4(0.10f,0.13f,0.18f,0.98f);
    c[ImGuiCol_Border]           = ImVec4(0.20f,0.28f,0.38f,0.65f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.12f,0.16f,0.22f,1.0f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.18f,0.25f,0.35f,1.0f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.24f,0.34f,0.46f,1.0f);
    c[ImGuiCol_TitleBg]          = ImVec4(0.07f,0.09f,0.13f,1.0f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.10f,0.18f,0.28f,1.0f);
    c[ImGuiCol_CheckMark]        = ImVec4(0.20f,0.85f,0.85f,1.0f);
    c[ImGuiCol_SliderGrab]       = ImVec4(0.18f,0.70f,0.80f,1.0f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.25f,0.90f,0.95f,1.0f);
    c[ImGuiCol_Button]           = ImVec4(0.14f,0.22f,0.32f,1.0f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.20f,0.32f,0.46f,1.0f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.25f,0.42f,0.60f,1.0f);
    c[ImGuiCol_Header]           = ImVec4(0.14f,0.24f,0.34f,1.0f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.20f,0.34f,0.48f,1.0f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.26f,0.42f,0.58f,1.0f);
    c[ImGuiCol_Tab]              = ImVec4(0.10f,0.14f,0.20f,1.0f);
    c[ImGuiCol_TabHovered]       = ImVec4(0.20f,0.30f,0.44f,1.0f);
    c[ImGuiCol_TabActive]        = ImVec4(0.16f,0.26f,0.38f,1.0f);
    c[ImGuiCol_ScrollbarBg]      = ImVec4(0.05f,0.07f,0.09f,0.6f);
    c[ImGuiCol_ScrollbarGrab]    = ImVec4(0.20f,0.28f,0.38f,1.0f);
    c[ImGuiCol_MenuBarBg]        = ImVec4(0.09f,0.12f,0.16f,1.0f);
}
std::vector<std::string> scan_roms() {
    std::vector<std::string> list;
    for (const char* d : {"roms", "."}) {
        std::error_code ec;
        if (fs::exists(d, ec) && fs::is_directory(d, ec)) {
            for (const auto& e : fs::directory_iterator(d, ec)) {
                if (!e.is_regular_file()) continue;
                auto ext = e.path().extension().string();
                for (auto& ch : ext) ch = (char)std::tolower((unsigned char)ch);
                if (ext == ".ch8" || ext == ".sc8" || ext == ".bin" || ext == ".rom")
                    list.push_back(e.path().string());
            }
        }
    }
    return list;
}
int main(int argc, char** argv) {
    std::cout << "====================================================\n"
              << "  COSMIC CHIP-8 & SCHIP FLIGHT CONTROL WORKBENCH   \n"
              << "  Mission Control Status: Stellar                   \n"
              << "====================================================\n";
    cosmo_polo_telemetry();
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        std::cerr << "[Error] SDL Init failed: " << SDL_GetError() << "\n"; return 1;
    }
    // Audio setup
    AudioSettings audio;
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100; want.format = AUDIO_S16SYS;
    want.channels = 1; want.samples = 2048;
    want.callback = audio_callback; want.userdata = &audio;
    SDL_AudioDeviceID audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_dev) SDL_PauseAudioDevice(audio_dev, 0);
    // Window
    SDL_Window* window = SDL_CreateWindow(
        "Cosmic Chip-8 Flight Workbench | Mission Control Status: Stellar",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 768,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) { std::cerr << "[Error] Window: " << SDL_GetError() << "\n"; SDL_Quit(); return 1; }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) { std::cerr << "[Error] Renderer: " << SDL_GetError() << "\n"; SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Nav keyboard disabled: emulator keys (Tab, Space, etc) must pass through
    apply_space_imgui_theme();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);
    // Display texture: 128x64 streaming (covers both standard 64x32 & SCHIP 128x64)
    SDL_Texture* chip8_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, 128, 64);
    SDL_SetTextureScaleMode(chip8_tex, SDL_ScaleModeNearest);
    uint32_t tex_buf[128*64];
    std::memset(tex_buf, 0, sizeof(tex_buf));
    // Chip8 instance
    Chip8 chip8;
    std::string cur_rom = (argc >= 2) ? argv[1] : "";
    if (!cur_rom.empty()) chip8.load_rom(cur_rom);
    std::string sav_path = cur_rom.empty() ? "cosmo_default.sav" : (cur_rom + ".sav");
    // State
    bool paused = false;
    int cpf = 12;
    int pal_idx = 3;
    std::string status_msg = "Flight Computer Online";
    std::set<uint16_t> breakpoints;
    uint16_t bp_input = 0x200;
    bool step_once = false;
    bool show_viewport = true, show_disasm = true, show_regs = true;
    bool show_audio = true, show_roms = true, show_mem = false;
    std::vector<std::string> rom_list = scan_roms();
    char custom_rom[256] = "";
    int mem_offset = 0x200;
    bool running = true;
    uint32_t last_tick = SDL_GetTicks();
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) { running = false; }
            else if (ev.type == SDL_KEYDOWN) {
                auto sym = ev.key.keysym.sym;
                // Emulator hotkeys: only when ImGui is not capturing keyboard
                // Emulator hotkeys: always processed (F-keys, Space, Esc, Tab)
                    if (sym == SDLK_ESCAPE)          running = false;
                    else if (sym == SDLK_SPACE)      { paused = !paused; status_msg = paused ? "Paused" : "Running"; }
                    else if (sym == SDLK_F10)        { paused = true; step_once = true; }
                    else if (sym == SDLK_F1 || sym == SDLK_MINUS)  { if (cpf > 1)   { cpf--; status_msg = "Speed: " + std::to_string(cpf) + " CPF"; } }
                    else if (sym == SDLK_F2 || sym == SDLK_EQUALS) { if (cpf < 200) { cpf++; status_msg = "Speed: " + std::to_string(cpf) + " CPF"; } }
                    else if (sym == SDLK_F3 || sym == SDLK_TAB)    { pal_idx = (pal_idx + 1) % PALETTE_COUNT; status_msg = std::string("Palette: ") + PALETTES[pal_idx].name; chip8.draw_flag = true; }
                    else if (sym == SDLK_F5)  { chip8.save_state(sav_path); status_msg = "State Saved"; }
                    else if (sym == SDLK_F6)  { chip8.load_state(sav_path); chip8.draw_flag = true; status_msg = "State Restored"; }
                    else if (sym == SDLK_F9)  { chip8.cosmo_polo_telemetry(); status_msg = "Telemetry Transmitted"; }
                // Hexpad keys: always pass through UNLESS typing in a text field
                if (!io.WantTextInput) {
                    for (int i = 0; i < 16; i++) if (sym == KEYMAP[i]) chip8.key[i] = 1;
                }
            } else if (ev.type == SDL_KEYUP) {
                auto sym = ev.key.keysym.sym;
                for (int i = 0; i < 16; i++) if (sym == KEYMAP[i]) chip8.key[i] = 0;
            }
        }
        // Emulation: run cycles
        if (!paused && !chip8.is_halted()) {
            for (int i = 0; i < cpf; i++) {
                if (breakpoints.count(chip8.get_pc())) {
                    paused = true;
                    std::ostringstream ss; ss << "Breakpoint @ 0x" << std::hex << std::uppercase << chip8.get_pc();
                    status_msg = ss.str(); break;
                }
                chip8.emulate_cycle();
            }
            chip8.update_timers();
        } else if (step_once) {
            chip8.emulate_cycle(); chip8.update_timers(); step_once = false;
        }
        audio.beeping = (chip8.get_sound_timer() > 0);
        // Build display texture
        const DisplayPalette& pal = PALETTES[pal_idx];
        uint32_t bg = (0xFFu<<24)|(pal.bg.r<<16)|(pal.bg.g<<8)|pal.bg.b;
        uint32_t fg = (0xFFu<<24)|(pal.fg.r<<16)|(pal.fg.g<<8)|pal.fg.b;
        if (chip8.hires) {
            for (int y=0; y<64; y++) for (int x=0; x<128; x++)
                tex_buf[x + y*128] = chip8.display[x + y*128] ? fg : bg;
        } else {
            for (int y=0; y<32; y++) for (int x=0; x<64; x++) {
                uint32_t col = chip8.display[x + y*64] ? fg : bg;
                tex_buf[(x*2)   + (y*2)*128]   = col;
                tex_buf[(x*2+1) + (y*2)*128]   = col;
                tex_buf[(x*2)   + (y*2+1)*128] = col;
                tex_buf[(x*2+1) + (y*2+1)*128] = col;
            }
        }
        SDL_UpdateTexture(chip8_tex, NULL, tex_buf, 128*sizeof(uint32_t));
        // Begin ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        // Main menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Mission")) {
                if (ImGui::MenuItem("Rescan ROMs")) { rom_list = scan_roms(); status_msg = "ROMs Rescanned"; }
                if (ImGui::MenuItem("Reset CPU (Ctrl+R)")) { chip8.reset(); if (!cur_rom.empty()) chip8.load_rom(cur_rom); status_msg = "CPU Reset"; }
                ImGui::Separator();
                if (ImGui::MenuItem("Save State",  "F5")) { chip8.save_state(sav_path); status_msg = "State Saved"; }
                if (ImGui::MenuItem("Load State",  "F6")) { chip8.load_state(sav_path); chip8.draw_flag = true; status_msg = "State Loaded"; }
                ImGui::Separator();
                if (ImGui::MenuItem("Telemetry Beacon", "F9")) { chip8.cosmo_polo_telemetry(); }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Esc")) running = false;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Emulation")) {
                if (ImGui::MenuItem("Pause/Resume", "Space", paused)) { paused = !paused; }
                if (ImGui::MenuItem("Step Opcode",  "F10"))  { paused = true; step_once = true; }
                ImGui::Separator();
                if (ImGui::MenuItem("Low  (~100 Hz)"))  cpf = 2;
                if (ImGui::MenuItem("Normal (~600Hz)")) cpf = 10;
                if (ImGui::MenuItem("Fast (~1200Hz)"))  cpf = 20;
                if (ImGui::MenuItem("Warp (~3000Hz)"))  cpf = 50;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Palettes")) {
                for (int i=0; i<PALETTE_COUNT; i++)
                    if (ImGui::MenuItem(PALETTES[i].name, NULL, pal_idx==i)) { pal_idx=i; chip8.draw_flag=true; }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Windows")) {
                ImGui::MenuItem("Optical Viewport",   NULL, &show_viewport);
                ImGui::MenuItem("Disassembler",       NULL, &show_disasm);
                ImGui::MenuItem("Registers & Stack",  NULL, &show_regs);
                ImGui::MenuItem("Audio Synthesizer",  NULL, &show_audio);
                ImGui::MenuItem("ROM Browser",        NULL, &show_roms);
                ImGui::MenuItem("Memory Viewer",      NULL, &show_mem);
                ImGui::EndMenu();
            }
            std::string hud = std::string("Status: Stellar | ") + (paused?"[PAUSED] ":"") + status_msg;
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(hud.c_str()).x - 20.f);
            ImGui::TextColored(ImVec4(0.2f,0.9f,0.9f,1.f), "%s", hud.c_str());
            ImGui::EndMainMenuBar();
        }
        // 1. OPTICAL VIEWPORT
        if (show_viewport) {
            ImGui::SetNextWindowSize(ImVec2(560, 360), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(16, 40), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Optical Sensor Viewport", &show_viewport)) {
                ImGui::Text("ROM: %s | %s | Palette: %s",
                    cur_rom.empty() ? "<None>" : cur_rom.c_str(),
                    chip8.hires ? "SCHIP 128x64" : "Chip-8 64x32",
                    PALETTES[pal_idx].name);
                ImVec2 avail = ImGui::GetContentRegionAvail();
                float w = avail.x, h = w * 0.5f;
                if (h > avail.y && avail.y > 32.f) { h = avail.y; w = h * 2.f; }
                ImGui::Image(reinterpret_cast<ImTextureID>(chip8_tex), ImVec2(w, h));
                ImGui::Separator();
                // Virtual hexpad visual indicator
                ImGui::Text("Hexpad State:");
                for (int i=0; i<16; i++) {
                    if (i && i%8) ImGui::SameLine();
                    if (chip8.key[i])
                        ImGui::TextColored(ImVec4(0.2f,1.f,0.2f,1.f), "[%X]", i);
                    else
                        ImGui::TextDisabled("[%X]", i);
                }
            }
            ImGui::End();
        }
        // 2. DISASSEMBLER & DEBUGGER
        if (show_disasm) {
            ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(596, 40), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Flight Disassembler & Debugger", &show_disasm)) {
                if (paused) { if (ImGui::Button("Resume (Space)")) { paused=false; status_msg="Running"; } }
                else        { if (ImGui::Button("Pause  (Space)")) { paused=true;  status_msg="Paused"; } }
                ImGui::SameLine();
                if (ImGui::Button("Step F10")) { paused=true; step_once=true; }
                ImGui::SameLine();
                if (ImGui::Button("Reset"))    { chip8.reset(); if (!cur_rom.empty()) chip8.load_rom(cur_rom); status_msg="Reset"; }
                ImGui::SameLine();
                ImGui::Text("PC: 0x%03X  SP: %d  Mode: %s", chip8.get_pc(), chip8.get_sp(), chip8.hires ? "SCHIP" : "Chip-8");
                ImGui::Separator();
                ImGui::Text("Breakpoints:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::InputScalar("##bp", ImGuiDataType_U16, &bp_input, NULL, NULL, "%03X");
                ImGui::SameLine();
                if (ImGui::Button("Toggle BP")) {
                    if (breakpoints.count(bp_input)) breakpoints.erase(bp_input);
                    else breakpoints.insert(bp_input);
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear BPs")) breakpoints.clear();
                ImGui::Text("Active breakpoints: %zu", breakpoints.size());
                ImGui::Separator();
                // Disassembly listing
                if (ImGui::BeginChild("Disasm", ImVec2(0,0), true)) {
                    uint16_t pc = chip8.get_pc();
                    const uint8_t* mem = chip8.get_memory();
                    int start = (int)pc - 20; if (start < 0x200) start = 0x200;
                    int end   = start + 60;   if (end > 4094)   end = 4094;
                    for (int addr = start; addr <= end; addr += 2) {
                        uint16_t op = ((uint16_t)mem[addr] << 8) | mem[addr+1];
                        bool is_cur = (addr == pc);
                        bool has_bp = breakpoints.count((uint16_t)addr) > 0;
                        bool tmp_bp = has_bp;
                        ImGui::PushID(addr);
                        if (ImGui::Checkbox("##cb", &tmp_bp)) {
                            if (tmp_bp) breakpoints.insert((uint16_t)addr);
                            else        breakpoints.erase((uint16_t)addr);
                        }
                        ImGui::PopID();
                        ImGui::SameLine();
                        std::string mn = disassemble_chip8(op);
                        if (is_cur)
                            ImGui::TextColored(ImVec4(0.2f,0.95f,0.95f,1.f), ">>> 0x%03X  %04X  %s", addr, op, mn.c_str());
                        else if (has_bp)
                            ImGui::TextColored(ImVec4(1.f,0.35f,0.35f,1.f), " BP 0x%03X  %04X  %s", addr, op, mn.c_str());
                        else
                            ImGui::Text("    0x%03X  %04X  %s", addr, op, mn.c_str());
                        if (is_cur) ImGui::SetScrollHereY(0.4f);
                    }
                }
                ImGui::EndChild();
            }
            ImGui::End();
        }
        // 3. TELEMETRY REGISTERS & STACK
        if (show_regs) {
            ImGui::SetNextWindowSize(ImVec2(560, 310), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(16, 418), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Astronavigation Registers & Stack", &show_regs)) {
                ImGui::Text("V0-VF Telemetry Registers:");
                if (ImGui::BeginTable("Regs", 8, ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg)) {
                    for (int i=0; i<8; i++) {
                        ImGui::TableNextColumn();
                        ImGui::Text("V%X=%02X", i, chip8.get_v(i));
                    }
                    for (int i=8; i<16; i++) {
                        ImGui::TableNextColumn();
                        if (i==0xF) ImGui::TextColored(ImVec4(0.9f,0.7f,0.2f,1.f), "VF=%02X", chip8.get_v(i));
                        else        ImGui::Text("V%X=%02X", i, chip8.get_v(i));
                    }
                    ImGui::EndTable();
                }
                ImGui::Separator();
                ImGui::Columns(4, "Nav", false);
                ImGui::Text("PC"); ImGui::TextColored(ImVec4(0.3f,0.9f,0.9f,1.f), "0x%03X", chip8.get_pc()); ImGui::NextColumn();
                ImGui::Text("I");  ImGui::TextColored(ImVec4(0.3f,0.9f,0.9f,1.f), "0x%03X", chip8.get_index()); ImGui::NextColumn();
                ImGui::Text("SP"); ImGui::TextColored(ImVec4(0.3f,0.9f,0.9f,1.f), "%d/16", chip8.get_sp()); ImGui::NextColumn();
                ImGui::Text("Timers"); ImGui::Text("DT:%d ST:%d", chip8.get_delay_timer(), chip8.get_sound_timer());
                ImGui::Columns(1);
                ImGui::Separator();
                ImGui::Text("Return Stack (depth %d):", chip8.get_sp());
                if (chip8.get_sp() == 0) { ImGui::TextDisabled("Stack empty"); }
                else { for (int s=0; s<chip8.get_sp(); s++) ImGui::Text("  [%02d] 0x%03X", s, chip8.get_stack(s)); }
            }
            ImGui::End();
        }
        // 4. AUDIO SYNTHESIZER
        if (show_audio) {
            ImGui::SetNextWindowSize(ImVec2(380, 220), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(16, 735), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Acoustic Beacon Synthesizer", &show_audio)) {
                const char* waves[] = {"Square (Classic)","Sine (Pure)","Triangle (Warm)","Sawtooth (Aggressive)","Cosmic Noise"};
                int wt = (int)audio.wave_type;
                if (ImGui::Combo("Waveform", &wt, waves, 5)) audio.wave_type = (WaveformType)wt;
                ImGui::SliderFloat("Frequency (Hz)", &audio.frequency, 100.f, 2000.f, "%.0f Hz");
                ImGui::SliderFloat("Volume",         &audio.volume,    0.f,   1.f,    "%.2f");
                ImGui::Checkbox("Mute", &audio.muted);
                ImGui::Separator();
                bool active = chip8.get_sound_timer() > 0;
                ImGui::TextColored(active ? ImVec4(0.2f,1.f,0.2f,1.f) : ImVec4(0.5f,0.5f,0.5f,1.f),
                    "Beacon: %s", active ? "TRANSMITTING" : "Silent");
                ImGui::SameLine();
                if (ImGui::Button("Test Beacon (0.3s)")) chip8.set_sound_timer(18);
            }
            ImGui::End();
        }
        // 5. ROM BROWSER
        if (show_roms) {
            ImGui::SetNextWindowSize(ImVec2(400, 280), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(876, 735), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("ROM Cartridge Archive", &show_roms)) {
                if (ImGui::Button("Rescan")) { rom_list = scan_roms(); }
                ImGui::SameLine(); ImGui::Text("%zu ROMs found", rom_list.size());
                ImGui::Separator();
                if (ImGui::BeginChild("RomList", ImVec2(0, 140), true)) {
                    for (const auto& rom : rom_list) {
                        bool sel = (rom == cur_rom);
                        if (ImGui::Selectable(rom.c_str(), sel)) {
                            cur_rom = rom; sav_path = rom + ".sav";
                            chip8.load_rom(cur_rom); status_msg = "Loaded: " + rom;
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::InputText("Path", custom_rom, 256);
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    if (custom_rom[0]) {
                        cur_rom = custom_rom; sav_path = cur_rom + ".sav";
                        chip8.load_rom(cur_rom); status_msg = "Loaded custom ROM";
                    }
                }
            }
            ImGui::End();
        }
        // 6. MEMORY VIEWER
        if (show_mem) {
            ImGui::SetNextWindowSize(ImVec2(620, 340), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Spacecraft Memory (4KB Hex View)", &show_mem)) {
                ImGui::SetNextItemWidth(100);
                ImGui::InputScalar("Offset", ImGuiDataType_S32, &mem_offset, NULL, NULL, "0x%03X");
                if (mem_offset < 0) mem_offset = 0;
                if (mem_offset > 4096-256) mem_offset = 4096-256;
                ImGui::SameLine(); if (ImGui::Button("ROM 0x200")) mem_offset = 0x200;
                ImGui::SameLine(); if (ImGui::Button("I Register")) mem_offset = chip8.get_index();
                ImGui::Separator();
                if (ImGui::BeginChild("HexDump", ImVec2(0,0), true)) {
                    const uint8_t* mem = chip8.get_memory();
                    for (int r=0; r<24; r++) {
                        int addr = mem_offset + r*16;
                        if (addr+16 > 4096) break;
                        std::ostringstream line;
                        line << std::hex << std::uppercase << std::setw(3) << std::setfill('0') << addr << ":  ";
                        for (int b=0; b<16; b++) line << std::setw(2) << std::setfill('0') << (int)mem[addr+b] << " ";
                        line << " ";
                        for (int b=0; b<16; b++) { uint8_t c=mem[addr+b]; line << ((c>=32&&c<=126)?(char)c:'.'); }
                        bool is_pc_row = ((int)chip8.get_pc() >= addr && (int)chip8.get_pc() < addr+16);
                        if (is_pc_row) ImGui::TextColored(ImVec4(0.2f,0.95f,0.95f,1.f), "%s", line.str().c_str());
                        else           ImGui::TextUnformatted(line.str().c_str());
                    }
                }
                ImGui::EndChild();
            }
            ImGui::End();
        }
        // RENDER
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 20, 24, 32, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
        uint32_t ft = SDL_GetTicks() - last_tick;
        if (ft < 16) SDL_Delay(16 - ft);
        last_tick = SDL_GetTicks();
    }
    // Shutdown sequence
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    if (chip8_tex)   SDL_DestroyTexture(chip8_tex);
    if (audio_dev)   SDL_CloseAudioDevice(audio_dev);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "[Mission Control] Flight computer shut down. Mission Complete.\n";
    return 0;
}