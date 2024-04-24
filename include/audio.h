// audio.h
// macros:
// NO_RECORD_BUFFER

#ifndef _AUDIO_H
#define _AUDIO_H

// can not add more than INIT_ITEMS_SIZE of effects, because if the list of elements grows pointers to those elements will be invalidated, and there is currently no handling of this
#define MAX_EFFECTS (INIT_ITEMS_SIZE)

#define MINUTES(SECS) (SECS * 60)
#define HOURS(MINS) (MINUTES(60 * MINS)) 

#define RECORD_BUFFER_LENGTH_SECS (MINUTES(10))

// convert from milliseconds to number of samples, based on the channel rate and channel count
#define MS_TO_SAMPLES(sample_rate, channels, ms) ((sample_rate) * ((ms / 1000.0f) * channels))
#define SECS_TO_SAMPLES(sample_rate, channels, seconds) (MS_TO_SAMPLES(sample_rate, channels, seconds * 1000.0f))

typedef struct Audio_source {
  f32* buffer;
  size_t samples;
  u32 channel_count;
  bool ready;
  bool internal;
  i32 drawable;
  size_t cursor;
  Ticket mutex;
} Audio_source;

typedef struct Audio_engine {
  i32 sample_rate;
  i32 frames_per_buffer;
  i32 channel_count;
  f32* out_buffer;
  f32* in_buffer;
  f32 dt;
  bool quit; // set to true to stop processing audio
  bool done; // done is set to true when audio processing has stopped
  bool restart; // restart is set whenever the audio engine needs to be restarted, but only after processing current audio buffer
  Instrument instrument;
  Effect effect_chain[MAX_EFFECTS];
  size_t effect_count;
  i16* record_buffer;
  i32 record_buffer_size;
  i32 record_buffer_index;
  i32 recording;
  f32 db;
} Audio_engine;

extern Audio_engine audio_engine;

Audio_engine audio_engine_new(i32 sample_rate, i32 frames_per_buffer, i32 channel_count);
Result audio_engine_start(Audio_engine* audio);
Result audio_engine_start_new(Audio_engine* audio);
Result audio_engine_detach_instrument(void);
Instrument* audio_engine_attach_instrument(Instrument* ins);
Effect* audio_engine_attach_effect(Effect* effect);
void audio_engine_clear_effects(void);
void audio_engine_restart(void);
void audio_engine_exit(Audio_engine* audio);
Audio_source audio_load_audio(const char* path);
void audio_unload_audio(Audio_source* source);
void audio_copy_split(const f32* input, f32* left_output, f32* right_output, const size_t samples);
Audio_source audio_source_copy_into_new(const f32* input, const size_t samples, const u32 channel_count);
void audio_source_copy(Audio_source* dest, Audio_source* source); // basic copy, but keeps the mutex from source as is
f32 audio_calc_rms(f32* buffer, size_t size);
f32 audio_calc_rms_clamp(f32* buffer, size_t size);
Result audio_engine_process(const void* in, void* out, i32 frames);

#endif // _AUDIO_H
