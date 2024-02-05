// platform.h

#ifndef _PLATFORM_H
#define _PLATFORM_H

extern void Vibrate(f32 seconds);
extern void StartAccelerometerListening(void);
extern void StopAccelerometerListening(void);
extern Vector3 GetAccelerometerAxis(void);
extern f32 GetAccelerometerX(void);
extern f32 GetAccelerometerY(void);
extern f32 GetAccelerometerZ(void);
extern void ShowSoftKeyboard(void);
extern void HideSoftKeyboard(void);
extern bool IsSoftKeyboardActive(void);
extern i32 GetLastSoftKeyCode(void);
extern u16 GetLastSoftKeyLabel(void);
extern i32 GetLastSoftKeyUnicode(void);
extern char GetLastSoftKeyChar(void);
extern void ClearLastSoftKey(void);
extern void SoftKeyboardEditText(char* text, u32 size);
extern void KeepScreenOn(bool keep_on);

#endif // _PLATFORM_H
