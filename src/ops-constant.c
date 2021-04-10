/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include "ops-constant.h"
#include "filter.h"
#include "util.h"
#include "curly.h"

#include "tingea/hash.h"
#include "tingea/ting.h"

static   mcxHash*    cstTable_g        =  NULL;    /* constants         */


mcxTing* yamConstantGet
(  mcxTing* key
)
   {  mcxKV*   kv =  (mcxKV*) mcxHashSearch(key, cstTable_g, MCX_DATUM_FIND)
   ;  return kv ? (mcxTing*) kv->val : NULL
;  }


/* fixme
 * is key really owned by  yCN?
*/

mcxTing* yamConstantNew
(  mcxTing* key
,  const char* val
)
   {  mcxKV*   kv =  (mcxKV*) mcxHashSearch(key, cstTable_g, MCX_DATUM_INSERT)

   ;  if ((mcxTing*) kv->key != key)
      mcxTingWrite((mcxTing*)kv->val, val)
   ;  else
      kv->val = mcxTingNew(val)

   ;  return (mcxTing*) kv->key
;  }


void mod_constant_init
(  dim n
)
   {  cstTable_g           =  yamHashNew(n)
;  }


void mod_constant_exit
(  void
)
   {  mcxHashFree(&cstTable_g, mcxTingRelease, mcxTingRelease)
;  }


