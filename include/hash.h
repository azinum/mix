// hash.h

#ifndef _HASH_H
#define _HASH_H

#ifndef Hash
  typedef size_t Hash;
#endif

Hash hash_djb2(const u8* data, const size_t size);
Hash hash_sdbm(const u8* data, const size_t size);
Hash hash_basic(const u8* data, const size_t size);

#endif // _HASH_H
