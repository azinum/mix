#!/bin/sh

CC="clang"
FLAGS="-O2 -pedantic -Iinclude"
LIBS="-lraylib -lm -lpthread -lglfw -ldl"
set -xe

${CC} src/main.c -o mix ${FLAGS} ${LIBS}
