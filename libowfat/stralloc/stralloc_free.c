#include "stralloc.h"
#include <stdlib.h>

void stralloc_free(stralloc *sa) {
  if (sa->s)
    free(sa->s);
  sa->s = 0;
}
