// audio_miniaudio.c

#define MINIAUDIO_IMPLEMENTATION
#define MA_SOUND_FLAG_NO_PITCH
#define MA_SOUND_FLAG_NO_SPATIALIZATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION

#define MA_MALLOC memory_alloc
#define MA_REALLOC memory_realloc
#define MA_FREE memory_free
#include "miniaudio.h"

static ma_device device = {0};

static void stereo_callback(ma_device* device, void* out, const void* in, u32 sample_count) {
  (void)device;
  audio_engine_process(in, out, (i32)sample_count);
}

Result audio_new(Audio_engine* audio) {
  ma_device_config device_config = ma_device_config_init(ma_device_type_playback);
  device_config.playback.format = ma_format_f32;
  device_config.playback.channels = audio->channel_count;
  device_config.sampleRate = audio->sample_rate;
  device_config.dataCallback = stereo_callback;
  device_config.pUserData = NULL;
  device_config.periodSizeInFrames = audio->frames_per_buffer;

  if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "miniaudio error: failed to open playback device\n");
    return Error;
  }

  if (ma_device_start(&device) != MA_SUCCESS) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "miniaudio error: failed to start playback device\n");
    ma_device_uninit(&device);
    return Error;
  }
  log_print(STDOUT_FILENO, LOG_TAG_INFO, "using device: %s\n", device.playback.name);
  log_print(STDOUT_FILENO, LOG_TAG_INFO, "using miniaudio audio backend\n");
  return Ok;
}

void audio_exit(Audio_engine* audio) {
  (void)audio;
  ma_device_uninit(&device);
}
