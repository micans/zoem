## $Id: Makefile.am,v 1.41 2006-02-15 14:23:46 flux Exp $

TROFF = groff
ZOEM = ../src/zoem
COL = col
## perhaps could better run zoem as installed on system, so just
## say
##  ZOEM = zoem


## tidy is a HTML syntax checker and reformatter, available from
## http://www.w3.org/People/Raggett/tidy/ , by Dave Raggett <dsr@w3.org>
TIDY = tidy

docdir = $(datadir)/doc/$(PACKAGE)
docdocdir = $(docdir)/doc

azms = zum.azm zoem.azm

## buzz* should get installed in /usr/local/share/doc/zoem/doc/ .  NOT
## in doc/zoem/examples : html documents contain relative hyperlinks
## should azms get installed in /usr/local/share/doc/zoem/doc/ too?
docdoc_DATA = zum.html zoem.html zoem.ps $(azms) 
man_MANS = zoem.1

EXTRA_DIST = ecosphere.azm zoem.shared $(azms) \
      $(docdoc_DATA)  $(man_MANS) stamp.year stamp.month stamp.day stamp.stamp

SUFFIXES = .1 .7 .azm .html .ps .txt

.azm.1:
	$(ZOEM) -d roff -i $< -o $@
	$(ZOEM) -d roff -i $< -o $@

.azm.7:
	$(ZOEM) -d roff -i $< -o $@
	$(ZOEM) -d roff -i $< -o $@

.1.ps:
	$(TROFF) -man $< > $@

.1.txt:
	$(TROFF) -t -e -mandoc -Tascii $< | $(COL) -bx > $@

.7.ps:
	$(TROFF) -man $< > $@

.azm.html:
	$(ZOEM) -d html -i $< -o $@
	$(ZOEM) -d html -i $< -o $@
	- $(TIDY) -e $@

zum: zum.html

ZOEMSEARCHPATH=../mac:${ZOEMSEARCHPATH}

zum.html: zum.azm ecosphere.azm
	$(ZOEM) --allow=date:../src/zoem: --system-honor --d=html --i=$(VPATH)/zum --o=$@
	$(ZOEM) --allow=date:../src/zoem: --system-honor --d=html --i=$(VPATH)/zum --o=$@
	- $(TIDY) -e $@

zoem: zoem.1 zoem.html zoem.ps

index.html: webindex.azm ecosphere.azm
	zoem  -i webindex.azm -o index.html -d html
	- tidy -e index.html

