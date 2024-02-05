// platform.c

#ifdef TARGET_ANDROID

#else

void Vibrate(f32 seconds) { (void)seconds; }

void StartAccelerometerListening(void) {}

void StopAccelerometerListening(void) {}

Vector3 GetAccelerometerAxis(void) { return (Vector3) {0, 0, 0}; }

f32 GetAccelerometerX(void) { return 0; }

f32 GetAccelerometerY(void) { return 0; }

f32 GetAccelerometerZ(void) { return 0; }

void ShowSoftKeyboard(void) {}

void HideSoftKeyboard(void) {}

bool IsSoftKeyboardActive(void) { return false; }

i32 GetLastSoftKeyCode(void) { return 0; }

u16 GetLastSoftKeyLabel(void) { return 0; }

i32 GetLastSoftKeyUnicode(void) { return 0; }

char GetLastSoftKeyChar(void) { return '\0'; }

void ClearLastSoftKey(void) {}

void SoftKeyboardEditText(char* text, u32 size) { (void)text; (void)size; }

void KeepScreenOn(bool keep_on) { (void)keep_on; }

#endif
