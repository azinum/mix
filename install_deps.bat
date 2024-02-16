:: install_deps.bat
:: download package manager from https://www.msys2.org/

@echo off

:: update the package manager
pacman -Syu

:: install the base developer toolchain (make, gcc, g++, gdb, e.t.c.)
pacman -S --needed base-devel mingw-w64-x86_64-toolchain

:: optional cmake for installing raylib
pacman -S mingw-w64-x86_64-cmake

:: glfw3
pacman -S mingw-w64-x86_64-glfw

:: portaudio for audio playback (optional, miniaudio can be used if portaudio is unwanted)
pacman -S mingw-w64-x86_64-portaudio
