# config.mk

NO_SIMD=0
USE_STATIC_MEMORY_ALLOCATOR=1

USE_PORTAUDIO=1
USE_MINIAUDIO=0

CC=clang
LIBS=-lraylib -lm
SRC=src/main.c
INC=include
FLAGS=-I${INC} -Wall -Wextra -ffast-math -O0 -ggdb
TARGET=mix
PREFIX=/usr/local
