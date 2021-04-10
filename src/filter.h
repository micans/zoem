/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_filter_h__
#define zoem_filter_h__


/*
 *    The filter manipulation code is a bit hackish, but works well.
 *    It's main shortcoming is the lack of documentation.
*/

#include <stdio.h>

#include "segment.h"
#include "util.h"

#include "tingea/ting.h"
#include "tingea/types.h"
#include "tingea/io.h"


typedef struct filter filter;


typedef mcxstatus  (*fltfnc)(filter* fd, mcxTing* txt, int offset, int length);
extern fltfnc flts[6];

filter* filterNew
(  FILE* fp
)  ;

void filterFree
(  filter* fd
)  ;


void yamSpecialSet
(  long c
,  const char* str
)  ;

void yamSpecialClean
(  void
)  ;

void mod_filter_init
(  int            n
)  ;

void mod_filter_exit
(  void
)  ;

void filterList
(  const char* mode
)  ;

void filterLinefeed
(  filter* fd
,  FILE*   fp
)  ;

void filterSetFP
(  filter* fd
,  FILE*   fp
)  ;

extern int ZOEM_FILTER_NONE   ;
extern int ZOEM_FILTER_DEFAULT;
extern int ZOEM_FILTER_DEVICE ;
extern int ZOEM_FILTER_TXT    ;
extern int ZOEM_FILTER_COPY   ;
extern int ZOEM_FILTER_STRIP  ;

#endif

