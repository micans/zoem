/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ops-grape.h"

#include "util.h"
#include "parse.h"

#include "tingea/ting.h"
#include "tingea/hash.h"
#include "tingea/alloc.h"
#include "tingea/types.h"
#include "tingea/compile.h"


static const char* spaces =
"                                                                           ";
/* must be 75 spaces */


/* ahem, design:
 * all the keys in a tbl are mcxTings.
 * all the vals in a tbl are dnodes.
 * all user values are stored in dnode->dat.
*/


typedef struct dnode
{  mcxTing* dat
;  mcxHash* tbl
;
}  dnode       ;     /* data node */


                                       /* (how) can I do this in one step? */
static   dnode    dn__           =     { NULL, NULL } ;
static   dnode*   dnode_g        =     &dn__ ;


static dnode* dnode_new
(  const char* str
,  int n
)
   {  dnode* dn      =  mcxAlloc(sizeof(dnode), EXIT_ON_FAIL)
   ;  dn->dat        =  str ? mcxTingNew(str) : NULL
   ;  dn->tbl        =  n   ? yamHashNew(4) : NULL
   ;  return dn
;  }



static void dnode_free
(  void* dnodepp
)
   {  dnode* dn = *((dnode**) dnodepp)
   ;  if (dn)
      mcxFree(dn)
;  }


/*
 * if slot exists, remove everything attached to key slot in dn->tbl.
 * otherwise, empty tbl and dat members in dn,
 * leave dn in consistent state.
*/

static mcxbool dnode_cut
(  dnode* dn
,  mcxTing* slot
)
   {  if (!dn)
      return FALSE

   ;  if (slot && !dn->tbl)
      return FALSE

   ;  else if (slot)
      {  mcxKV* kv   =  mcxHashSearch(slot, dn->tbl, MCX_DATUM_DELETE)
      ;  dnode* dnx  =  kv ? (dnode*) kv->val : NULL
      ;  mcxbool ok  =  kv ? dnode_cut(dnx, NULL) : FALSE
      ;  mcxTing* txt=  kv ? (mcxTing*) kv->key : NULL

      ;  mcxTingFree(&txt)
      ;  dnode_free(&dnx)
      ;  return ok
   ;  }
      else
      {  mcxbool ok  =  TRUE
      ;  if (dn->dat)
         {  mcxTingFree(&(dn->dat))
      ;  }
         if (dn->tbl)
         {  mcxHashWalk* walk = mcxHashWalkInit(dn->tbl)
         ;  dim i_bucket
         ;  mcxKV* kv

         ;  while((kv = mcxHashWalkStep(walk, &i_bucket)))
            {  dnode* dnx = (dnode*) kv->val
            ;  ok = ok && dnode_cut(dnx, NULL)
         ;  }
            mcxHashFree(&(dn->tbl), mcxTingRelease, mcxHashFreeScalar)
         ;  mcxHashWalkFree(&walk)
      ;  }
        /*  these two actions leave dnx->tbl in consistent state so that
         *  calling dnode_cut can either issue a mcxHashFree (in this branch,
         *  the !slot case) on the table in which dnx is a value,
         *  or it can just remove the key-value pair in which dnx is the
         *  value (in the slot! branch).
        */
         return ok
   ;  }
      return TRUE             /* unreachcode */
;  }


void dnodePrint
(  dnode* dn
,  int    level
)  ;


mcxstatus yamDataSet
(  const char* str
,  mcxbits  bits
,  mcxbool  *overwrite
,  mcxbool  cond
)
   {  mcxTing* val   =  mcxTingNew(str)
   ;  mcxbool  warn  =  bits & YAM_SET_WARN
   ;  mcxbool  unary =  bits & YAM_SET_UNARY
   ;  int i
   ;  dnode* dn      =  dnode_g
   ;  mcxTing* key   =  mcxTingEmpty(NULL, 10)
   ;  mcxbool scopes =  !unary && seescope(val->str, val->len) >= 0 ? TRUE : FALSE
   ;  mcxbool newleaf = TRUE

   ;  for (i=1;i<=n_args_g;i++)
      {  mcxKV* kv
      ;  if (!dn->tbl)
         dn->tbl     =  yamHashNew(4)
      ;  mcxTingWrite(key, key_and_args_g[i].str)
      ;  kv          =  mcxHashSearch
                        (  key
                        ,  dn->tbl
                        ,  MCX_DATUM_INSERT
                        )
      ;  if (kv->key == key)   /* autovivification, key must now be renewed */
         {  key      =  mcxTingEmpty(NULL, 10)
         ;  kv->val  =  dnode_new(NULL, 0)
         ;  newleaf  =  TRUE
      ;  }
         else
         newleaf = FALSE
      ;  dn =  (dnode*) kv->val
   ;  }

      if (scopes)
      {  yamSeg*  tmpseg  = yamStackPushTmp(val)
      ;  int   x
      ;  int   ct = 0

      ;  if (!dn->tbl)
         dn->tbl = yamHashNew(4)

      ;  while ((x = yamParseScopes(tmpseg, 2, 0)) == 2)
         {  mcxTing* lkey  =  mcxTingNew(arg1_g->str)
         ;  mcxKV*   kv    =  mcxHashSearch(lkey, dn->tbl, MCX_DATUM_INSERT)

         ;  ct += x

         ;  if (kv->key == lkey)
            kv->val = dnode_new(arg2_g->str, 0)
         ;  else
            {  mcxTingFree(&lkey)
            ;  if (!cond)
               {  mcxTingWrite(((dnode*) kv->val)->dat, arg2_g->str)
               ;  if (overwrite)
                  *overwrite = TRUE
               ;  if (warn)
                  mcxErr
                  (  "data"
                  ,  "overwriting key/val <%s>/<%s>"
                  ,  ((mcxTing*) kv->key)->str
                  ,  arg2_g->str
                  )
            ;  }
            }
         }

         ct += x
      ;  yamStackFreeTmp(&tmpseg)
      ;  mcxTingFree(&val)

      ;  if (ct == 1)
         {  if (!dn->dat)
            dn->dat = mcxTingNew(arg1_g->str)
         ;  else if (!cond)
            mcxTingWrite(dn->dat, arg1_g->str)
      ;  }
         else if (x == 1)
         yamErr
         (  "\\dset#2"
         ,  "ignoring trailing value <%s>"
         ,  arg1_g->str  
         )
   ;  }
      else
      {  if (!dn->dat)
         dn->dat = val
      ;  else if (!cond)
         {  mcxTingWrite(dn->dat, val->str)
         ;  if (overwrite)
            *overwrite = TRUE
         ;  if (warn)
            mcxErr
            (  "data"
            ,  "overwriting key/val <%s>/<%s>"
            ,  key->str
            ,  val->str
            )
         ;  mcxTingFree(&val)
      ;  }
      }

      mcxTingFree(&key)
   ;  return STATUS_OK
;  }


const char* yamDataGet
(  void
)
   {  int i
   ;  dnode* dn      =  dnode_g
   ;  mcxTing* key   =  mcxTingEmpty(NULL, 10)

   ;  for (i=1;i<=n_args_g;i++)
      {  mcxKV* kv
      ;  if (!dn || !dn->tbl)
         {  mcxTingFree(&key)
         ;  return NULL
      ;  }

         mcxTingWrite(key, key_and_args_g[i].str)
      ;  kv  =  mcxHashSearch
                (  key
                ,  dn->tbl
                ,  MCX_DATUM_FIND
                )
      ;  if (!kv)
         {  mcxTingFree(&key)
         ;  return NULL
      ;  }
         dn =  (dnode*) kv->val
   ;  }

      mcxTingFree(&key)
   ;  return dn->dat ? dn->dat->str : NULL
;  }


mcxstatus yamDataFree
(  void
)
   {  int i
   ;  dnode* dn      =  dnode_g
   ;  mcxTing* key   =  mcxTingEmpty(NULL, 10)
   ;  mcxTing* slot  =  NULL
   ;  mcxbool ok     =  TRUE

   ;  for (i=1;i<n_args_g;i++)
      {  mcxKV* kv
      ;  if (!dn || !dn->tbl)
         {  mcxTingFree(&key)
         ;  return STATUS_FAIL
      ;  }

         mcxTingWrite(key, key_and_args_g[i].str)
      ;  kv =  mcxHashSearch
               (  key
               ,  dn->tbl
               ,  MCX_DATUM_FIND
               )
      ;  if (!kv)
         {  mcxTingFree(&key)
         ;  return STATUS_FAIL
      ;  }
         dn =  (dnode*) kv->val
   ;  }

      if (n_args_g)
      slot = mcxTingNew(key_and_args_g[n_args_g].str)
   ;  else
      slot = NULL

   ;  ok =  dnode_cut(dn, slot)

   ;  mcxTingFree(&key)
   ;  mcxTingFree(&slot)

   ;  return ok ? STATUS_OK : STATUS_FAIL
;  }


mcxstatus yamDataPrint
(  void
)
   {  int i
   ;  dnode* dn      =  dnode_g
   ;  mcxTing* key   =  mcxTingEmpty(NULL, 10)

   ;  fprintf(stdout, "# printing node <ROOT>")

   ;  for (i=1;i<=n_args_g;i++)
      {  mcxKV* kv
      ;  if (!dn || !dn->tbl)
         {  mcxTingFree(&key)
         ;  return STATUS_FAIL
      ;  }
         mcxTingWrite(key, key_and_args_g[i].str)
      ;  fprintf(stdout, "<%s>", key->str)
      ;  kv =  mcxHashSearch
               (  key
               ,  dn->tbl
               ,  MCX_DATUM_FIND
               )
      ;  if (!kv)
         {  mcxTingFree(&key)
         ;  return STATUS_FAIL
      ;  }
         dn =  (dnode*) kv->val
   ;  }

      fprintf(stdout, "\n")
   ;  dnodePrint(dn, n_args_g)
   ;  fprintf(stdout, "# done printing data\n")
   ;  mcxTingFree(&key)
   ;  return STATUS_OK
;  }


void dnodePrint
(  dnode* dn
,  int    level
)
   {  if (level > 74)
      {  fprintf(stdout, "%s....\n", spaces)
      ;  return
   ;  }

      if (dn->dat)
      {  fprintf(stdout, "%s", spaces+(75-level))
      ;  fprintf(stdout, "|<%s>\n", dn->dat->str)
   ;  }

   ;  if (dn->tbl)
      {  mcxHashWalk *walk = mcxHashWalkInit(dn->tbl)
      ;  mcxKV* kv
      ;  dim i_bucket

      ;  while((kv = mcxHashWalkStep(walk, &i_bucket)))
         {  fprintf(stdout, "%s", spaces+(75-level-1))
         ;  fprintf(stdout, "*<%s>\n", ((mcxTing*) kv->key)->str)
         ;  dnodePrint((dnode*) kv->val, level+1)
      ;  }
         mcxHashWalkFree(&walk)
   ;  }
      return
;  }


/* fixme: ugly use of n_args_g in here.
*/

void mod_grape_exit
(  void
)
   {  n_args_g = 0
   ;  yamDataFree()
;  }

void mod_grape_init
(  void
)
   {
;  }


