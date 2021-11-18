## Process this file with automake to produce Makefile.in
## $Id: Makefile.am,v 1.20 2004/06/24 11:43:11 flux Exp $

## all-local: stamp-util
## stamp-util:
## 	## do some magic to fiddle with util.  util is in cvs in ../util/ ,
## 	## not in the zoem tree itself.
## 	for i in $(top_srcdir)/../util/*.h $(top_srcdir)/../util/*.c; do \
##           $(INSTALL_DATA) $$i $(top_srcdir)/util; \
##         done
## 	touch stamp-util

maintainer-clean-local:
	-rm -f $(top_srcdir)/util/*.h $(top_srcdir)/util/*.c

SUBDIRS = . src doc examples shed

VERSION = @VERSION@

# AM_LDFLAGS = -lunwind -ltcmalloc
AM_LDFLAGS = -lm

configure.ac: configure.ac.in VERSION
	shed/setversion

EXTRA_DIST = configure.ac.in VERSION LICENSE TODO_DESIGN\
   DESIGN

