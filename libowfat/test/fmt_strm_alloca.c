#include "byte.h"
#include "fmt.h"
#include <assert.h>
#include <stdlib.h>

int main() {
  char *c = fmt_strm_alloca("foo", " bar", "\n");
  assert(byte_equal(c, sizeof("foo bar\n"), "foo bar\n"));
}
