# Makefile

ifeq ("${wildcard config.mk}", "")
$(shell cp default.mk config.mk)
endif

include config.mk
include platform.mk

all: ${TARGET}

${TARGET}: ${SRC}
	${CC} $< -o $@ ${FLAGS} ${LIBS} ${FLAGS_DEBUG}

release:
	${CC} src/main.c -o ${PROG} ${FLAGS} ${LIBS} ${FLAGS_RELEASE}

install: release
	mkdir ${DATA_PATH} -p
	cp -dr data ${DATA_PATH}
	chmod o+x ${TARGET}
	cp ${TARGET} ${PREFIX}/bin/

install_shared:
	chmod o+x lib${TARGET}.so
	cp lib${TARGET}.so ${PREFIX}/lib
	mkdir -p ${PREFIX}/include/${TARGET}
	cp -r ${INC}/* ${PREFIX}/include/${TARGET}

shared: ${SRC}
	${CC} $< -o lib${TARGET}.so ${FLAGS} ${LIBS} -shared -rdynamic -fPIC || echo "\nerror: compiling shared library failed; make sure raylib is compiled with position independent code (-fPIC)"

profile:
	perf record -e cycles -c 2000000 ./${TARGET} && perf report -n -f > perf.txt && rm -f perf.data perf.data.old

deps:
	make -C deps/raylib/src RAYLIB_MODULE_MODELS=FALSE

test:
	${CC} src/test.c -o test ${FLAGS}

android:
	make -C android

run:
	./${TARGET}

clean:
	rm -f ${TARGET}

.PHONY: ${TARGET} clean test deps android
