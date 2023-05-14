## Process this file with automake to produce Makefile.in
## $Id: Makefile.am,v 1.20 2004/06/24 11:43:11 flux Exp $

SUBDIRS = . src doc examples shed

VERSION = @VERSION@

# AM_LDFLAGS = -lunwind -ltcmalloc
# AM_LDFLAGS = -lm

configure.ac: configure.ac.in VERSION
	shed/setversion

EXTRA_DIST = configure.ac.in VERSION LICENSE TODO_DESIGN DESIGN install-this-zoem.sh

