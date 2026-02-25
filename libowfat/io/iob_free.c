#include "iob_internal.h"
#include <stdlib.h>

void iob_free(io_batch *b) {
  iob_reset(b);
  free(b);
}
