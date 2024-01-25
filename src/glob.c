// glob.c

// * = match any string
// ? = match any character
bool glob(const char* pattern, const char* path) {
  for (;;) {
    if (*pattern == '*') {
      if (*(pattern+1) == *(path+1)) {
        pattern += 1;
      }
      path += 1;
    }
    else if (*pattern == '?') {
      pattern += 1;
      path += 1;
    }
    else if (*pattern == *path) {
      if (*pattern == 0) {
        return true;
      }
      path += 1;
      pattern += 1;
    }
    else {
      return false;
    }
  }
  return false;
}
