// test_ui.c

#define ELEMENT_COUNT 1024

static void button_onclick(Element* e);

void button_onclick(Element* e) {
  stb_printf("button %u said ok!\n", e->id);
}

Element test_ui_new(void) {
  random_init(0xbadb011);
  Element test_ui = ui_container(NULL);
  test_ui.sizing = SIZING_PERCENT(100, 100);
  test_ui.scissor = false;
  test_ui.border = false;
  test_ui.background = false;
  test_ui.placement = PLACEMENT_BLOCK;
  test_ui.padding = 24;

  Element container_default = ui_container(NULL);
  container_default.placement = PLACEMENT_BLOCK;
  container_default.border = container_default.background = true;
  container_default.scissor = false;

  const i32 button_height = 2 * FONT_SIZE;

  static f32 some_float = 0.5f;
  static i32 some_int = 3;

  Element button_default = ui_button("ok");
  button_default.box = BOX(0, 0, 0, button_height);
  button_default.tooltip = "ok";
  button_default.onclick = button_onclick;

  Element int_slider_default = ui_slider_int(&some_int, 0, 10);
  Element float_slider_default = ui_slider_float(&some_float, 0.0f, 1.0f);
  Element int_input_default = ui_input_int("some_int", &some_int);
  Element float_input_default = ui_input_float("some_float", &some_float);

  Element some_int_text = ui_text("some_int");
  some_int_text.sizing = SIZING_PERCENT(100, 0);
  Element some_float_text = ui_text("some_float");
  some_float_text.sizing = SIZING_PERCENT(100, 0);

  Element elements[] = {
    some_int_text,
    int_slider_default,
    some_float_text,
    float_slider_default,
    some_int_text,
    int_input_default,
    some_float_text,
    float_input_default,
  };

  {
    Element e = container_default;
    ui_set_title(&e, "scissor: on");
    e.sizing = SIZING_PERCENT(50, 50);
    e.scissor = true;
    Element* container = ui_attach_element(&test_ui, &e);
    for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
      Element e = button_default;
      e.box.w = 32 + random_number() % 64;
      ui_attach_element(container, &e);
    }
  }
  {
    Element e = container_default;
    e.sizing = SIZING_PERCENT(50, 50);
    e.scissor = true;
    Element* container = ui_attach_element(&test_ui, &e);
    for (size_t i = 0; i < 16; ++i) {
      Element e = button_default;
      e.box.w = 32 + random_number() % 64;
      ui_attach_element(container, &e);
    }
    for (size_t i = 0; i < LENGTH(elements); ++i) {
      Element e = elements[i];
      switch (e.type) {
        case ELEMENT_INPUT: {
          e.box.h = FONT_SIZE;
          e.sizing = SIZING_PERCENT(100, 0);
          break;
        }
        case ELEMENT_SLIDER: {
          e.box.h = button_height;
          e.sizing = SIZING_PERCENT(100, 0);
          break;
        }
        default: {
          break;
        }
      }
      ui_attach_element(container, &e);
    }
  }
  {
    Element e = container_default;
    e.scissor = true;
    e.sizing = SIZING_PERCENT(50, 150);
    Element* container = ui_attach_element(&test_ui, &e);
    for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
      Element e = button_default;
      e.box.w = 32 + random_number() % 64;
      ui_attach_element(container, &e);
    }
  }
  {
    Element e = container_default;
    ui_set_title(&e, "scissor: off");
    e.sizing = SIZING_PERCENT(50, 50);
    Element* container = ui_attach_element(&test_ui, &e);
    for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
      Element e = button_default;
      e.box.w = 32 + random_number() % 64;
      ui_attach_element(container, &e);
    }
  }
  return test_ui;
}

#undef ELEMENT_COUNT
#undef LOTS_OF_TEXT
