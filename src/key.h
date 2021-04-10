/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_key_h
#define zoem_key_h

#include "filter.h"
#include "dict.h"

#include "tingea/ting.h"
#include "tingea/io.h"
#include "tingea/types.h"
#include "tingea/hash.h"


mcxstatus usrDictPush
(  const char* label
)  ;

mcxstatus usrDictPop
(  const char* label
)  ;

void  yamKeySet
(  const char* key
,  const char* val
)  ;

void  yamKeyDef
(  const char* key
,  const char* val
)  ;

mcxTing* yamKeyDelete
(  const char* keystr
,  mcxbool     global
)  ;

mcxbool yamKeyDeletex
(  const char* key
)  ;

mcxTing* yamKeyGet
(  mcxTing*  key
)  ;

mcxTing* yamKeyGetByDictName
(  mcxTing* dict_name
,  mcxTing* key
)  ;

mcxTing* yamKeyGetLocal
(  mcxTing*  key
)  ;

mcxTing* yamKeyGetGlobal
(  mcxTing*  key
)  ;

mcxTing* yamKeyInsert
(  mcxTing*       key
,  const char*    valstr
,  dim            vallen
,  mcxbits        bits         /* YAM_KEY_{COND,APPEND,GLOBAL} */
,  const mcxTing* dict_name
)  ;


void mod_key_init
(  dim n
)  ;

void mod_key_exit
(  void
)  ;

void yamKeyStats
(  void
)  ;

void yamKeyList
(  const char* mode
)  ;

void keyULimit
(  dim   user
)  ;

#endif


