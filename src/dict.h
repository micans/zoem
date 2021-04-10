/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_dict_h
#define zoem_dict_h

#include "tingea/ting.h"
#include "tingea/io.h"
#include "tingea/types.h"
#include "tingea/hash.h"


typedef struct keyDict
{  struct keyDict*   down
;  mcxHash*          table
;  mcxTing*          name
;  unsigned          level
;
}  keyDict           ;


typedef struct
{  keyDict*    bottom
;  keyDict*    top
;  int         n_dict
;  int         N_dict
;
}  dictStack   ;


dictStack* dictStackNew
(  dim         dict_size
,  dim         n_dict_max
,  const char* name
)  ;


/* Does not free stack itself */
void dictStackFree
(  dictStack** stack
,  const char* type
,  const char* fname
)  ;

mcxstatus dictStackPush
(  dictStack* stack
,  int dict_size
,  const char* name
)  ;

mcxstatus dictStackPop
(  dictStack* stack
,  const char* type
,  const char* name
)  ;


#endif

