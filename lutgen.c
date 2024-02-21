// lutgen.c
// cc lutgen.c -o lutgen -Wall -Iinclude -lm -lraylib -lpthread -ldl

#include <math.h>
#include <fcntl.h>
#include <raylib.h>

#define STB_SPRINTF_IMPLEMENTATION
#define USE_STB_SPRINTF
#include "src/ext/stb/stb_sprintf.h"

#define COMMON_IMPLEMENTATION
#include "common.h"

#include "memory.h"
#include "buffer.h"
#include "src/memory.c"
#include "src/buffer.c"

#define WIDTH 7

static void print_header(i32 fd, const char* name, const char* type, size_t size);
static void print_sine_table(i32 fd, const char* name, const char* type, size_t size);
static void print_wave_file(i32 fd, const char* path, const char* name);

i32 main(void) {
  memory_init();
  SetTraceLogLevel(LOG_WARNING);
  print_wave_file(STDOUT_FILENO, "data/audio/kick.wav", "kick");
  print_wave_file(STDOUT_FILENO, "data/audio/snare.wav", "snare");
  print_wave_file(STDOUT_FILENO, "data/audio/hihat.wav", "hihat");
  print_sine_table(STDOUT_FILENO, "sine", "f32", 44100);
  return EXIT_SUCCESS;
}

void print_header(i32 fd, const char* name, const char* type, size_t size) {
  dprintf(fd, "static const %s %s[%zu] = {\n", type, name, size);
}

void print_sine_table(i32 fd, const char* name, const char* type, size_t size) {
  print_header(fd, name, type, size);
  for (size_t i = 0; i < size; ++i) {
    f32 v = sinf((i * PI32 * 2) / (f32)size);
    stb_dprintf(fd, "%.6ff,", v);
    if (!((i+1) % WIDTH)) {
      stb_dprintf(fd, "\n");
    }
  }
  stb_dprintf(fd, "};\n");
}

void print_wave_file(i32 fd, const char* path, const char* name) {
  i32 wave_fd = open(path, O_RDONLY);
  if (wave_fd < 0) {
    return;
  }
#define WAVE_HEADER_SIZE 44
  Buffer buffer = buffer_new_from_fd(wave_fd);
  close(wave_fd);

  Wave wave = LoadWaveFromMemory(".wav", buffer.data, buffer.count);
  if (wave.data) {
    size_t sample_count = wave.frameCount * wave.channels;
    print_header(fd, name, "f32", sample_count);
    i16* data = (i16*)wave.data;
    for (size_t i = 0; i < sample_count; ++i) {
      f32 sample = data[i] / (f32)INT16_MAX;
      dprintf(fd, "%.6ff,", sample);
      if (!((i+1) % WIDTH)) {
        dprintf(fd, "\n");
      }
    }
    dprintf(fd, "};\n");
  }
  UnloadWave(wave);
  buffer_free(&buffer);
}
