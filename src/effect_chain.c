// effect_chain.c

void effect_chain_ui_new(struct Mix* mix, Element* container) {
  (void)mix;
  container->y_padding = FONT_SIZE;
  container->x_padding = FONT_SIZE * .5f;
}
