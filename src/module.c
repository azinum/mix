// module.c

#if defined(TARGET_LINUX) || defined(TARGET_OSX)

#include <dlfcn.h>

void* module_open(const char* path) {
  return dlopen(path, RTLD_LAZY);
}

void* module_symbol(void* handle, const char* name) {
  return dlsym(handle, name);
}

void module_close(void* handle) {
  if (handle) {
    dlclose(handle);
  }
}

#else

void* module_open(const char* path) {
  NOT_IMPLEMENTED();
  return NULL;
}

void* module_symbol(const void* handle, const char* name) {
  NOT_IMPLEMENTED();
  return NULL;
}

void module_close(void* handle) {
  NOT_IMPLEMENTED();
}

#endif
