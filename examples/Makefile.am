## Process this file with automake to produce Makefile.in
# $Id: Makefile.am,v 1.15 2005/06/30 16:23:25 flux Exp $

docdir = $(datadir)/doc/$(PACKAGE)
docexamplesdir = $(docdir)/examples

docexamples_DATA = \
 README \
 8q.azm 8q.ozm \
 8q2.azm 8q2.ozm \
 power-of-2.azm   power-of-2.ozm \
 catch.azm        catch.ozm \
 fib.azm          \
 loop.azm         loop.ozm \
 meta.azm         meta.ozm \
 nest.azm         nest.ozm \
 reverse.azm      reverse.ozm \
 scope.azm        scope.ozm \
 special.azm      special.ozm \
 stress.azm       stress.reg  \
 stress2.azm      stress2.reg \
 tr.azm           tr.ozm \
 txt-table.azm    txt-table.ozm \
 xml.azm          xml.ozm

EXTRA_DIST = README Makefile $(docexamples_DATA)

SUFFIXES = .azm .ozm

.azm.ozm:
	../src/zoem -i $< -o $@

# fib.azm is for demonstration of interactive behaviour.
fib.ozm: fib.azm
	true

stress.ozm: stress.azm
	../src/zoem -i stress --system-honor --unsafe-silent --err-out -s get_it_on=1 -o - > stress.ozm

stress2.ozm: stress2.azm
	../src/zoem -i stress2 --system-honor --unsafe-silent --err-out -s get_it_on=1 -o - > stress2.ozm

power-of-2.ozm: power-of-2.azm
	../src/zoem -i power-of-2 --allow=ls

xml.ozm: xml.azm
	../src/zoem -i xml -tl 3 

sys1:
	zoem -i system -o - --allow=date

sys2:
	zoem -i system -o system.ozm --allow=date

sys3:
	zoem -i system -o - --allow=date > system.ozm

