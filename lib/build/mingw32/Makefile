PACKAGE = libchaos
VERSION = 0

srcdir = ../..

CC = gcc
CFLAGS = -g -O2 -Wall
LDFLAGS = -Wl,--export-all-symbols
CPPFLAGS = -I$(srcdir)/include -I.
DEFS = -DWIN32 -DBUILD_LIBCHAOS -DDLLEXT=\".dll\" -DPLUGINDIR=\"modules\" -DHAVE_CONFIG_H=1

EXEEXT = .exe

vpath ../.. .
VPATH = ../..:.

PROGRAMS = servauth$(EXEEXT)
LIBRARY = $(PACKAGE)-$(VERSION).dll
TESTS = cfgtest$(EXEEXT) dbtest$(EXEEXT) giftest$(EXEEXT) graphtest$(EXEEXT) imagetest$(EXEEXT) strtest$(EXEEXT) ttftest$(EXEEXT)

all: $(LIBRARY) $(PROGRAMS) $(TESTS)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEFS) -c $<

$(LIBRARY): cfg.o child.o connect.o db.o divdi3.o dlfcn_win32.o dlink.o filter.o gif.o graph.o hook.o htmlp.o httpc.o image.o ini.o io.o listen.o log.o mem.o mfile.o module.o net.o queue.o sauth.o ssl.o str.o syscall.o timer.o ttf.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

cfg.o: src/cfg.c
child.o: src/child.c
connect.o: src/connect.c
db.o: src/db.c
divdi3.o: src/divdi3.c
#dlfcn_darwin.o: src/dlfcn_darwin.c
dlfcn_win32.o: src/dlfcn_win32.c
dlink.o: src/dlink.c
filter.o: src/filter.c
gif.o: src/gif.c
graph.o: src/graph.c
hook.o: src/hook.c
htmlp.o: src/htmlp.c
httpc.o: src/httpc.c
image.o: src/image.c
ini.o: src/ini.c
io.o: src/io.c
listen.o: src/listen.c
log.o: src/log.c
mem.o: src/mem.c
mfile.o: src/mfile.c
module.o: src/module.c
net.o: src/net.c
queue.o: src/queue.c
sauth.o: src/sauth.c
ssl.o: src/ssl.c
str.o: src/str.c
syscall.o: src/syscall.c
timer.o: src/timer.c
ttf.o: src/ttf.c
