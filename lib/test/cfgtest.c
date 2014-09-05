#include "mem.h"
#include "timer.h"
#include "log.h"
#include "io.h"
#include "cfg.h"

int main()
{
  log_init(STDOUT_FILENO, LOG_ALL, L_status);
  io_init_except(STDOUT_FILENO, STDOUT_FILENO, STDOUT_FILENO);
  mem_init();
  dlink_init();
  timer_init();
  cfg_init();



  cfg_shutdown();
  timer_shutdown();
  dlink_shutdown();
  mem_shutdown();
  log_shutdown();
  io_shutdown();

  return 0;
}

