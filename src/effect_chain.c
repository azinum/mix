// effect_chain.c

void effect_chain_ui_new(struct Mix* mix, Element* container) {
  (void)mix;
  container->x_padding = UI_BORDER_THICKNESS;
  container->y_padding = UI_BORDER_THICKNESS;
}
