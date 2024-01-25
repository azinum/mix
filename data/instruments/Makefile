# Makefile

CC=cc
FLAGS=-shared -ggdb -Wall -O2
LIBS=-lmix
TARGETS=${subst .c,.so, ${shell find . -type f -name '*.c'}}

all: ${TARGETS}

%.so: %.c
	${CC} $< -o $@ ${FLAGS} ${LIBS}

clean:
	rm ${TARGETS}

.PHONY: clean
