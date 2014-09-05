#include "libchaos/mem.h"
#include "libchaos/str.h"
#include "libchaos/log.h"
#include "libchaos/io.h"
#include "libchaos/timer.h"

#include <time.h>

int main()
{
  char buffer[2048];

  str_init();
  log_init(STDOUT_FILENO, LOG_ALL, L_status);
  io_init_except(STDOUT_FILENO, STDOUT_FILENO, STDOUT_FILENO);
  mem_init();
  timer_init();

 uint64_t t = time(NULL)  * 1000llu;
  str_snprintf(buffer, sizeof(buffer), "%i %T", 1337, &t);

	log(LOG_ALL, L_status, "str_nprintf() result: %s", buffer);

	int r = str_cmp("test", "test");
	log(LOG_ALL, L_status, "str_cmp(test,test) result: %i", r);

	int r2 = str_cmp("test", "blah");
	log(LOG_ALL, L_status, "str_cmp(test,blah) result: %i", r2);


  timer_shutdown();
  mem_shutdown();
  log_shutdown();
  io_shutdown();
  str_shutdown();

  return 0;
}

