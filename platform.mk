# platform.mk

MACHINE=${strip ${shell ${CC} -dumpmachine}}
ifndef PLATFORM
	PLATFORM=UNKNOWN
	ifneq (, ${findstring -linux, ${MACHINE}})
		PLATFORM=LINUX
	endif
	ifneq (, ${findstring -freebsd, ${MACHINE}})
		PLATFORM=BSD
	endif
	ifneq (, ${findstring -mingw32, ${MACHINE}})
		PLATFORM=WINDOWS
	endif
	ifneq (, ${findstring -darwin, ${MACHINE}})
		PLATFORM=DARWIN
	endif
endif

ifeq (${NO_SIMD}, 1)
	FLAGS+=-DNO_SIMD
endif

ifeq (${INLINE_RAYLIB}, 1)
	FLAGS+=-DINLINE_RAYLIB
else
	LIBS+=-lraylib
endif

ifeq (${USE_STATIC_MEMORY_ALLOCATOR}, 1)
	FLAGS+=-DMEMORY_ALLOC_STATIC -DMEMORY_ALLOC_STATIC_PRINT_OVERHEAD -DMEMORY_USE_NAMED_TAGS
endif

ifeq (${USE_PORTAUDIO}, 1)
	LIBS+=-lportaudio
	FLAGS+=-DUSE_PORTAUDIO
else ifeq (${USE_MINIAUDIO}, 1)
	FLAGS+=-DUSE_MINIAUDIO
endif

ifeq (${PLATFORM}, LINUX)
	LIBS+=-lpthread -lglfw -ldl
	FLAGS+=-DNPROC=`nproc` -DCACHELINESIZE=`getconf LEVEL1_DCACHE_LINESIZE`
endif

ifeq (${PLATFORM}, WINDOWS)
	LIBS+=-lopengl32 -lgdi32 -lwinmm
	FLAGS+=-DNPROC=${NUMBER_OF_PROCESSORS} -L.\deps\raylib\src -I.\deps\raylib\src
endif

ifeq (${PLATFORM}, DARWIN)
	LIBS+=-lpthread -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo
	FLAGS+=-Wno-missing-braces -DNPROC=`sysctl -n hw.logicalcpu`
endif
