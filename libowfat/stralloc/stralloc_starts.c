#include "stralloc.h"
#include "byte.h"
#include "str.h"

extern int stralloc_starts(stralloc *sa,const char *in) {
  register size_t len=strlen(in);
  return (len<=sa->len && !byte_diff(sa->s,len,in));
}

