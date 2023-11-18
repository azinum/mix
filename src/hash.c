// hash.c

inline Hash hash_djb2(const u8* data, const size_t size) {
  Hash h = 5381;
  for (size_t i = 0; i < size; ++i) {
    h = ((h << 5) + h) + data[i];
  }
  return h;
}

inline Hash hash_sdbm(const u8* data, const size_t size) {
  Hash h = 0;
  for (size_t i = 0; i < size; ++i) {
    h = data[i] + (h << 6) + (h << 16) - h;
  }
  return h; 
}

inline Hash hash_basic(const u8* data, const size_t size) {
  ASSERT(!(size % (sizeof(Hash) * 2)));
#ifdef USE_SIMD
  __m128i h = _mm_set_epi64x(5381, 7253);
  const __m128i shift = _mm_set1_epi64x(7);
  const __m128i* it = (__m128i*)data;
  size_t chunk_size = sizeof(h);
  for (size_t i = 0; i < size; i += chunk_size, it += 1) {
    h = _mm_add_epi64(
      h,
      _mm_add_epi64(
        _mm_sll_epi64(h, shift),
        *it
      )
    );
  }
  Hash* hash = (Hash*)&h;
  return hash[0] + hash[1];
#endif
  {
    Hash h[] = {
      7253,
      5381,
    };
    const size_t chunk_size = sizeof(h);
    const Hash* it = (Hash*)data;
    for (size_t i = 0; i < size; i += chunk_size, it += 2) {
      h[0] += (h[0] << 7) + it[0];
      h[1] += (h[1] << 7) + it[1];
    }
    return h[0] + h[1];
  }
}

