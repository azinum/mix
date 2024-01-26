:: build.bat

@echo off

set BUILD_RAYLIB=1

if %BUILD_RAYLIB% == 1 (
	make -C deps\raylib\src RAYLIB_MODULE_AUDIO=FALSE RAYLIB_MODULE_MODELS=FALSE
)

set CC=gcc
set LIBS=-lopengl32 -lraylib -lgdi32 -lwinmm -lportaudio
set FLAGS=-Iinclude -L.\deps\raylib\src -I.\deps\raylib\src -DUSE_PORTAUDIO

@echo on

%CC% src\main.c -o mix.exe %LIBS% %FLAGS%
