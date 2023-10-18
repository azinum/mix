// module.h

#ifndef _MODULE_H
#define _MODULE_H

void* module_open(const char* path);
void* module_symbol(void* handle, const char* name);
void module_close(void* handle);

#endif // _MODULE_H
