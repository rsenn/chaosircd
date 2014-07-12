# $Id: Makefile.in,v 1.1 2006/09/27 10:10:38 roman Exp $
# ===========================================================================
#
# GNU Makefile for ashish m4 macros
#
# relative directories
# ---------------------------------------------------------------------------
srcdir               = .
top_srcdir           = ..
builddir             = .
top_builddir         = ..
thisdir              = m4/

# m4 macros
# ---------------------------------------------------------------------------
MACROS = $(wildcard *.m4)

# targets
# ---------------------------------------------------------------------------
all: Makefile 
install:

# include config.mak (makefile helper) if there
# ---------------------------------------------------------------------------
include $(top_builddir)/config.mk

# files to be cleaned
# ---------------------------------------------------------------------------
CLEANFILES           = *~
DISTCLEANFILES       = Makefile 
MAINTAINERCLEANFILES = 

# files that go into distribution tarballs
# ---------------------------------------------------------------------------
FILES                = Makefile.in $(MACROS)
DISTFILES            = $(FILES)
DEVFILES             = CVS $(FILES)

include $(top_builddir)/build.mk
