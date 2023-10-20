// common.h

// macros:
//  COMMON_IMPLEMENTATION
//  NO_STDLIB
//  NO_STDIO
//  NO_SIMD
//  NO_ASSERT
//  ASSERT
//  NPROC = 1
//  CACHELINESIZE = 64
//  BITS
//  MAX_PATH_LENGTH = 512
//  RANDOM_MAX
//  USE_STB_SPRINTF
//  SPRINTF_BUFFER_SIZE

#ifndef _COMMON_H
#define _COMMON_H

#include <stdint.h>
#include <stddef.h> // size_t
#include <stdarg.h> // va_list, va_arg

#if UINTPTR_MAX == 0xffffffffffffffff
  #define BITS 64
#elif UINTPTR_MAX == 0xffffffff
  #define BITS 32
#else
  #error "unsupported architecture"
#endif

// common type definitions
#if BITS == 64
  typedef double f64;
  typedef int64_t i64;
  typedef uint64_t u64;
#endif
typedef float f32;
typedef int32_t i32;
typedef uint32_t u32;
typedef int16_t i16;
typedef uint16_t u16;
typedef int8_t i8;
typedef uint8_t u8;

#ifndef bool
  typedef char bool;
#endif

#ifndef true
  #define true 1
#endif

#ifndef false
  #define false 0
#endif

#ifndef NPROC
  #define NPROC 1
#endif

#ifndef CACHELINESIZE
  #define CACHELINESIZE 64
#endif

#if __APPLE__ || __MACH__
  #define TARGET_APPLE
#elif __linux__
  #define TARGET_LINUX
#elif _WIN32 || _WIN64
  #define TARGET_WINDOWS
  #include <windows.h>
#elif defined(TARGET_WASM)
  #define ssize_t intmax_t
#else
  #error "unsupported target"
#endif

#ifndef MAX_PATH_LENGTH
  #define MAX_PATH_LENGTH 512
#endif
#define ALIGN(N, ALIGNMENT) ((N % ALIGNMENT) ? (N + ALIGNMENT - (N % ALIGNMENT)) : N)
#define LENGTH(ARR) (sizeof(ARR) / sizeof(ARR[0]))
#define return_defer(value) do { result = (value); goto defer; } while (0)
#define break_if(cond) if ((cond)) { break; }
#define continue_if(cond) if ((cond)) { continue; }
#define return_if(cond, ...) if ((cond)) { return __VA_ARGS__; }
typedef enum { Ok = 0, Error } Result;

#define KB(n) (n * 1024)
#define MB(n) (KB(n * 1024))
#define GB(n) (MB(n * 1024))

#ifndef RANDOM_MAX
  #define RANDOM_MAX (size_t)((~0-1) >> 1)
#endif

#ifdef NO_STDLIB
  void* memset(void* p, i32 c, size_t n);
  void* memcpy(void* dest, const void* src, size_t n);
  size_t strlen(const char* s);
  size_t strnlen(const char* s, size_t maxlen);
  i32 strcmp(const char* s1, const char* s2);
  i32 strncmp(const char* s1, const char* s2, size_t n);
  char* strcpy(char* dest, const char* src);
  char* strncpy(char* dest, const char* src, size_t n);
#else
  #include <stdlib.h>
  #include <string.h> // memset, memcpy
#endif

#ifdef NO_STDIO
  i32 printf(const char* fmt, ...);
  i32 dprintf(i32 fd, const char* fmt, ...);
  i32 sprintf(char* str, const char* fmt, ...);
  i32 snprintf(char* str, size_t size, const char* fmt, ...);

  i32 vprintf(const char* fmt, va_list argp);
  i32 vdprintf(i32 fd, const char* fmt, va_list argp);
  i32 vsprintf(char* str, const char* fmt, va_list argp);
  i32 vsnprintf(char* str, size_t size, const char* fmt, va_list argp);
#else
  #include <stdio.h>
#endif

#if defined(TARGET_LINUX) || defined(TARGET_APPLE)
  #include <unistd.h> // read, write
#elif defined(TARGET_WINDOWS)
  #include <io.h>
  #define read _read
  #define write _write
#elif defined(TARGET_WASM)
  extern i32 write(i32 fd, const void* buf, size_t size);
#endif

#ifdef TARGET_WASM
    #define DEBUG_BREAK
#else
  #if _MSC_VER
    #define DEBUG_BREAK __debugbreak()
  #elif defined(__has_builtin)
    #if __has_builtin(__builtin_debugtrap)
      #define DEBUG_BREAK __builtin_debugtrap()
    #elif __has_builtin(__builtin_trap)
      #define DEBUG_BREAK __builtin_trap()
    #endif
  #endif
#endif

#ifndef DEBUG_BREAK
  #include <signal.h>
  #if defined(SIGTRAP)
    #define DEBUG_BREAK raise(SIGTRAP)
  #else
    #define DEBUG_BREAK raise(SIGABRT)
  #endif
#endif

#ifndef __FUNCTION_NAME__
  #ifdef TARGET_WINDOWS
    #define __FUNCTION_NAME__ __FUNCTION__
  #else
    #define __FUNCTION_NAME__ __func__
  #endif
#endif

#ifndef STDIN_FILENO
  #if defined(TARGET_WINDOWS) && !defined(NO_STDIO)
    #define STDIN_FILENO _fileno(stdin)
  #else
    #define STDIN_FILENO 0
  #endif
#endif

#ifndef STDOUT_FILENO
  #if defined(TARGET_WINDOWS) && !defined(NO_STDIO)
    #define STDOUT_FILENO _fileno(stdout)
  #else
    #define STDOUT_FILENO 1
  #endif
#endif

#ifndef STDERR_FILENO
  #if defined(TARGET_WINDOWS) && !defined(NO_STDIO)
    #define STDERR_FILENO _fileno(stderr)
  #else
    #define STDERR_FILENO 2
  #endif
#endif

#ifndef EXIT_SUCCESS
  #define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
  #define EXIT_FAILURE 1
#endif

#ifndef NO_ASSERT
  #ifdef ASSERT
    // use custom assert macro
  #else
    #define ASSERT(...) \
      do { \
        if (!(__VA_ARGS__)) { \
          report_assert_failure(STDERR_FILENO, __FILE__, __LINE__, __FUNCTION_NAME__, #__VA_ARGS__); \
          DEBUG_BREAK; \
        } \
      } while (0)
  #endif
#else
  #define ASSERT(...)
#endif

#define NOT_IMPLEMENTED() ASSERT(!"not implemented")

void report_assert_failure(i32 fd, const char* filename, size_t line, const char* function_name, const char* message);
i32 is_terminal(i32 fd);
#ifdef TARGET_WINDOWS
bool enable_vt100_mode(void);
#else
#define enable_vt100_mode(...) 1
#endif

#endif // _COMMON_H

#ifdef COMMON_IMPLEMENTATION

#ifdef NO_STDLIB

void* memset(void* p, i32 c, size_t n) {
  u8* it = (u8*)p;
  for (size_t i = 0; i < n; ++i) {
    *it++ = c;
  }
  return p;
}

void* memcpy(void* dest, const void* src, size_t n) {
  u8* dest_it = (u8*)dest;
  const u8* src_it = (const u8*)src;
  for (size_t i = 0; i < n; ++i) {
    *dest_it++ = *src_it++;
  }
  return dest;
}

size_t strlen(const char* s) {
  size_t n = 0;
  for (; *s++ != 0; ++n);
  return n;
}

size_t strnlen(const char* s, size_t maxlen) {
  size_t n = 0;
  for (; *s++ != 0 && n < maxlen; ++n);
  return n;
}

i32 strcmp(const char* s1, const char* s2) {
  for (; *s1 && *s1 == *s2; ++s1, ++s2);
  return *(u8*)s1 - *(u8*)s2;
}

i32 strncmp(const char* s1, const char* s2, size_t n) {
  size_t i = 0;
  for (; *s1 && *s1 == *s2 && i < n; ++s1, ++s2, ++i);
  if (i == n) {
    return 0;
  }
  return *(u8*)s1 - *(u8*)s2;
}

char* strcpy(char* dest, const char* src) {
  char* result = dest;
  for (;; ++dest, ++src) {
    *dest = *src;
    if (*dest == '\0') {
      return result;
    }
  }
  return NULL;
}

char* strncpy(char* dest, const char* src, size_t n) {
  size_t i = 0;
  for (; i < n && src[i] != '\0'; ++i) {
    dest[i] = src[i];
  }
  for (; i < n; ++i) {
    dest[i] = '\0';
  }
  return dest;
}

#endif // NO_STDLIB

#ifdef NO_STDIO

#ifdef USE_STB_SPRINTF

#ifndef SPRINTF_BUFFER_SIZE
  #define SPRINTF_BUFFER_SIZE 4096
#endif

static char sprintf_buffer[SPRINTF_BUFFER_SIZE] = {0};

i32 printf(const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  size_t n = vprintf(fmt, argp);
  va_end(argp);
  return n;
}

i32 dprintf(i32 fd,  const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  size_t n = vdprintf(fd, fmt, argp);
  va_end(argp);
  return n;
}

i32 sprintf(char* str, const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  size_t n = stbsp_vsprintf(str, fmt, argp);
  va_end(argp);
  return n;
}

i32 snprintf(char* str, size_t size, const char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  size_t n = stbsp_vsnprintf(str, size, fmt, argp);
  va_end(argp);
  return n;
}

i32 vprintf(const char* fmt, va_list argp) {
  return vdprintf(STDOUT_FILENO, fmt, argp);
}

i32 vdprintf(i32 fd, const char* fmt, va_list argp) {
  size_t n = stbsp_vsnprintf(sprintf_buffer, SPRINTF_BUFFER_SIZE, fmt, argp);
  write(fd, sprintf_buffer, n);
  return n;
}

i32 vsprintf(char* str, const char* fmt, va_list argp) {
  return stbsp_vsprintf(str, fmt, argp);
}

i32 vsnprintf(char* str, size_t size, const char* fmt, va_list argp) {
  return stbsp_vsnprintf(str, size, fmt, argp);
}

#endif // USE_STB_SPRINTF

#endif // NO_STDIO

void report_assert_failure(i32 fd, const char* filename, size_t line, const char* function_name, const char* message) {
  dprintf(fd, "[assert-failed]: %s:%zu %s(): %s\n", filename, line, function_name, message);
}

i32 is_terminal(i32 fd) {
#if defined(TARGET_APPLE) || defined(TARGET_LINUX)
  return isatty(fd);
#elif defined(TARGET_WINDOWS)
  return _isatty(fd);
#else
  return 0;
#endif
}

#ifdef TARGET_WINDOWS
bool enable_vt100_mode(void) {
  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD console_mode = 0;
  GetConsoleMode(handle, &console_mode);
  console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  console_mode |= DISABLE_NEWLINE_AUTO_RETURN;
  return SetConsoleMode(handle, console_mode) != 0;
}
#endif

#endif // COMMON_IMPLEMENTATION

#undef COMMON_IMPLEMENTATION
