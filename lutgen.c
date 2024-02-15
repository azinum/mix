#!/usr/bin/env -S tcc -run -Iinclude -Wall -lm

#include <math.h>
#include <fcntl.h>

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
  size_t sample_count = (buffer.count - WAVE_HEADER_SIZE) / sizeof(i16);
  print_header(fd, name, "f32", sample_count);
  if (buffer.count > WAVE_HEADER_SIZE) {
    i16* sample = (i16*)&buffer.data[WAVE_HEADER_SIZE];
    for (size_t i = WAVE_HEADER_SIZE; i < buffer.count; i += sizeof(i16), sample += 1) {
      stb_dprintf(fd, "%.6ff,", *sample / (f32)INT16_MAX);
      if (!((i+1) % WIDTH)) {
        stb_dprintf(fd, "\n");
      }
    }
    stb_dprintf(fd, "};\n");
  }
  buffer_free(&buffer);
  close(wave_fd);
}
