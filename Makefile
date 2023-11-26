# Makefile

ifeq ("${wildcard config.mk}", "")
$(shell cp default.mk config.mk)
endif

include config.mk
include platform.mk

all: ${TARGET}

${TARGET}: ${SRC}
	${CC} $< -o $@ ${FLAGS} ${LIBS}

run:
	./${TARGET}

clean:
	rm -f ${TARGET}

.PHONY: ${TARGET} clean
