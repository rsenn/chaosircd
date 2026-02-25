#include "buffer.h"
#include "stralloc.h"

int buffer_putsaflush(buffer *b, stralloc *sa) {
  return buffer_putflush(b, sa->s, sa->len);
}
