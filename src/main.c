// main.c

#include "mix.c"

#ifdef TARGET_WINDOWS

i32 CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, i32 cmd_show) {
  (void)instance;
  (void)prev_instance;
  (void)cmd_line;
  (void)cmd_show;
  return mix_main(0, NULL);
}

#else

i32 main(i32 argc, char** argv) {
  return mix_main(argc, argv);
}

#endif
