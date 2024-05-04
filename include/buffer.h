// buffer.h

#ifndef _BUFFER_H
#define _BUFFER_H

typedef struct Buffer {
  u8* data;
  size_t count; // how many bytes are in use?
  size_t size;  // number of bytes allocated
} Buffer;

Buffer buffer_new(size_t size);
Buffer buffer_new_from_str(const char* str);
Buffer buffer_new_from_fmt(size_t size, const char* fmt, ...);
i32 buffer_to_int(Buffer* buffer);
i16 buffer_to_int16(Buffer* buffer);
i8 buffer_to_int8(Buffer* buffer);
f32 buffer_to_float(Buffer* buffer);
void buffer_from_fmt(Buffer* buffer, size_t size, const char* fmt, ...);
void buffer_reset(Buffer* buffer);
void buffer_append(Buffer* buffer, u8 byte);
void buffer_insert(Buffer* buffer, u8 byte, size_t index);
void buffer_erase(Buffer* buffer, size_t index);
void buffer_free(Buffer* buffer);
Buffer buffer_new_from_fd(i32 fd);
Buffer buffer_new_from_file(const char* path);

#endif // _BUFFER_H
