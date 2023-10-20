#!/usr/bin/env -S tcc -run -Iinclude -Wall -lm

#define COMMON_IMPLEMENTATION
#include "common.h"

#include <math.h>
#include <fcntl.h>

#define WIDTH 7

typedef i32 (*generator_fn)(i32);

typedef enum {
  TYPE_FLOAT,
  TYPE_INTEGER,

  MAX_TYPE,
} Type;

const char* type_str[] = {
  "f32",
  "i32",
};

const char* type_fmt_str[] = {
  "%.6ff",
  "%d",
};

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
  print_sine_table(fd, "sine", "f32", WIDTH*20);
  close(fd);
  return EXIT_SUCCESS;
}

void print_header(i32 fd, const char* name, const char* type, size_t size) {
  dprintf(fd, "static const %s %s[%zu] = {\n", type, name, size);
}

void print_sine_table(i32 fd, const char* name, const char* type, size_t size) {
  print_header(fd, name, type, size);
  for (size_t i = 0; i < size; ++i) {
    f32 v = sinf((f32)i);
    dprintf(fd, "%.6ff,", v);
    if (!((i+1) % WIDTH)) {
      dprintf(fd, "\n");
    }
  }
  dprintf(fd, "};\n");
}
