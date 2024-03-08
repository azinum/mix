// lutgen.c
// cc lutgen.c -o lutgen -Wall -Iinclude -lm -lraylib -lpthread -ldl

#include <math.h>
#include <fcntl.h>
#include <raylib.h>
#include <float.h>

#define STB_SPRINTF_IMPLEMENTATION
#define USE_STB_SPRINTF
#include "src/ext/stb/stb_sprintf.h"

#define COMMON_IMPLEMENTATION
#include "common.h"

#include "memory.h"
#include "buffer.h"
#include "src/memory.c"
#include "src/buffer.c"

#define SAMPLE_RATE 44100
#define CHANNEL_COUNT 2
#define WIDTH 7

typedef enum Shape {
  SHAPE_SINE,
  SHAPE_NOISY,
  SHAPE_SAW,

  MAX_SHAPE,
} Shape;

typedef struct Sound_source {
  f32 buffer[SAMPLE_RATE * CHANNEL_COUNT];
} Sound_source;

static void sound_init(Sound_source* sound);
static void generate(Sound_source* sound, Shape shape, f32 amplitude);
static void export(Sound_source* sound, const char* path);
static void print_header(i32 fd, const char* name, const char* type, size_t size);
static void print_sine_table(i32 fd, const char* name, const char* type, size_t size);
static void print_wave_file(i32 fd, const char* path, const char* name);

i32 main(void) {
  memory_init();
  SetTraceLogLevel(LOG_WARNING);

  Sound_source sound;
  {
    sound_init(&sound);
    generate(&sound, SHAPE_SINE, 1.0f);
    export(&sound, "data/audio/shapes/sine.wav");
  }
  {
    sound_init(&sound);
    generate(&sound, SHAPE_NOISY, 1.0f);
    export(&sound, "data/audio/shapes/noisy.wav");
  }
  {
    sound_init(&sound);
    generate(&sound, SHAPE_SAW, 1.0f);
    export(&sound, "data/audio/shapes/saw.wav");
  }
  {
    sound_init(&sound);
    generate(&sound, SHAPE_SAW, 0.5f);
    generate(&sound, SHAPE_SINE, 0.5f);
    export(&sound, "data/audio/shapes/saw_and_sine.wav");
  }
  {
    sound_init(&sound);
    generate(&sound, SHAPE_SAW, 0.2f);
    generate(&sound, SHAPE_SINE, 0.2f);
    generate(&sound, SHAPE_SAW, 0.2f);
    generate(&sound, SHAPE_NOISY, 0.2f);
    generate(&sound, SHAPE_SINE, 0.2f);
    export(&sound, "data/audio/shapes/combined.wav");
  }
  // print_wave_file(STDOUT_FILENO, "data/audio/drums/kick.wav", "kick");
  // print_wave_file(STDOUT_FILENO, "data/audio/drums/snare.wav", "snare");
  // print_wave_file(STDOUT_FILENO, "data/audio/drums/hihat.wav", "hihat");
  // print_sine_table(STDOUT_FILENO, "sine", "f32", 44100);
  return EXIT_SUCCESS;
}

void sound_init(Sound_source* sound) {
  memset(sound->buffer, 0, sizeof(sound->buffer));
}

void generate(Sound_source* sound, Shape shape, f32 amplitude) {
  const size_t samples = SAMPLE_RATE * CHANNEL_COUNT;
  switch (shape) {
    case SHAPE_SINE: {
      for (size_t i = 0; i < samples; ++i) {
        f32 sample = amplitude * sinf((i * PI32 * CHANNEL_COUNT) / ((f32)samples / CHANNEL_COUNT));
        sound->buffer[i] += sample;
      }
      break;
    }
    case SHAPE_NOISY: {
      for (size_t i = 1; i < samples; ++i) {
        f32 sample = amplitude * sinf((i * PI32 * CHANNEL_COUNT) / ((f32)samples / CHANNEL_COUNT));
        f32 prev = sound->buffer[i - 1];
        if (fabs(prev) < FLT_EPSILON) {
          prev = 1.0f;
        }
        sample += (sample * sample * sample) / prev;
        sound->buffer[i] += sample;
      }
      break;
    }
    case SHAPE_SAW: {
      f32 step = 2.0f / ((f32)samples / CHANNEL_COUNT);
      f32 sample = -1.0f;
      for (size_t i = 0; i < samples; ++i) {
        sound->buffer[i] += sample;
        sample += step;
      }
      break;
    }
    default: {
      ASSERT(0);
      break;
    }
  }
}

void export(Sound_source* sound, const char* path) {
  const size_t samples = SAMPLE_RATE * CHANNEL_COUNT;
  i16* buffer = memory_alloc(sizeof(i16) * samples);
  ASSERT(buffer != NULL);
  for (size_t i = 0; i < samples; ++i) {
    buffer[i] = (i16)(sound->buffer[i] * INT16_MAX);
  }
  Wave wave = (Wave) {
    .frameCount = SAMPLE_RATE / CHANNEL_COUNT,
    .sampleRate = SAMPLE_RATE,
    .sampleSize = 8 * sizeof(i16),
    .channels   = CHANNEL_COUNT,
    .data       = buffer,
  };
  ExportWave(wave, path);
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
