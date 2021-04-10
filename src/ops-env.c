/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include "ops-env.h"
#include "util.h"
#include "segment.h"
#include "parse.h"
#include "curly.h"
#include "key.h"
#include "sink.h"
#include "digest.h"

#include "tingea/ting.h"
#include "tingea/hash.h"
#include "tingea/alloc.h"
#include "tingea/compile.h"


typedef struct
{  mcxTing* init
;  mcxTing* open
;  mcxTing* close
;
}  envy     ;


static   mcxHash*    envTable_g        =  NULL;    /* environment keys  */


mcxstatus yamEnvNew
(  const char* tag
,  const char* initstr
,  const char* openstr
,  const char* closestr
,  yamSeg*     seg
)
   {  mcxTing*  opentag    =  mcxTingNew(tag)
   ;  mcxbool ok           =  FALSE
   ;  mcxKV*  kv

   ;  do
      {  envy* e = NULL
      ;  kv =  mcxHashSearch(opentag, envTable_g, MCX_DATUM_INSERT)

      ;  if (kv->key != opentag)
         {  yamErr("\\env#4", "overwriting key <%s>",opentag->str)
         ;  mcxTingFree(&opentag)
      ;  }

         e = kv->val

      ;  if (!e)
         {  e = mcxAlloc(sizeof(envy), RETURN_ON_FAIL)
         ;  if (!e)
            break
         ;  e->init  = mcxTingEmpty(NULL, 40)
         ;  e->open  = mcxTingEmpty(NULL, 80)
         ;  e->close = mcxTingEmpty(NULL, 20)
         ;  kv->val = e
      ;  }

         mcxTingWrite(e->init, initstr)
      ;  mcxTingWrite(e->open, openstr)
      ;  mcxTingWrite(e->close, closestr)
      ;  if (yamDigest(e->init, e->init, seg))
         break

      ;  ok = TRUE
   ;  }
      while (0)

   ;  if (!ok)
      {  mcxTingFree(&opentag)
      ;  return STATUS_FAIL
   ;  }

      return STATUS_OK
;  }


const char* yamEnvOpen
(  const char* label_
,  const char* data_
,  yamSeg*  seg
)
   {  mcxTing* label    =  mcxTingNew(label_)
   ;  mcxTing* data     =  data_ ? mcxTingNew(data_) : NULL
   ;  const char* val   =  NULL
   ;  mcxKV*  kv        =  mcxHashSearch(label, envTable_g, MCX_DATUM_FIND)
   ;  mcxTing* lkey     =  mcxTingEmpty(NULL, 30)
   ;  mcxTing* tg_tmp   =  mcxTingEmpty(NULL, 20)
   ;  const char* me    =  "\\begin#2"
   ;  mcxbool ok        =  FALSE

   ;  while (1)
      {  if (kv)
         {  yamSeg* initseg   =  NULL
         ;  envy* e           =  kv->val
         ;  int x             =  0

         ;  if (sinkDictPush(label->str))   /* localize everything */
            break

         ;  initseg  =  yamStackPushTmp(e->init)
         ;  val      =  e->open->str

         ;  while ((x = yamParseScopes(initseg, 2, 0)) == 2)
            {  int delta = (uchar) arg1_g->str[0] == '$' ? 1 : 0
            ;  mcxTingPrint(tg_tmp, "$%s", arg1_g->str+delta)
            ;  yamKeyDef(tg_tmp->str, arg2_g->str)
         ;  }
            yamStackFreeTmp(&initseg)
      ;  }
         else
         {  yamErr(me, "env <%s> does not exist", label->str)
         ;  break
      ;  }

         if (data && data->len)
         {  int x, n_args = 0
         ;  yamSeg*  argseg

         ;  yamKeyDef("$__args__", data->str)
         ;  if (yamDigest(data, data, seg))
            {  yamErr
               (me, "arguments in env <%s> did not parse", label->str)
            ;  break
         ;  }
            yamKeyDef("$__xargs__", data->str)
         ;  argseg = yamStackPushTmp(data)

         ;  while ((x = yamParseScopes(argseg, 2, 0)) == 2)
            {  int namelen = 0
            ;  mcxbits keybits = 0
            ;  int delta = (uchar) arg1_g->str[0] == '$' ? 1 : 0
            ;  mcxTingPrint(lkey, "$%s", arg1_g->str+delta)
            ;  n_args++
            ;  if
               (  (  checkusrsig
                     (  lkey->str, lkey->len, NULL, &namelen, &keybits
                     )
                  != lkey->len         /* mixed sign comparison */
                  )
               || keybits & KEY_ZOEMONLY
               )
               {  yamErr(me, "invalid key <%s>", lkey->str)
               ;  break
            ;  }
               else
               yamKeySet(lkey->str, arg2_g->str)
         ;  }

            yamStackFreeTmp(&argseg)
         ;  mcxTingPrint(lkey, "%d", n_args)
         ;  yamKeyDef("$0", lkey->str)
      ;  }
         else
            yamKeyDef("$0", "0")
         ,  yamKeyDef("$__args__", "")
         ,  yamKeyDef("$__xargs__", "")

      ;  ok = TRUE
      ;  break
   ;  }

      mcxTingFree(&label)
   ;  mcxTingFree(&data)
   ;  mcxTingFree(&lkey)
   ;  mcxTingFree(&tg_tmp)

   ;  return ok ? val : NULL
;  }


mcxstatus yamEnvClose
(  const char* label_
)
   {  mcxTing* label = mcxTingNew(label_)
   ;  mcxstatus status = STATUS_FAIL

   ;  while (1)
      {  if (sinkDictPop(label->str))  /* no trailing garbage */
         break
      ;  status = STATUS_OK
      ;  break
   ;  }

      mcxTingFree(&label)
   ;  return status
;  }


const char* yamEnvEnd
(  const char* label_
,  yamSeg*  seg_unused cpl__unused
)
   {  mcxTing* label =  mcxTingNew(label_)
   ;  mcxKV* kv      =  mcxHashSearch(label, envTable_g, MCX_DATUM_FIND)
   ;  mcxTingFree(&label)
   ;  return kv ? ((envy*) kv->val)->close->str : NULL
;  }


void mod_env_init
(  dim n
)
   {  envTable_g = yamHashNew(n)
;  }


void envy_free
(  void*   envy_v
)
   {  envy* e = envy_v
   ;  mcxTingFree(&(e->init))
   ;  mcxTingFree(&(e->open))
   ;  mcxTingFree(&(e->close))
;  }


void mod_env_exit
(  void
)
   {  mcxHashFree(&envTable_g, mcxTingRelease, envy_free)
;  }


