#include "tai.h"
#include <time.h>

void tai_now(struct tai *t) { tai_unix(t, time((time_t *)0)); }
