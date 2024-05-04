// ui_audio.c

static void ui_audio_draw_sample(i32 mouse_x, i32 mouse_y, i32 width, i32 height, Audio_source* source);
static void ui_audio_render_sample(Element* e);
static void ui_audio_drag_and_drop_sample(Element* e);
static Result ui_audio_load_sample(const char* path, Audio_source* source);

void ui_audio_draw_sample(i32 mouse_x, i32 mouse_y, i32 width, i32 height, Audio_source* source) {
  if (!source->buffer) {
    return;
  }

  bool mod_key = IsKeyDown(KEY_LEFT_CONTROL);

  if (!mod_key && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !ui_input_interacting() && width > 0 && height > 0 && source->ready && !source->internal) {
    f32 x = mouse_x / (f32)width;
    f32 sample = -(mouse_y / (f32)height - 0.5f) * 2.0f;
    i32 sample_index = x * source->samples;
    i32 window_size = ((source->channel_count * source->samples) / width) * 4;
    sample_index = CLAMP(sample_index - (window_size / 2), 0, (i32)source->samples - window_size);
    if (window_size > 0) {
      f32 step = 2.0f / window_size;
      f32 interpolator = 0.0f;
      if (sample_index == 0) {
        interpolator += 0.5f;
        step -= 0.5f / window_size;
      }
      for (i32 i = 0; i < window_size; ++i) {
        interpolator += step;
        f32 factor = sinf((interpolator * PI32) / 2.0f);
        f32 current_sample = source->buffer[(sample_index + i) % source->samples];
        source->buffer[(sample_index + i) % source->samples] = lerp_f32(current_sample, sample, factor);
      }
    }
  }
}

void ui_audio_render_sample(Element* e) {
  Audio_source* source = (Audio_source*)e->userdata;
  if (!source->buffer) {
    return;
  }

  ui_audio_render_curve(source->buffer, source->samples, e->box, COLOR_RGB(130, 190, 100), true, source->cursor);
}

void ui_audio_drag_and_drop_sample(Element* e) {
  Audio_source* source = (Audio_source*)e->userdata;

  if (IsFileDropped()) {
    FilePathList files = LoadDroppedFiles();
    if (files.count > 0) {
      const char* path = files.paths[0];
      ui_audio_load_sample(path, source);
      UnloadDroppedFiles(files);
    }
  }

  if (source->drawable) {
    ui_audio_draw_sample(e->data.canvas.mouse_x, e->data.canvas.mouse_y, e->box.w, e->box.h, source);
  }
}

Result ui_audio_load_sample(const char* path, Audio_source* source) {
  Audio_source loaded_source = audio_load_audio(path);
  Audio_source copy = *source;
  audio_unload_audio(&copy);
  if (loaded_source.buffer != NULL && loaded_source.samples > 0) {
    ticket_mutex_begin(&source->mutex);
    audio_source_copy(source, &loaded_source);
    ticket_mutex_end(&source->mutex);
    return Ok;
  }
  ui_alert("failed to load audio file %s", path);
  return Error;
}

Element ui_audio_canvas(const char* title, i32 height, Audio_source* source, bool drawable) {
  return ui_audio_canvas_ex(title, height, source, drawable, NULL);
}

Element ui_audio_canvas_ex(const char* title, i32 height, Audio_source* source, bool drawable, Element** canvas) {
  Element container = ui_container((char*)title);
  container.data.container.auto_adjust_height = true;
  source->drawable = drawable;
  {
    Element e = ui_canvas(true);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = height, };
    e.userdata = source;
    e.onrender = ui_audio_render_sample;
    e.onhover = ui_audio_drag_and_drop_sample;
    e.tooltip = "drag and drop audio file here";
    Element* canvas_element = ui_attach_element(&container, &e);
    if (canvas) {
      *canvas = canvas_element;
    }
  }
  return container;
}

void ui_audio_render_curve(const f32* samples, const size_t count, Box box, Color color, bool render_cursor, const size_t cursor) {
  box = ui_pad_box_ex(box, 1, 2);
  const i32 width = box.w;
  const i32 height = box.h;
  const i32 x = box.x;
  const i32 y = box.y + height / 2;
  Color colors[2] = {
    color,
    saturate_color(color, 0.4f)
  };
  DrawLine(x, y, x + width, y, COLOR(255, 255, 255, 50));
  // invert samples so that negative values are in the bottom, and positive at top
  f32 sample = -CLAMP(samples[0], -1.0f, 1.0f);
  f32 prev_sample = 0;
  f32 index = 0;
  f32 step = count / (f32)width;
  for (i32 i = 0; i < width && index < (f32)count; ++i, index += step) {
    prev_sample = sample;
    sample = -CLAMP(samples[(size_t)index], -1.0f, 1.0f);
    DrawLine(
      x + i,
      y + (height / 2.0f * prev_sample),
      x + i + 1,
      y + (height / 2.0f * sample),
      colors[(i % 2) == 0]
    );
  }
  if (render_cursor) {
    DrawLine(
      box.x + width * (cursor/(f32)count),
      box.y,
      box.x + width * (cursor/(f32)count),
      box.y + height,
      brighten_color(saturate_color(color, -0.3f), 0.2f)
    );
  }
}
