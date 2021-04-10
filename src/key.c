/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *   (C) Copyright 2008, 2009  Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include "key.h"
#include "dict.h"
#include "sink.h"
#include "read.h"
#include "util.h"

#include "tingea/ting.h"
#include "tingea/io.h"
#include "tingea/minmax.h"
#include "tingea/types.h"
#include "tingea/array.h"
#include "tingea/hash.h"

/*
,  __$args__
,  __$xargs__

!  __sysval__
!  __zoemput__
!  __zoemstat__
*/

const char *strSession[]
=
{  "\\$__args__       (local to env) key/value pairs given to \\begin#2"
,  "\\$__xargs__      (local to env) key/value pairs given to \\begin#2, expanded"
,  "\\__device__     name of device (given by -d)"
,  "\\__fnbase__     base name of entry file (given by -i/-I)"
,  "\\__fnentry__    name of entry file (given by -i/-I)"
,  "\\__fnin__       name of current input file"
,  "\\__fnout__      name of current output file"
,  "\\__fnpath__     path component of entry file (given by -i/-I)"
,  "\\__fnwrite__    arg1 to \\write#3, accessible in arg3 scope"
,  "\\__lc__         expands to a left curly (only for magic)"
,  "\\__line__       index of current input line"
,  "\\__parmode__    paragraph slurping mode for interactive sessions"
,  "\\__rc__         expands to a right curly (only for magic)"
,  "\\__searchpath__ search path for macro packages (e.g. man.zmm)"
,  "\\__split__      user space toggle for chapter mode indicator"
,  "\\__sysval__     exit status of last system command"
,  "\\__version__    version of zoem, formatted as e.g. 2003, 2004-010"
,  "\\__zoemput__    result text of last \\try#1 key"
,  "\\__zoemstat__   status of last \\try#1 key"

,  NULL
}  ;


static dictStack* usrstack = NULL;
static dim n_user_override = 0;


mcxstatus usrDictPop
(  const char* label
)
   {  return dictStackPop(usrstack, "user", label)
;  }

mcxstatus usrDictPush
(  const char* label
)
   {  return dictStackPush(usrstack, 16, label)
;  }


void yamKeyList
(  const char* mode
)
   {  mcxbool listAll = strstr(mode, "all") != NULL
   ;  if (listAll || strstr(mode, "session"))  
      {  int m
      ;  fputs("\nPredefined session variables\n", stdout)
      ;  for (m=0;strSession[m];m++)
         fprintf(stdout, "%s\n", strSession[m])
   ;  }
;  }


void  yamKeySet
(  const char* key
,  const char* val
)
   {  mcxTing*  keytxt   =  mcxTingNew(key)

   ;  if (yamKeyInsert(keytxt, val, strlen(val), 0, NULL) != keytxt)
      mcxTingFree(&keytxt)
;  }


void  yamKeyDef
(  const char* key
,  const char* val
)
   {  mcxTing*  keytxt   =  mcxTingNew(key)

   ;  if (yamKeyInsert(keytxt, val, strlen(val), 0, NULL) != keytxt)
      {  yamErr("yamKeyDef", "overwriting key <%s> (now %s)", keytxt->str, val)
      ;  mcxTingFree(&keytxt)
   ;  }
   }


mcxKV* yamKVgetx
(  mcxTing*  key
,  const mcxTing*  dict_name
,  mcxbits   bits
)
   {  keyDict *dict = usrstack->top
   ;  mcxenum DATUM_MODE = bits & YAM_SET_EXISTING ? MCX_DATUM_FIND : MCX_DATUM_INSERT
   ;  mcxKV* kv = NULL
   ;  int n_dict_ok = 0       /* number of matching dictionaries */
   ;  mcxbool dollar = (unsigned char) key->str[0] == '$'

   ;  if (dollar)
      dict = sinkGetDLRtop()
   ;  else if (bits & YAM_SET_GLOBAL)
      dict = usrstack->bottom

;if(0)fprintf(stderr, "hey!\n")
   ;  while (dict)
      {  int dict_ok = !dict_name || !strcmp(dict_name->str, dict->name->str)
      ;  n_dict_ok += dict_ok
      ;  if (dict_ok && (kv = mcxHashSearch(key, dict->table, DATUM_MODE)))
         break
      ;  dict = dict->down
   ;  }

      if (!kv && dollar)
      {  dict = sinkGetDLRdefault()
;if(0)fprintf(stderr, "hi!\n")
      ;  while (dict)
         {  if (dict_name && strcmp(dict_name->str, dict->name->str))
            do { } while (0)
         ;  else if ((kv = mcxHashSearch(key, dict->table, DATUM_MODE)))
            break
         ;  dict = dict->down
      ;  }
      }

      if (dict_name && !n_dict_ok)
      yamErr
      (  "\\set#3"
      ,  "%s dictionary <%s> not found"
      ,  dollar ? "dollar" : "user"
      ,  dict_name->str
      )
   ;  else if (bits & YAM_SET_EXISTING && !kv)
      yamErr("\\set#3", "key <%s> does not exist", key->str)

   ;  return kv
;  }


mcxKV* yamKVget
(  mcxTing*  key
)
   {  keyDict *dict =      (unsigned char) key->str[0] == '$'
                        ?  sinkGetDLRtop()
                        :  usrstack->top
   ;  mcxKV* kv = NULL

   ;  while (dict)
      {  if ((kv = mcxHashSearch(key, dict->table, MCX_DATUM_FIND)))
         break
      ;  dict = dict->down
   ;  }

      if (!kv && (unsigned char) key->str[0] == '$')
      {  dict = sinkGetDLRdefault()
      ;  while (dict)
         {  if ((kv = mcxHashSearch(key, dict->table, MCX_DATUM_FIND)))
            break
         ;  dict = dict->down
      ;  }
      }
;if (kv && !kv->val)
fprintf(stderr, "Odd: no value for key %s\n", key->str);
      return kv
;  }


mcxTing* yamKeyInsert
(  mcxTing*       key
,  const char*    valstr
,  dim            vallen
,  mcxbits        bits
,  const mcxTing* dict_name
)
   {  mcxKV* kv = NULL
   ;  mcxbool scope_env = (unsigned char) key->str[0] == '$'
   
   ;  if ((bits & YAM_SET_EXISTING) || dict_name)
      kv = yamKVgetx(key, dict_name, bits & (YAM_SET_EXISTING | YAM_SET_GLOBAL))
   ;  else
      {  keyDict *dict
      =    scope_env
         ?  sinkGetDLRtop()
         :     bits & YAM_SET_GLOBAL
            ?  usrstack->bottom
            :  usrstack->top

      ;  kv =  mcxHashSearch
               (  key
               ,  dict->table
               ,  MCX_DATUM_INSERT
               )
   ;  }

      if (kv)
      {  if (!kv->val)
         kv->val = mcxTingNNew(valstr, vallen)
      ;  else if (bits & YAM_SET_APPEND)
         mcxTingNAppend((mcxTing*) (kv->val), valstr, vallen)
      ;  else if (!(bits & YAM_SET_COND))
         mcxTingNWrite((mcxTing*) (kv->val), valstr, vallen)
   ;  }
      return kv ? kv->key : NULL
;  }


mcxbool yamKeyDeletex
(  const char* key
)
   {  mcxTing* deletee = yamKeyDelete(key, FALSE)
   ;  mcxbool  found = deletee ? TRUE : FALSE
   ;  mcxTingFree(&deletee)
   ;  return found
;  }


mcxTing* yamKeyDelete
(  const char* keystr
,  mcxbool     global
)
   {  mcxTing* key   = mcxTingNew(keystr)
   ;  keyDict *dict  =     global
                        ?  usrstack->bottom
                        :     keystr[0] == '$'
                           ?  sinkGetDLRtop()
                           :  usrstack->top

   ;  mcxKV*   kv
            =  (mcxKV*) mcxHashSearch
               (  key
               ,  dict->table
               ,  MCX_DATUM_DELETE
               )
   ;  mcxTingFree(&key)

   ;  if (kv)
      {  mcxTing* kvval = kv->val
      ;  mcxTing* kvkey = kv->key
      ;  mcxTingFree(&kvkey)
      ;  return kvval
   ;  }
      return NULL
;  }


mcxTing* yamKeyGetGlobal
(  mcxTing*  key
)
   {  keyDict *dict = usrstack->bottom
   ;  mcxKV*   kv
      =  (mcxKV*) mcxHashSearch
         (  key
         ,  dict->table
         ,  MCX_DATUM_FIND
         )
   ;  if (kv)
      return (mcxTing*) kv->val
   ;  return NULL
;  }


mcxTing* yamKeyGetLocal
(  mcxTing*  key
)
   {  keyDict *dict  = *(key->str) == '$' ? sinkGetDLRtop() : usrstack->top
   ;  mcxKV*   kv
      =  (mcxKV*) mcxHashSearch
         (  key
         ,  dict->table
         ,  MCX_DATUM_FIND
         )
   ;  if (kv)
      return (mcxTing*) kv->val
   ;  return NULL
;  }


mcxTing* yamKeyGetByDictName
(  mcxTing*  dict_name
,  mcxTing*  key
)
   {  keyDict *dict = usrstack->top
   ;  mcxKV* kv = NULL
   ;  mcxbool dollar = (unsigned char) key->str[0] == '$'

   ;  if (dollar)
      dict = sinkGetDLRtop()

   ;  while (dict)
      {  if
         (  !strcmp(dict_name->str, dict->name->str)
         && (kv = mcxHashSearch(key, dict->table, MCX_DATUM_FIND))
         )
         break
      ;  dict = dict->down
   ;  }

      if (!kv && dollar)
      {  dict = sinkGetDLRdefault()
      ;  while (dict)
         {  
if(0)fprintf(stderr, "search dict %s\n", dict->name->str)
;           if
            (  !strcmp(dict_name->str, dict->name->str)
            && (kv = mcxHashSearch(key, dict->table, MCX_DATUM_FIND))
            )
            break
         ;  dict = dict->down
      ;  }
      }

   ;  return kv ? kv->val : NULL
;  }


mcxTing* yamKeyGet
(  mcxTing*  key
)
   {  mcxKV* kv = yamKVget(key)
   ;  return kv ? kv->val : NULL
;  }


void mod_key_exit
(  void
)
   {  mcxTing* ting = mcxTingNew("__fnout__")
   ;  mcxTing* fnout = yamKeyGet(ting)
               /* fnout is written in bottom-level dictionary,
                * so dictStackFree may only use third argument
                * /before/ it frees that dictionary.
               */
   ;  dictStackFree(&usrstack, "user", fnout ? fnout->str : "panic")
   ;  mcxTingFree(&ting)
;  }


void mod_key_init
(  dim dict_size
)
   {  dim n_max = n_user_override > 0 ? n_user_override : 100
   ;  usrstack = dictStackNew(dict_size, n_max, "''")
;  }


void yamKeyStats
(  void
)
   {  mcxHashStats(stdout, usrstack->bottom->table)
;  }


void keyULimit
(  dim   n_user
)
   {  n_user_override = n_user;
;  }


