// buffer.c

#define BUFFER_INIT_SIZE 16

Buffer buffer_new(size_t size) {
  Buffer buffer = (Buffer) {
    .data = memory_calloc(size, 1),
    .count = 0,
    .size = size,
  };
  ASSERT(buffer.data != NULL || size == 0);
  return buffer;
}

Buffer buffer_new_from_str(const char* str) {
  size_t length = strlen(str);
  Buffer buffer = (Buffer) {
    .data = memory_alloc(length),
    .count = length,
    .size = length,
  };
  memcpy(buffer.data, str, length);
  return buffer;
}

Buffer buffer_new_from_fmt(size_t size, const char* fmt, ...) {
  Buffer buffer = buffer_new(size);

  va_list argp;
  va_start(argp, fmt);
  buffer.count = stb_vsnprintf((char*)buffer.data, size, fmt, argp);
  va_end(argp);

  return buffer;
}

i32 buffer_to_int(Buffer* buffer) {
  i32 value = 0;
  if (buffer->count > 0) {
    sscanf((char*)buffer->data, "%d", &value);
  }
  return value;
}

f32 buffer_to_float(Buffer* buffer) {
  f32 value = 0.0f;
  if (buffer->count > 0) {
    sscanf((char*)buffer->data, "%f", &value);
  }
  return value;
}

void buffer_from_fmt(Buffer* buffer, size_t size, const char* fmt, ...) {
  if (size <= buffer->size) {
    va_list argp;
    va_start(argp, fmt);
    buffer->count = stb_vsnprintf((char*)buffer->data, size, fmt, argp);
    va_end(argp);
    return;
  }
  buffer_free(buffer);
  *buffer = buffer_new(size);
  va_list argp;
  va_start(argp, fmt);
  buffer->count = stb_vsnprintf((char*)buffer->data, size, fmt, argp);
  va_end(argp);
}

void buffer_reset(Buffer* buffer) {
  buffer->count = 0;
}

void buffer_append(Buffer* buffer, u8 byte) {
  if (buffer->count + sizeof(byte) >= buffer->size - 1) {
    size_t new_size = buffer->size * 2;
    if (!new_size) {
      new_size = BUFFER_INIT_SIZE;
    }
    buffer->data = memory_realloc(buffer->data, new_size);
    buffer->size = new_size;
    memset(&buffer->data[buffer->count], 0, buffer->size - buffer->count);
  }
  buffer->data[buffer->count++] = byte;
}

void buffer_insert(Buffer* buffer, u8 byte, size_t index) {
  buffer_append(buffer, 0);
  memmove(&buffer->data[index + 1], &buffer->data[index], buffer->count - index + 1);
  buffer->data[index] = byte;
}

void buffer_erase(Buffer* buffer, size_t index) {
  if (index < buffer->count && buffer->count > 0) {
    memmove(&buffer->data[index], &buffer->data[index + 1], (buffer->count - index) - 1);
    buffer->data[--buffer->count] = 0;
    return;
  }
  buffer->data[0] = 0;
  buffer->count = 0;
}

void buffer_free(Buffer* buffer) {
  if (buffer->data) {
    memory_free(buffer->data);
    buffer->data = NULL;
  }
  buffer->count = 0;
  buffer->size = 0;
}

Buffer buffer_new_from_fd(i32 fd) {
  size_t file_size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  Buffer buffer = buffer_new(file_size);
  if (read(fd, buffer.data, file_size) == (i32)file_size) {
    buffer.count = file_size;
    buffer.size = file_size;
  }
  else {
    buffer_free(&buffer);
  }
  return buffer;
}

Buffer buffer_new_from_file(const char* path) {
#ifdef TARGET_ANDROID
  i32 size = 0;
  u8* data = LoadFileData(path, &size);
  if (data && size >= 0) {
    return (Buffer) {
      .data = data,
      .count = (u32)size,
      .size = (u32)size,
    };
  }
  return (Buffer) {
    .data = NULL,
    .count = 0,
    .size = 0,
  };
#else
  i32 fd = open(path, O_RDONLY);
  if (fd < 0) {
    return (Buffer) {
      .data = NULL,
      .count = 0,
      .size = 0,
    };
  }
  return buffer_new_from_fd(fd);
#endif
}
