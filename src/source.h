/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/


/*
 * This module maintains a stack of input files. It is also woefully
 * underdocumented. For starters, find+document/resolve the difference
 * between enter_interactive and sourceStdia
*/

#ifndef zoem_source_h__
#define zoem_source_h__

#include "sink.h"
#include "util.h"

#include "tingea/io.h"
#include "tingea/ting.h"
#include "tingea/types.h"


void enter_interactive
(  void
)  ;


enum
{  STATUS_FAIL_OPEN  =  STATUS_UNUSED
,  STATUS_FAIL_READ
,  STATUS_FAIL_ZOEM
}  ;


#define  SOURCE_DEFAULT_SINK   8      /* interpretation only        */
#define  SOURCE_NO_INLINE     16
#define  SOURCE_NO_SEARCH     32
#define  SOURCE_DEBUG         64


mcxstatus sourceAscend
(  mcxTing        *fnsearch
,  mcxbits        modes
,  size_t         chunk_size
)  ;

void sourceStdia
(  mcxTing* filetxt
)  ;


void sourceIncrLc
(  const mcxTing* txt
,  int   d
)  ;
long sourceGetLc
(  void
)  ;
const char* sourceGetName
(  void
)  ;
mcxTing* sourceGetPath
(  void
)  ;
mcxbool sourceCanPush
(  void
)  ;

void mod_source_init
(  void
)  ;

void mod_source_exit
(  void
)  ;

#endif

