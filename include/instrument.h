// instrument.h

#ifndef _INSTRUMENT_H
#define _INSTRUMENT_H

#define DECLARE_INSTRUMENT(NAME) \
  void NAME##_init(Instrument* ins, struct Mix* mix); \
  void NAME##_ui_new(Instrument* ins, Element* container); \
  void NAME##_update(Instrument* ins, struct Mix* mix); \
  void NAME##_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt); \
  void NAME##_noteon(struct Instrument* ins, u8 note, f32 velocity); \
  void NAME##_noteoff(struct Instrument* ins, u8 note); \
  void NAME##_destroy(struct Instrument* ins)

#define INSTRUMENT_VOLUME_DEFAULT 0.4f

struct Mix;
struct Audio_engine;

typedef struct Instrument {
  f32* in_buffer;
  f32* out_buffer;
  size_t samples;
  f32 volume;
  f32 latency;
  f32 audio_latency;
  f32 wet;
  bool blocking;
  bool initialized;

  Ticket blocking_mutex;

  void* userdata;
  char* title;

  void (*init)(struct Instrument*, struct Mix*);
  void (*ui_new)(struct Instrument*, Element*);
  void (*update)(struct Instrument*, struct Mix*);
  void (*process)(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
  void (*noteon)(struct Instrument*, u8 note, f32 velocity);
  void (*noteoff)(struct Instrument*, u8 note);
  void (*destroy)(struct Instrument*);

  Element* ui;
} Instrument;

typedef enum {
  INSTRUMENT_WAVE_SHAPER,
  INSTRUMENT_DUMMY,
  INSTRUMENT_NOISE,
  INSTRUMENT_AUDIO_INPUT,
  INSTRUMENT_BASIC_POLY_SYNTH,
  INSTRUMENT_TRACKER,
  INSTRUMENT_PHYSICAL,

  MAX_INSTRUMENT_ID,
} Instrument_id;

extern Instrument instruments[MAX_INSTRUMENT_ID];

DECLARE_INSTRUMENT(waveshaper);
DECLARE_INSTRUMENT(dummy);
DECLARE_INSTRUMENT(noise);
DECLARE_INSTRUMENT(audio_input);
DECLARE_INSTRUMENT(basic_poly_synth);
DECLARE_INSTRUMENT(tracker);
DECLARE_INSTRUMENT(physical);

void instrument_init_default(Instrument* ins);
Instrument instrument_new(Instrument_id id);
Instrument instrument_new_from_path(const char* path);
void instrument_init(Instrument* ins, struct Audio_engine* audio, struct Mix* mix);
void instrument_ui_new(Instrument* ins, Element* container);
void instrument_update(Instrument* ins, struct Mix* mix);
void instrument_process(Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void instrument_destroy(Instrument* ins);

#undef DECLARE_INSTRUMENT
#endif // _INSTRUMENT_H
