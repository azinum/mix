# Makefile

ifeq ("${wildcard config.mk}", "")
$(shell cp default.mk config.mk)
endif

include config.mk
include platform.mk

all: ${TARGET}

${TARGET}: ${SRC}
	${CC} $< -o $@ ${FLAGS} ${LIBS}

install:
	chmod o+x ${TARGET}
	cp ${TARGET} ${PREFIX}/bin/

install_shared:
	chmod o+x lib${TARGET}.so
	cp lib${TARGET}.so ${PREFIX}/lib
	mkdir -p ${PREFIX}/include/${TARGET}
	cp -r ${INC}/* ${PREFIX}/include/${TARGET}

shared: ${SRC}
	${CC} $< -o lib${TARGET}.so ${FLAGS} ${LIBS} -shared -rdynamic -fPIC

profile:
	perf record -e cycles -c 2000000 ./${TARGET} && perf report -n -f > perf.txt && rm -f perf.data perf.data.old

run:
	./${TARGET}

clean:
	rm -f ${TARGET}

.PHONY: ${TARGET} clean
