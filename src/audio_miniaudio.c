// audio_miniaudio.c

// #error "miniaudio is not supported yet"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

static ma_device device = {0};

static void stereo_callback(ma_device* device, void* out, const void* in, u32 sample_count) {
  (void)device;
  audio_engine_process(in, out, (i32)sample_count);
}

Result audio_new(Audio_engine* e) {
  ma_device_config device_config = ma_device_config_init(ma_device_type_playback);
  device_config.playback.format = ma_format_f32;
  device_config.playback.channels = e->channel_count;
  device_config.sampleRate = e->sample_rate;
  device_config.dataCallback = stereo_callback;
  device_config.pUserData = NULL;
  device_config.periodSizeInFrames = e->frames_per_buffer;

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

void audio_exit(Audio_engine* e) {
  (void)e;
  ma_device_uninit(&device);
}
