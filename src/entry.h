/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_entry_h__
#define zoem_entry_h__

#include "filter.h"

#include "tingea/ting.h"
#include "tingea/types.h"

#include <stdio.h>

#define ZOEM_NO_AZM 1

/*
 * fbase always gets the '.azm' appended without check.
 * If fnout != NULL it will be the output name, otherwise the output
 * name is constructed from fbase and device.
*/

mcxstatus yamEntry
(  const char* fnentry
,  const char* fnpath
,  const char* fnbase
,  ssize_t     chunk_size
,  const char* fnout
,  const char* device
,  int         fltidx
,  mcxbits     flags
,  mcxbits     other_flags
,  mcxTing*    vars
,  mcxTing*    expr
)  ;

extern int buckets_user;
extern int buckets_zoem;

#endif

