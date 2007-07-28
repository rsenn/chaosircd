all-after: servauth.exe tests

.SUFFIXES: .c .o .h

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $^

servauth.exe: servauth/auth.o servauth/cache.o servauth/commands.o servauth/control.o servauth/dns.o servauth/proxy.o servauth/query.o servauth/servauth.o
	$(CC) $(LDFLAGS) -o $@ $^ chaos.dll

tests: 

servauth/auth.o: servauth/auth.c include/libchaos/defs.h include/libchaos/io.h \
  include/libchaos/defs.h include/libchaos/syscall.h \
  include/libchaos/queue.h include/libchaos/dlink.h \
  include/libchaos/timer.h include/libchaos/log.h include/libchaos/mem.h \
  include/libchaos/net.h include/libchaos/io.h include/libchaos/str.h \
  include/libchaos/mem.h servauth/auth.h        
servauth/cache.o: servauth/cache.c include/libchaos/defs.h include/libchaos/net.h \
  include/libchaos/io.h include/libchaos/syscall.h \
  include/libchaos/queue.h include/libchaos/dlink.h \
  include/libchaos/str.h include/libchaos/mem.h servauth/cache.h \
  servauth/control.h include/libchaos/queue.h servauth/servauth.h
servauth/commands.o: servauth/commands.c include/libchaos/defs.h \
  include/libchaos/io.h include/libchaos/syscall.h \
  include/libchaos/queue.h include/libchaos/dlink.h \
  include/libchaos/timer.h include/libchaos/queue.h \
  include/libchaos/log.h include/libchaos/net.h include/libchaos/str.h \
  include/libchaos/mem.h servauth/commands.h servauth/control.h \
  servauth/servauth.h servauth/cache.h servauth/dns.h servauth/auth.h \
  servauth/proxy.h servauth/query.h
servauth/control.o: servauth/control.c include/libchaos/defs.h \
  include/libchaos/io.h include/libchaos/syscall.h \
  include/libchaos/queue.h include/libchaos/dlink.h \
  include/libchaos/syscall.h include/libchaos/queue.h \
  include/libchaos/log.h include/libchaos/net.h include/libchaos/str.h \
  include/libchaos/mem.h servauth/servauth.h servauth/control.h \
  servauth/commands.h
servauth/dns.o: servauth/dns.c include/libchaos/defs.h include/libchaos/io.h \
  include/libchaos/syscall.h include/libchaos/queue.h \
  include/libchaos/dlink.h include/libchaos/syscall.h \
  include/libchaos/timer.h include/libchaos/log.h include/libchaos/mem.h \
  include/libchaos/net.h include/libchaos/str.h servauth/dns.h \
  servauth/control.h include/libchaos/queue.h servauth/servauth.h
servauth/proxy.o: servauth/proxy.c include/libchaos/defs.h include/libchaos/io.h \
  include/libchaos/syscall.h include/libchaos/queue.h \
  include/libchaos/dlink.h include/libchaos/timer.h \
  include/libchaos/log.h include/libchaos/mem.h include/libchaos/net.h \
  include/libchaos/str.h servauth/proxy.h
servauth/query.o: servauth/query.c include/libchaos/defs.h include/libchaos/io.h \
  include/libchaos/syscall.h include/libchaos/queue.h \
  include/libchaos/dlink.h include/libchaos/timer.h \
  include/libchaos/log.h include/libchaos/mem.h include/libchaos/net.h \
  include/libchaos/str.h servauth/control.h include/libchaos/queue.h \
  servauth/servauth.h servauth/cache.h servauth/auth.h servauth/dns.h \
  servauth/proxy.h servauth/query.h
servauth/servauth.o: servauth/servauth.c include/libchaos/defs.h \
  include/libchaos/connect.h include/libchaos/io.h \
  include/libchaos/syscall.h include/libchaos/queue.h \
  include/libchaos/dlink.h include/libchaos/timer.h \
  include/libchaos/net.h include/libchaos/ssl.h include/libchaos/log.h \
  include/libchaos/dlink.h include/libchaos/queue.h \
  include/libchaos/timer.h include/libchaos/log.h include/libchaos/mem.h \
  include/libchaos/str.h include/libchaos/io.h include/libchaos/syscall.h \
  servauth/control.h servauth/servauth.h servauth/cache.h servauth/auth.h \
  servauth/dns.h servauth/proxy.h servauth/query.h
