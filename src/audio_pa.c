// audio_pa.c

#include <portaudio.h>

static PaStream* stream = NULL;
static PaStreamParameters out_port;
static PaStreamParameters in_port;

static i32 stereo_callback(const void* in, void* out, unsigned long frames_per_buffer, const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags flags, void* userdata) {
  (void)time_info;
  (void)userdata;
  (void)flags;
  return ((const int[]){paAbort, paContinue})[audio_engine_process(in, out, (i32)frames_per_buffer) == Ok];
}

static Result open_stream(void) {
  Audio_engine* audio = &audio_engine;
  PaStreamParameters* input_port = AUDIO_INPUT ? &in_port : NULL;

  PaError err = Pa_OpenStream(
    &stream,
    input_port,
    &out_port,
    audio->sample_rate,
    audio->frames_per_buffer,
    paNoFlag,
    stereo_callback,
    NULL
  );
  if (err != paNoError) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "portaudio error: %s\n", Pa_GetErrorText(err));
    return Error;
  }
  err = Pa_StartStream(stream);
  if (err != paNoError) {
    Pa_Terminate();
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "portaudio error: %s\n", Pa_GetErrorText(err));
    return Error;
  }
  return Ok;
}

Result audio_new(Audio_engine* audio) {
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    Pa_Terminate();
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "portaudio error: %s\n", Pa_GetErrorText(err));
    return Error;
  }
  i32 device_count = Pa_GetDeviceCount();

  if (AUDIO_INPUT) {
    i32 input_device = Pa_GetDefaultInputDevice();
    in_port.device = AUDIO_PA_IN_PORT_ID >= 0 && AUDIO_PA_IN_PORT_ID < device_count ? AUDIO_PA_IN_PORT_ID : input_device;
    in_port.channelCount = CHANNEL_COUNT;
    in_port.sampleFormat = paFloat32;
    in_port.suggestedLatency = Pa_GetDeviceInfo(in_port.device)->defaultLowInputLatency;
    in_port.hostApiSpecificStreamInfo = NULL;
  }

  i32 output_device = Pa_GetDefaultOutputDevice();
  out_port.device = output_device;
  out_port.channelCount = audio->channel_count;
  out_port.sampleFormat = paFloat32;
  out_port.suggestedLatency = Pa_GetDeviceInfo(out_port.device)->defaultLowOutputLatency;
  out_port.hostApiSpecificStreamInfo = NULL;

  stb_dprintf(STDOUT_FILENO, "ID | INPUTS | OUTPUTS | SAMPLE RATE | DEVICE NAME\n");
  for (i32 device = 0; device < device_count; ++device) {
    const PaDeviceInfo* info = Pa_GetDeviceInfo(device);
    stb_dprintf(STDOUT_FILENO, "%-2i", device);
    stb_dprintf(STDOUT_FILENO, " | %6i | %7i | %11g", info->maxInputChannels, info->maxOutputChannels, info->defaultSampleRate);
    stb_dprintf(STDOUT_FILENO, " | %s", info->name);
    if (output_device == device) {
      stb_dprintf(STDOUT_FILENO, " [SELECTED]");
    }
    stb_dprintf(STDOUT_FILENO, "\n");
  }

  if ((err = Pa_IsFormatSupported(NULL, &out_port, audio->sample_rate)) != paFormatIsSupported) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "portaudio error: %s\n", Pa_GetErrorText(err));
    return Error;
  }

  log_print(STDOUT_FILENO, LOG_TAG_INFO, "using portaudio audio backend\n");
  return open_stream();
}

void audio_exit(Audio_engine* e) {
  (void)e;
  Pa_CloseStream(stream);
  Pa_Terminate();
}
