#!/usr/bin/env bash

NPROC=`nproc`
CACHELINESIZE=`getconf LEVEL1_DCACHE_LINESIZE`

CC="clang"
INC=include
FLAGS="-O0 -ggdb -ffast-math -I${INC} -Wall -Wextra -DNPROC=${NPROC} -DCACHELINESIZE=${CACHELINESIZE}"
LIBS="-lraylib -lm -lpthread -lglfw -ldl"
PREFIX=/usr/local
PROG=mix

set -xe

function build_default() {
	# ${CC} src/main.c -o ${PROG}_miniaudio ${FLAGS} ${LIBS} -DUSE_MINIAUDIO -DMEMORY_ALLOC_STATIC
	${CC} src/main.c -o ${PROG} ${FLAGS} ${LIBS} -lportaudio -DUSE_PORTAUDIO -DMEMORY_ALLOC_STATIC
}

function build_shared() {
	${CC} src/main.c -o lib${PROG}.so ${FLAGS} ${LIBS} -rdynamic -shared -fPIC
}

function install() {
	chmod o+x ${PROG}
	cp ${PROG} ${PREFIX}/bin/
}

function install_shared() {
	chmod o+x lib${PROG}.so
	cp lib${PROG}.so ${PREFIX}/lib
	mkdir -p ${PREFIX}/include/${PROG}
	cp -r ${INC}/* ${PREFIX}/include/${PROG}
}

case "$1" in
	shared) build_shared ;;
	install) install ;;
	install_shared) install_shared ;;
	*) build_default ;;
esac >/dev/null
