// buffer.c

Buffer buffer_new(size_t size) {
  Buffer buffer = (Buffer) {
    .data = memory_alloc(size),
    .count = 0,
    .size = size,
  };
  ASSERT(buffer.data != NULL);
  return buffer;
}

void buffer_free(Buffer* buffer) {
  memory_free(buffer->data);
  buffer->data = NULL;
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
