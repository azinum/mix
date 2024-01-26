:: install_deps.bat

@echo off

:: update the package manager
pacman -Syu

:: install the base developer toolchain (make, gcc, g++, gdb, e.t.c.)
pacman -S --needed base-devel mingw-w64-x86_64-toolchain

:: optional cmake for installing raylib
pacman -S mingw-w64-x86_64-cmake

:: glfw3
pacman -S mingw-w64-x86_64-glfw

:: portaudio for audio playback
pacman -S mingw-w64-x86_64-portaudio
