/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

/* TODO.
 *    Most of the logic is concentrated in sinkPush. It is a bit dense
 *    and underdocumented.
*/


#include <stdio.h>

#include "filter.h"
#include "util.h"
#include "entry.h"
#include "key.h"
#include "read.h"
#include "sink.h"

#include "tingea/hash.h"
#include "tingea/ting.h"
#include "tingea/types.h"
#include "tingea/alloc.h"
#include "tingea/io.h"
#include "tingea/minmax.h"
#include "tingea/compile.h"


#ifdef DEBUG
#  define DEBUG_BU DEBUG
#endif

#define DEBUG 0

typedef struct
{  mcxLink*       sp
;
}  busystack      ;


static busystack  bst         =  {  NULL  }  ;
static busystack* busystack_g =  &bst        ;
       sink* sink_default_g   =  NULL        ;
static int   sink_fltidx_g    =  0           ;    /* ZOEM_FILTER_NONE  */
static mcxHash*   wrtTable_g  =  NULL        ;    /* open output files */

static dim n_dict_dollar_max  =  100         ;


void mod_sink_init
(  dim   n
)
   {  wrtTable_g        =  yamHashNew(n)
   ;  busystack_g->sp   =  mcxListSource(10, MCX_GRIM_ARITHMETIC)
;  }


  /*  {  mcxIO* xf = yamOutputNew("foobar")
      ;  sinkPush(xf->usr, ZOEM_FILTER_NONE)
   ;  */


void warn_stack_free
(  lblStack** st
,  const char* type
,  const char* file
)
   {  mcxLink* lk = (*st)->sp
   ;  while (lk->prev)
      {  mcxErr
         (  "env"
         ,  "file <%s> open %s <%s>"
         ,  file
         ,  type
         ,  ((mcxTing*)lk->val)->str
         )
      ;  lk = lk->prev
   ;  }
      ting_stack_free(st)
;  }


void sinkFree
(  sink* sk
)
   {  filterFree(sk->fd)
   ;  warn_stack_free(&(sk->xmlStack), "tag", sk->fname)
   ;  dictStackFree(&(sk->dlrStack), "env", sk->fname)
   ;  mcxFree(sk->fname)
   ;  mcxFree(sk)
;  }


void yamOutputClose
(  const char*  s
)
   {  mcxTing* fname = mcxTingNew(s)
   ;  mcxKV* kv = mcxHashSearch(fname, wrtTable_g, MCX_DATUM_FIND)
   ;  if (kv)
      {  mcxIO* xf = (mcxIO*) kv->val
      ;  fflush(xf->fp)
      ;  mcxIOclose(xf)
   ;  }
      /* keep the object around; repeated \writeto should append
       * (this might be settable someday)
      */
      mcxTingFree(&fname)
;  }


sink* sinkNew
(  mcxIO* xf
)
   {  sink* sk       =  mcxAlloc(sizeof(sink), EXIT_ON_FAIL)
   ;  sk->fd         =  xf ? filterNew(xf->fp) : NULL
   ;  sk->xmlStack   =  ting_stack_new(10)
   ;  sk->dlrStack   =  dictStackNew(20, n_dict_dollar_max, "''")
   ;  sk->fname      =  mcxTingStr(xf->fn)
   ;  return sk
;  }


mcxIO* yamOutputNew
(  const char*  s
)
   {  mcxIO *xf            =  NULL
   ;  mcxTing *fname       =  mcxTingNew(s)
   ;  mcxKV *kv

   ;  if (fname->len > 1024)
      {  yamErr
         (  "yamOutputNew"
         ,  "output file name expansion too <%d> long"
         ,  fname->len
         )
      ;  mcxTingFree(&fname)
      ;  return NULL
   ;  }

      kv = mcxHashSearch(fname, wrtTable_g, MCX_DATUM_FIND)

   ;  if (!kv)
      {  xf = mcxIOnew(fname->str, "w")
      ;  kv = mcxHashSearch(fname, wrtTable_g, MCX_DATUM_INSERT)
      ;  kv->val = xf
   ;  }
      else
      {  xf = (mcxIO*) kv->val
      ;  if (!xf->fp)            /* used by writeto wrapper */
         mcxIOrenew(xf, NULL, "a")
      ;  mcxTingFree(&fname)
   ;  }

      if (!xf->fp && mcxIOopen(xf, RETURN_ON_FAIL) != STATUS_OK)
      {  yamErr
         (  "yamOutputNew"
         ,  "can not open file <%s> for writing\n"
         ,  s
         )
      ;  if (!(kv = mcxHashSearch(xf->fn, wrtTable_g, MCX_DATUM_DELETE)))
         {  yamErr
            (  "yamOutputNew panic"
            ,  "can not find IO object for stream <%s>"
            ,  xf->fn->str
            )
         ;  return NULL
      ;  }
         fname = ((mcxTing*) (kv->key))
      ;  mcxTingFree(&fname)
         /* might have xf->usr (in renewal case) should free it as well
          * (check though)
         */
      ;  mcxIOfree(&xf)
      ;  return NULL
   ;  }

      if (!xf->usr)
      xf->usr = sinkNew(xf)
   ;  else
      filterSetFP(((sink*) xf->usr)->fd, xf->fp)
            /* this case is relevant for "a"(ppend) renewal (only),
             * as the fp will have changed
            */

   ;  return xf
;  }


void yamIOfree_v
(  void*    xf_v
)
   {  mcxIO* xf =  xf_v
   ;  sinkFree((sink*) xf->usr)
   ;  xf->usr = NULL
   ;  mcxIOrelease(xf)
   ;  mcxTingFree(&xf->buffer)         /* fixme: crosses boundary */
;  }


void mod_sink_exit
(  void
)
   {  mcxHashFree(&wrtTable_g, mcxTingRelease, yamIOfree_v)
   ;  mcxListFree(&(busystack_g->sp), NULL)
;  }


#if 1
void sink_stack_debug
(  void
)
   {  mcxLink* lk = busystack_g->sp

   ;  while (lk)
      {  busysink* bs = lk->val
      ;  if (bs && bs->sk)
         fprintf
         (  stderr
         ,  "name [%s] fd [%p]\n"
         ,  bs->sk->fname, (void*) bs->sk->fd
         )
      ;  lk = lk->prev
   ;  }
   }
#endif

void sinkSetFNOUT
(  void
)
   {  yamKeySet("__fnout__", sink_default_g->fname)
;  }


busysink* new_busy_sink
(  void
)
   {  return mcxAlloc(sizeof(busysink), EXIT_ON_FAIL)
;  }

/* NOTE.
 *    In the stack of sinks the same sink may occur multiple times.
 *    when we issue writeto (redirect current default output stream)
 *    it has to be updated in all places.

 *    This design may be a case of legacy overengineering. Need
 *    an example where the stack is actually *used*
*/

void sinkPush
(  sink*    new_active_sink
,  int      fltidx
)
   {  mcxLink* busylk=  busystack_g->sp
   ;  busysink* curr =  busylk->val
   ;  busysink* next =  NULL

   ;  mcxbool write_to  =  fltidx < 0
   ;  mcxbool change_default = write_to || !sink_default_g

;if(0)fprintf(stderr, "boot=%d  writeto=%d  new=%d\n", sink_default_g ? 0 : 1, write_to ? 1 : 0, new_active_sink ? 1 : 0)

#if DEBUG
;  fprintf(stderr, "=== %p %d\n", (void*) new_active_sink, fltidx)
#endif

   ;  if (change_default)
      {  if (sink_default_g && new_active_sink)
         while (busylk && curr)               /* find all occurrences of target */
         {  if (curr->sk == sink_default_g)
            curr->sk = new_active_sink    /* old sk is cached in wrtTable_g */
         ;  busylk = busylk->prev
         ;  curr = busylk->val
      ;  }

#if DEBUG
   fprintf (stderr, "--> change sink to %s\n", new_active_sink->fname)
#endif
      ;  if (!sink_default_g)
         sink_fltidx_g  = fltidx
      ;  if (new_active_sink)
         sink_default_g = new_active_sink

      ;  sinkSetFNOUT()
   ;  }

      if (write_to)
      return                              /* otherwise push on stack */

   ;  next = new_busy_sink()              /* uninitialized */

   ;  if (!new_active_sink)               /* dofile or chunk (<- yamOutput) */
      next->sk = sink_default_g
   ;  else
      next->sk = new_active_sink

                                          /* next is now active sink */
   ;  busystack_g->sp = mcxLinkAfter(busystack_g->sp, next)
;  }



void sinkPop
(  sink* sk cpl__unused
)
   {  mcxLink* lk    =  busystack_g->sp->prev
   ;  busysink* curr =  busystack_g->sp->val  

#if DEBUG
;  fprintf
   (  stderr
   ,  "--> pop %s curr %s\n"
   ,  curr->sk->fname
   ,  sk ? sk->fname : "none"
   )
#endif

   ;  mcxFree(curr)
   ;  mcxLinkDelete(busystack_g->sp)
   ;  busystack_g->sp = lk
;  }


keyDict* sinkGetDLRtop
(  void
)
   {  sink* sk = ((busysink*) busystack_g->sp->val)->sk
   ;  mcxLink* sp = busystack_g->sp
;if(0)sink_stack_debug()
;if(0)fprintf(stderr, "DLRtop return %s\n", sk->fname)
   ;  return sk->dlrStack->top
;  }
   /*  return ((busysink*) busystack_g->sp->val)->sk->dlrStack->top */


keyDict* sinkGetDLRdefault
(  void
)
   {
;if(0)fprintf(stderr, "DLRdefault return %s\n", sink_default_g->fname)
   ;  return sink_default_g->dlrStack->top
;  }


dictStack* sinkGetDLR
(  void
)
   {  return ((busysink*) busystack_g->sp->val)->sk->dlrStack
;  }


sink* sinkGet
(  void
)
   {  return ((busysink*) busystack_g->sp->val)->sk
;  }


lblStack* sinkGetXML
(  void
)
   {  return ((busysink*) busystack_g->sp->val)->sk->xmlStack
;  }


const char* sinkGetDefaultName
(  void
)
   {  return sink_default_g->fname
;  }



mcxstatus sinkDictPush
(  const char* label
)
   {  dictStack* dlr = sinkGetDLR()
   ;  return dictStackPush(dlr, 16, label)
;  }


mcxstatus sinkDictPop
(  const char* name
)
   {  dictStack* dlr = sinkGetDLR()
   ;  return dictStackPop(dlr, "dollar", name)
;  }


void sinkULimit
(  dim   size
)
   {  n_dict_dollar_max = size
;  }


#ifdef DEBUG_BU
#  define DEBUG DEBUG_BU
#  undef DEBUG_BU
#endif

