// test_ui.c

#define ELEMENT_COUNT 512
// #define LOTS_OF_TEXT

static void button_onclick(Element* e);

#ifdef LOTS_OF_TEXT
static char* lots_of_text0;
static char* lots_of_text1;
#endif

void button_onclick(Element* e) {
  stb_printf("button %u said ok!\n", e->id);
}

Element test_ui_new(void) {
  // random_init(0xbadb011);
  f32 seed = get_time();
  random_init(hash_djb2((u8*)&seed, sizeof(seed)));
  Element test_ui = ui_container(NULL);
  test_ui.sizing = SIZING_PERCENT(100, 100);
  test_ui.scissor = false;
  test_ui.border = false;
  test_ui.background = false;
  test_ui.placement = PLACEMENT_BLOCK;
  test_ui.padding = 24;

#ifdef LOTS_OF_TEXT
  (void)button_onclick; // unused when LOTS_OF_TEXT is defined
  {
    Element e = ui_text(lots_of_text0);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&test_ui, &e);
  }
  {
    Element e = ui_text(lots_of_text1);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&test_ui, &e);
  }
#else
  Element container_default = ui_container(NULL);
  container_default.placement = PLACEMENT_BLOCK;
  container_default.border = container_default.background = true;
  container_default.scissor = false;

  Element container_rows = container_default;
  container_rows.placement = PLACEMENT_ROWS;

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
  some_int_text.box.h = FONT_SIZE;
  some_int_text.sizing = SIZING_PERCENT(100, 0);
  Element some_float_text = ui_text("some_float");
  some_float_text.sizing = SIZING_PERCENT(100, 0);

  Element elements[] = {
    some_int_text,
    int_slider_default,
    //some_float_text,
    float_slider_default,
    //some_int_text,
    int_input_default,
    //some_float_text,
    float_input_default,
  };

  {
    Element e = container_default;
    ui_set_title(&e, "sliders");
    e.sizing = SIZING_PERCENT(50, 50);
    e.scissor = true;
    Element* container = ui_attach_element(&test_ui, &e);
    static f32 values[ELEMENT_COUNT] = {0.0f};
    for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
      values[i] = random_number() / (f32)(RANDOM_MAX >> 2);
    }
    for (size_t i = 0; i < ELEMENT_COUNT; ++i) {
      Element e = ui_slider_float(&values[i], 0.0f, 1.0f);
      e.data.slider.slider_type = SLIDER_VERTICAL;
      e.box.w = 20;
      e.box.h = 120;
      ui_attach_element(container, &e);
    }
  }
  {
    Element e = container_default;
    e.sizing = SIZING_PERCENT(50, 50);
    e.scissor = true;
    Element* container = ui_attach_element(&test_ui, &e);

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
    Element line_break = ui_line_break(FONT_SIZE);
    ui_attach_element(container, &line_break);
    for (size_t i = 0; i < 16; ++i) {
      Element e = button_default;
      e.box.w = 32 + random_number() % 64;
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
    Element e = container_rows;
    e.scissor = true;
    e.sizing = SIZING_PERCENT(50, 70);
    Element* container = ui_attach_element(&test_ui, &e);
    for (size_t i = 0; i < 32; ++i) {
      Element e = button_default;
      e.sizing = (Sizing) {
        .x_mode = SIZE_MODE_PERCENT,
        .y_mode = SIZE_MODE_PIXELS,
        .x = 100,
        .y = button_height,
      };
      ui_attach_element(container, &e);
    }
  }
#endif // !defined(LOTS_OF_TEXT)
  return test_ui;
}

#ifdef LOTS_OF_TEXT
static char* lots_of_text0 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut placerat orci nulla pellentesque. Aenean sed adipiscing diam donec adipiscing tristique risus nec. Amet purus gravida quis blandit. Ultrices tincidunt arcu non sodales neque sodales. Vulputate sapien nec sagittis aliquam. Magna eget est lorem ipsum dolor sit amet consectetur adipiscing. Tellus cras adipiscing enim eu turpis egestas pretium. Laoreet suspendisse interdum consectetur libero id faucibus nisl tincidunt. Cursus sit amet dictum sit amet. Egestas quis ipsum suspendisse ultrices. Massa placerat duis ultricies lacus sed turpis tincidunt id. Dictum fusce ut placerat orci nulla. Cursus risus at ultrices mi tempus imperdiet nulla. Viverra maecenas accumsan lacus vel facilisis volutpat est velit egestas. Ullamcorper sit amet risus nullam. Vitae nunc sed velit dignissim sodales. Rhoncus mattis rhoncus urna neque viverra justo. Vulputate sapien nec sagittis aliquam malesuada bibendum arcu.";
static char* lots_of_text1 = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Posuere sollicitudin aliquam ultrices sagittis orci a. Ac turpis egestas sed tempus urna et pharetra pharetra massa. Nunc congue nisi vitae suscipit tellus mauris a. Viverra justo nec ultrices dui sapien eget mi. Nibh mauris cursus mattis molestie a iaculis at erat pellentesque. Sed ullamcorper morbi tincidunt ornare massa eget egestas purus. Porta non pulvinar neque laoreet. Urna neque viverra justo nec. Ante in nibh mauris cursus mattis molestie. Mollis aliquam ut porttitor leo a diam sollicitudin tempor id. Lectus vestibulum mattis ullamcorper velit sed ullamcorper morbi tincidunt ornare. Ultrices tincidunt arcu non sodales neque sodales. Sed egestas egestas fringilla phasellus faucibus scelerisque eleifend donec pretium. Lacus luctus accumsan tortor posuere ac ut.\n\nEgestas purus viverra accumsan in nisl nisi scelerisque eu. Duis convallis convallis tellus id interdum velit laoreet. Urna et pharetra pharetra massa. Pellentesque dignissim enim sit amet venenatis urna cursus. Id interdum velit laoreet id. Id eu nisl nunc mi ipsum. Vel quam elementum pulvinar etiam non quam. In hendrerit gravida rutrum quisque non tellus orci ac. Eu turpis egestas pretium aenean. Facilisis gravida neque convallis a cras semper auctor neque. At lectus urna duis convallis convallis tellus id interdum. Varius quam quisque id diam vel quam elementum pulvinar etiam. Ac felis donec et odio pellentesque diam volutpat commodo sed. Lacinia at quis risus sed vulputate odio ut enim. Convallis tellus id interdum velit laoreet id donec ultrices tincidunt. Amet purus gravida quis blandit turpis cursus. Blandit cursus risus at ultrices mi. Aliquam id diam maecenas ultricies. Sed augue lacus viverra vitae congue eu consequat ac.\n\nAliquam id diam maecenas ultricies mi eget mauris pharetra. Nec feugiat in fermentum posuere urna nec tincidunt praesent semper. Cursus metus aliquam eleifend mi in. Nam libero justo laoreet sit amet cursus sit amet dictum. Feugiat in ante metus dictum at tempor commodo ullamcorper. Arcu dictum varius duis at consectetur. Rutrum quisque non tellus orci ac. Posuere urna nec tincidunt praesent semper feugiat. Odio eu feugiat pretium nibh. Adipiscing enim eu turpis egestas pretium aenean pharetra. Arcu ac tortor dignissim convallis aenean et tortor. Nullam ac tortor vitae purus faucibus ornare suspendisse. Quis hendrerit dolor magna eget. Adipiscing enim eu turpis egestas pretium aenean pharetra magna ac.\nLaoreet id donec ultrices tincidunt arcu non sodales. Et malesuada fames ac turpis egestas maecenas pharetra convallis. Nisl condimentum id venenatis a condimentum. Sed viverra tellus in hac habitasse platea. Donec enim diam vulputate ut pharetra sit amet. Neque gravida in fermentum et. Mattis rhoncus urna neque viverra justo nec ultrices dui sapien. Nisi est sit amet facilisis magna etiam tempor orci. Sollicitudin tempor id eu nisl nunc mi ipsum. Massa sed elementum tempus egestas sed sed. Donec adipiscing tristique risus nec feugiat in fermentum posuere. Aliquet risus feugiat in ante metus dictum at tempor commodo. Sociis natoque penatibus et magnis dis parturient montes nascetur ridiculus. Mauris nunc congue nisi vitae suscipit tellus mauris a. Dignissim convallis aenean et tortor at risus viverra adipiscing at. Lacus sed viverra tellus in hac habitasse platea dictumst. Sit amet massa vitae tortor condimentum lacinia quis vel eros.\n\nAliquet enim tortor at auctor urna nunc id cursus metus. Malesuada fames ac turpis egestas. Egestas diam in arcu cursus euismod quis viverra nibh cras. Sem nulla pharetra diam sit amet nisl suscipit adipiscing bibendum. Suscipit tellus mauris a diam maecenas sed enim. Et tortor consequat id porta nibh venenatis cras. Risus feugiat in ante metus dictum. Imperdiet proin fermentum leo vel orci porta non pulvinar neque. Lorem dolor sed viverra ipsum nunc aliquet. Facilisis magna etiam tempor orci. Odio ut sem nulla pharetra diam sit amet. Eu augue ut lectus arcu. Molestie ac feugiat sed lectus vestibulum mattis ullamcorper velit. Aliquam sem et tortor consequat id porta. Sit amet consectetur adipiscing elit pellentesque habitant morbi. Velit ut tortor pretium viverra suspendisse potenti nullam. Consequat semper viverra nam libero justo.";

#endif
#undef ELEMENT_COUNT
#undef LOTS_OF_TEXT
