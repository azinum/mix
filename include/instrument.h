// instrument.h

#ifndef _INSTRUMENT_H
#define _INSTRUMENT_H

#define INSTRUMENT_VOLUME_DEFAULT 0.1f

struct Mix;
struct Audio_engine;

typedef struct Instrument {
  f32* buffer;
  size_t samples;
  f32 volume;
  f32 latency;
  f32 audio_latency;
  bool blocking;

  void* userdata;
  char* title;

  void (*init)(struct Instrument*);
  void (*ui_new)(struct Instrument*, Element*);
  void (*update)(struct Instrument*, struct Mix*);
  void (*process)(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
  void (*free)(struct Instrument*);
} Instrument;

typedef enum {
  INSTRUMENT_WAVE_SHAPER,

  MAX_INSTRUMENT_ID,
} Instrument_id;

Instrument instrument_new(Instrument_id id);
Instrument instrument_new_from_path(const char* path);
void instrument_init(Instrument* ins, struct Audio_engine* audio);
Element instrument_ui_new(Instrument* ins);
void instrument_update(Instrument* ins, struct Mix* mix);
void instrument_process(Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void instrument_free(Instrument* ins);

#endif // _INSTRUMENT_H
