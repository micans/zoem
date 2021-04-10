/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_ops_h__
#define zoem_ops_h__

#include "segment.h"

#include "tingea/types.h"
#include "tingea/ting.h"

mcxbool  yamOpList
(  const char* mode
)  ;

void yamOpsStats
(  void
)  ;

void mod_ops_init
(  dim n
)  ;

void mod_ops_exit
(  void
)  ;

mcxstatus mod_registrees
(  void
)  ;

void mod_ops_digest
(  void
)  ;

xpnfnc yamOpGet
(  mcxTing* txt
,  const char** compositepp
)  ;

#endif

