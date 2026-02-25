#define NO_UINT64_MACROS
#include "uint32.h"
#include "uint64.h"

void uint64_unpack(const char *in, uint64 *out) { *out = uint64_read(in); }
