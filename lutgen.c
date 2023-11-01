#!/usr/bin/env -S tcc -run -Iinclude -Wall -lm

#define STB_SPRINTF_IMPLEMENTATION
#define USE_STB_SPRINTF
#include "src/ext/stb_sprintf.h"

#define COMMON_IMPLEMENTATION
#include "common.h"

#include <math.h>
#include <fcntl.h>

#define WIDTH 7

static void print_header(i32 fd, const char* name, const char* type, size_t size);
static void print_sine_table(i32 fd, const char* name, const char* type, size_t size);

i32 main(void) {
  const char* path = "include/lut.h";
  i32 fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0664);
  if (fd < 0) {
    dprintf(STDERR_FILENO, "error: failed to open file `%s` for writing\n", path);
    exit(EXIT_FAILURE);
  }
  dprintf(fd, "// lut.h\n");
  print_sine_table(fd, "sine", "f32", 2096);
  close(fd);
  return EXIT_SUCCESS;
}

void print_header(i32 fd, const char* name, const char* type, size_t size) {
  dprintf(fd, "static const %s %s[%zu] = {\n", type, name, size);
}

void print_sine_table(i32 fd, const char* name, const char* type, size_t size) {
  print_header(fd, name, type, size);
  for (size_t i = 0; i < size; ++i) {
    f32 v = sinf((i * 2 * 110) / (f32)size);
    dprintf(fd, "%.6ff,", v);
    if (!((i+1) % WIDTH)) {
      dprintf(fd, "\n");
    }
  }
  dprintf(fd, "};\n");
}
