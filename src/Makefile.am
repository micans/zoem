# $Id: Makefile.am,v 1.21 2006-02-14 15:50:07 flux Exp $

AM_CPPFLAGS = -I$(top_srcdir)

## noinst_PROGRAMS = zoem
bin_PROGRAMS = zoem
zoem_SOURCES = ops-grape.c ops-xtag.c op-inspect.c op-format.c \
ops-env.c ops-constant.c curly.c digest.c entry.c \
iface.c source.c sink.c filter.c key.c dict.c ops.c parse.c read.c \
segment.c util.c zoem.c version.c

# zoem_LDFLAGS = -lm # -lunwind -ltcmalloc

LDADD = -ltingea -lm

EXTRA_DIST = curly.h digest.h entry.h parse.h segment.h key.h dict.h \
source.h sink.h read.h util.h filter.h iface.h zlimits.h version.h \
ops.h ops-xtag.h ops-grape.h ops-env.h ops-constant.h \
op-inspect.h op-format.h


