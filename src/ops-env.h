/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_env_h__
#define zoem_env_h__

#include "segment.h"

#include "tingea/ting.h"

mcxstatus yamEnvNew
(  const char* tag
,  const char* initstr
,  const char* openstr
,  const char* closestr
,  yamSeg* seg
)  ;

const char* yamEnvOpen
(  const char* label
,  const char* data
,  yamSeg*  seg
)  ;

const char* yamEnvEnd
(  const char* str
,  yamSeg*  seg
)  ;

mcxstatus yamEnvClose
(  const char* str
)  ;

void mod_env_init
(  dim n
)  ;

void mod_env_exit
(  void
)  ;

#endif

