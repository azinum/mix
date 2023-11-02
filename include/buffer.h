// buffer.h

#ifndef _BUFFER_H
#define _BUFFER_H

typedef struct Buffer {
  u8* data;
  size_t count; // how many bytes are in use?
  size_t size;  // number of bytes allocated
} Buffer;

Buffer buffer_new(size_t size);
void buffer_free(Buffer* buffer);
Buffer buffer_new_from_fd(i32 fd);

#endif // _BUFFER_H
