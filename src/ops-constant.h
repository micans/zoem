/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_constant_h__
#define zoem_constant_h__

#include "tingea/ting.h"


void mod_constant_init
(  dim n
)  ;

void mod_constant_exit
(  void
)  ;

mcxTing* yamConstantGet
(  mcxTing* label
)  ;

mcxTing* yamConstantNew
(  mcxTing* key
,  const char* val
)  ;

#endif

