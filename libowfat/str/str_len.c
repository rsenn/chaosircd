#undef __dietlibc__
#include "str.h"

size_t strlen(const char* in) {
  register const char* t=in;
  for (;;) {
    if (!*t) break; ++t;
    if (!*t) break; ++t;
    if (!*t) break; ++t;
    if (!*t) break; ++t;
  }
  return t-in;
}
