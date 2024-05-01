// test_ui.c

Element test_ui_new(Mix* mix) {
  f32 seed = get_time();
  random_init((Random)seed);
  Element test_ui = ui_container(NULL);
  test_ui.sizing = SIZING_PERCENT(100, 100);
  test_ui.scissor = false;
  test_ui.border = false;
  test_ui.background = false;
  test_ui.placement = PLACEMENT_BLOCK;
  test_ui.x_padding = 16;
  test_ui.y_padding = 16;

  return test_ui;
}
