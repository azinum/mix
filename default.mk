# config.mk

NO_SIMD=0
USE_STATIC_MEMORY_ALLOCATOR=1

USE_PORTAUDIO=1
USE_MINIAUDIO=0
INLINE_RAYLIB=0

CC=gcc
LIBS=-lm
SRC=src/main.c
INC=include
FLAGS=-I${INC} -Wall -Wextra -march=native -ggdb -O0
TARGET=mix
PREFIX=/usr/local
