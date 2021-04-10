/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include "segment.h"
#include "util.h"
#include "iface.h"
#include "parse.h"
#include "digest.h"     /* for STATUS_THROW only, move that someplace else */
#include "zlimits.h"


#include "tingea/ting.h"
#include "tingea/alloc.h"
#include "tingea/gralloc.h"

static int segment_depth_max  =  200;
static int stackidx_g         =  -1;
static int stack_depth_max    =  100;

static mcxGrim* src_seg_g     =  NULL;


void yamSegUlimit
(  int   segment
,  int   stack
)
   {  if (segment >= 0)
      segment_depth_max = segment
   ;  if (stack >= 0)
      stack_depth_max = stack
;  }


int yamStackIdx
(  void
)
   {  return stackidx_g
;  }


void yamSegFree
(  yamSeg   **segpp
)
   {  yamSeg* seg = *segpp

   ;  if (!seg)
      return

   ;  if (seg->idx && !seg->txt)
      yamErrx(1, "yamSegFree PBD", "no txt at seg idx <%d>\n", seg->idx)

   ;  if (seg->idx)            /* the first segment txt is not freed */
      mcxTingFree(&(seg->txt))

   ;  if (!seg->prev)
      stackidx_g--

   ;  mcxGrimLet(src_seg_g, seg)
   ;  *segpp = NULL
;  }


void yamSegInit
(  yamSeg*  this_seg
,  mcxTing*  txt
)
   {  this_seg->txt        =  txt
   ;  this_seg->prev       =  NULL
   ;  this_seg->offset     =  0
   ;  this_seg->idx        =  0
   ;  this_seg->flags      =  0
;  }


yamSeg* yamStackPush
(  mcxTing* txt
)
   {  yamSeg* newseg = yamSegPush(NULL, txt)
   ;  stackidx_g++

   ;  if (stack_depth_max && stackidx_g > stack_depth_max)
      {  yamErr
         (  "yamStackPush"
         ,  "exceeding maximum stack depth <%d>"
         ,  stack_depth_max
         )
      ;  newseg->flags |= SEGMENT_ERROR
   ;  }
      return newseg
;  }


void yamStackFree
(  yamSeg   **segpp
)
   {  yamSeg* seg = *segpp
   ;  if (!seg)
      return

   ;  while(seg)
      {  yamSeg* down = seg->prev
      ;  yamSegFree(&seg)
      ;  seg = down
   ;  }
;  }


yamSeg* yamStackPushTmp
(  mcxTing*  txt
)
   {  yamSeg* newseg
   ;  stack_depth_max++
   ;  newseg = yamStackPush(txt)
   ;  return newseg
;  }

void yamStackFreeTmp
(  yamSeg   **segpp
)
   {  yamStackFree(segpp)
   ;  stack_depth_max--
;  }


yamSeg*  yamSegPushEmpty
(  yamSeg*  prevseg
)
   {  return yamSegPush(prevseg, mcxTingEmpty(NULL, 0))
;  }


yamSeg*  yamSegPush
(  yamSeg*  prevseg
,  mcxTing*  txt
)
   {  yamSeg* next_seg  =  mcxGrimGet(src_seg_g)
   ;  int new_idx       =  prevseg ? prevseg->idx + 1  :  0 
   ;  int new_flags     =  prevseg ? prevseg->flags    :  0 

      /* don't print the entire entry file grm: we still print other files
       * fixmefixme
      */
   ;  if (tracing_g & ZOEM_TRACE_SEGS && (new_idx || stackidx_g))
      {  fprintf(stdout, " seg| %d stack %d\n", new_idx, stackidx_g)  
      ;  traceput("dat", txt)
   ;  }

      next_seg->txt        =  txt
   ;  next_seg->offset     =  0
   ;  next_seg->idx        =  new_idx
   ;  next_seg->prev       =  prevseg
   ;  next_seg->flags      =  new_flags

   ;  if (segment_depth_max && new_idx > segment_depth_max)
      {  yamErr
         (  "yamSegPush"
         ,  "exceeding maximum segment depth <%d>"
         ,  segment_depth_max
         )
      ;  next_seg->flags |= SEGMENT_ERROR
   ;  }

      return next_seg
;  }


yamSeg* yamSegPushx
(  yamSeg*   prevseg
,  mcxTing*  txt
,  int       flags
)
   {  yamSeg* newseg   =  yamSegPush(prevseg, txt)
   ;  newseg->flags    =  prevseg->flags | flags
   ;  return newseg
;  }


/* set error if not already error or throw by digestion */

void seg_check_ok
(  mcxbool ok
,  yamSeg* seg
)
   {  if (!ok && !(seg->flags & SEGMENT_INTERRUPT))
      seg->flags |= SEGMENT_ERROR
;  }


void seg_check_flag
(  mcxbits flag_zoem
,  yamSeg* seg
)
   {  if (seg->flags & SEGMENT_INTERRUPT)    /* already set */
      return
   ;  seg->flags |= flag_zoem
;  }


void mod_segment_init
(  void
)
   {  src_seg_g = mcxGrimNew(sizeof(yamSeg), 40, MCX_GRIM_ARITHMETIC)
;  }


void mod_segment_exit
(  void
)
   {  mcxGrimFree(&src_seg_g)
;  }


