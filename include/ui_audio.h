// ui_audio.h

#ifndef _UI_AUDIO_H
#define _UI_AUDIO_H

// container (settings: auto adjust height)
//  canvas - holds source in userdata, can drag and drop samples into canvas to replace (settings: 100% width)
//  controls
Element ui_audio_canvas(const char* title, i32 height, Audio_source* source, bool drawable);
void ui_audio_render_curve(const f32* samples, const size_t count, Box box, Color color, bool render_cursor, const size_t cursor);

#endif // _UI_AUDIO_H
