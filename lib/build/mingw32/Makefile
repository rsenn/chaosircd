PACKAGE = libchaos
VERSION = 0

srcdir = ..\..
includedir = $(srcdir)\include
pkgincludedir = $(includedir)\libchaos

configheader = $(pkgincludedir)\config.h

CC = gcc
CFLAGS = -g -O2 -Wall
LDFLAGS = -Wl,--export-all-symbols
CPPFLAGS = -I$(srcdir)\include -I.
DEFS = -DWIN32 -DBUILD_LIBCHAOS -DDLLEXT=\".dll\" -DPLUGINDIR=\"modules\" -DHAVE_CONFIG_H=1

EXEEXT = .exe

vpath ..\..\src
VPATH = ..\..\src

PROGRAMS = servauth$(EXEEXT)
LIBRARY = $(PACKAGE)-$(VERSION).dll
TESTS = cfgtest$(EXEEXT) dbtest$(EXEEXT) giftest$(EXEEXT) graphtest$(EXEEXT) imagetest$(EXEEXT) strtest$(EXEEXT) ttftest$(EXEEXT)

all: $(configheader) $(LIBRARY) $(PROGRAMS) $(TESTS)

$(configheader): config.h
	copy $< $(pkgincludedir)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEFS) -c $<

$(LIBRARY): cfg.o child.o connect.o db.o divdi3.o dlfcn_win32.o dlink.o filter.o gif.o graph.o hook.o htmlp.o httpc.o image.o ini.o io.o listen.o log.o mem.o mfile.o module.o net.o queue.o sauth.o ssl.o str.o syscall.o timer.o ttf.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)
