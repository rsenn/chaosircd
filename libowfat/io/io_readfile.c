#define _FILE_OFFSET_BITS 64
#include "io_internal.h"
#include <fcntl.h>
#include <unistd.h>

int io_readfile(int64 *d, const char *s) {
  long fd = open(s, O_RDONLY);
  if (fd != -1) {
    *d = fd;
    return 1;
  }
  return 0;
}
