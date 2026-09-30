import subprocess, os, shutil, sys

tmp = os.path.expanduser('~') + '/chip8_build_temp.exe'
dst = 'chip8.exe'

sources = [
    'src/main.cpp',
    'src/chip8.cpp',
    'src/imgui/imgui.cpp',
    'src/imgui/imgui_draw.cpp',
    'src/imgui/imgui_tables.cpp',
    'src/imgui/imgui_widgets.cpp',
    'src/imgui/imgui_impl_sdl2.cpp',
    'src/imgui/imgui_impl_sdlrenderer2.cpp',
]

cmd = [
    'g++', '-std=c++17', '-O2', '-Wall',
    '-Isrc', '-Isrc/imgui', '-IC:/msys64/ucrt64/include/SDL2',
] + sources + [
    '-o', tmp,
    '-lmingw32', '-lSDL2main', '-lSDL2', '-mconsole',
]

print('Compiling Cosmic Chip-8 Flight Workbench...')
print(' '.join(cmd[:6]), '...')
r = subprocess.run(cmd, capture_output=True, text=True)
if r.stderr:
    for line in r.stderr.strip().split(chr(10)):
        if 'error' in line.lower() or 'src/main' in line or 'src/chip8' in line:
            print(line)
if r.returncode != 0:
    print('BUILD FAILED (rc={})'.format(r.returncode))
    sys.exit(1)
shutil.copy2(tmp, dst)
os.remove(tmp)
print('Build OK: {} ({} bytes)'.format(dst, os.path.getsize(dst)))
