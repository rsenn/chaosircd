#include "iob.h"
#include "str.h"

int iob_adds(io_batch *b, const char *s) {
  return iob_addbuf(b, s, str_len(s));
}
