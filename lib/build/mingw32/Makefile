PACKAGE = libchaos
VERSION = 0

prefix = /opt/mingw64
srcdir = ../..
includedir = $(srcdir)/include
pkgincludedir = $(includedir)/libchaos

configheader = $(pkgincludedir)/config.h

CP = cp -f
#CP = copy

CC = gcc
CFLAGS = -g -O2 -Wall
LDFLAGS = #-shared -Wl,--export-all-symbols
CPPFLAGS = -I$(srcdir)/include -I. -I$(prefix)/include -I$(prefix)/include/libowfat 
DEFS = -DWIN32 -DBUILD_LIBCHAOS -DDLLEXT=\".dll\" -DPLUGINDIR=\"modules\" -DHAVE_CONFIG_H=1
LIBS = -lowfat -lssl -lcrypto -lws2_32

EXEEXT = .exe

vpath ../../src ../../servauth
VPATH = ../../src:../../servauth

PROGRAMS = servauth$(EXEEXT)
LIBRARY = $(PACKAGE)-$(VERSION).dll
TESTS = cfgtest$(EXEEXT) dbtest$(EXEEXT) giftest$(EXEEXT) graphtest$(EXEEXT) imagetest$(EXEEXT) strtest$(EXEEXT) ttftest$(EXEEXT)

all: $(configheader) $(LIBRARY) $(PROGRAMS) #$(TESTS)

$(configheader): config.h
	$(CP) $< $(pkgincludedir)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEFS) -c $<

$(LIBRARY): cfg.o child.o connect.o db.o divdi3.o dlfcn_win32.o dlink.o filter.o gif.o graph.o hook.o htmlp.o httpc.o image.o ini.o io.o listen.o log.o mem.o mfile.o module.o net.o queue.o sauth.o ssl.o str.o syscall.o timer.o ttf.o
	$(CC) -shared -Wl,--export-all-symbols -Wl,--out-implib,libchaos.dll.a $(CFLAGS) -o $@ $^ $(LIBS)

servauth$(EXEEXT): CPPFLAGS += -I../..
servauth$(EXEEXT): LIBS += -L. -lchaos
servauth$(EXEEXT): LIBS += -ldnsapi
servauth$(EXEEXT): auth.o cache.o commands.o control.o dns.o proxy.o query.o servauth.o userdb.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

