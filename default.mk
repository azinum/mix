# config.mk

NO_SIMD=0
USE_STATIC_MEMORY_ALLOCATOR=1

USE_PORTAUDIO=1
USE_MINIAUDIO=0

PROG=mix

CC=cc
LIBS=-lm
SRC=src/main.c
INC=include
FLAGS=-I${INC} -Wall -Wextra -march=native -ffast-math
FLAGS_DEBUG=-ggdb -O0
FLAGS_RELEASE=-DNO_ASSERT -O2 
TARGET=mix
PREFIX=/usr/local
