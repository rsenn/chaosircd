#include "array.h"
#include "str.h"

void array_cats0(array* to,const char* from) {
  array_catb(to,from,strlen(from)+1);
}
