/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under
 * the terms of the GNU General Public License;  either version 3 of the
 * License or (at your option) any later  version.  You should have received a
 * copy of the GPL along with Zoem, in the file COPYING.
*/

#include <string.h>

#include "digest.h"
#include "filter.h"
#include "parse.h"
#include "iface.h"
#include "segment.h"
#include "util.h"

#include "tingea/ting.h"

/*
 *    creates successively new segs, and these segs
 *    and their content will be freed. The input txt is left
 *    alone however, and is responsibility of caller.
*/

mcxbits yamOutput
(  mcxTing       *txtin
,  sink*          sk
,  int            fltidx
)
   {  yamSeg   *seg
   ;  filter* fd = sk ? sk->fd : NULL

   ;  if (!txtin)
      yamErrx(1, "yamOutput PBD", "void argument")

   ;  sinkPush(sk, fltidx)

   ;  seg = yamStackPush(txtin)

   ;  if (seg->flags & SEGMENT_ERROR)

   ;  else
      while(seg)
      {  int  prev_offset  =  seg->offset
      ;  yamSeg* prev_seg  =  seg
      ;  mcxbool dofilter  =  fltidx
      
      ;  int offset        =  yamFindKey(seg)
      ;  mcxbool done      =  offset == -1
      ;  mcxbool err       =  offset == -2
      ;  int len

      ;  if (err)
         break

      ;  if (done)
         offset = seg->txt->len
      
      ;  len = offset - prev_offset

      ;  if (tracing_g & ZOEM_TRACE_OUTPUT)
         {  fprintf
            (  stdout
            ,  "----.----. %s seg %d stack %d offset %d len %d txt len %d\n"
            ,  dofilter ? "filtering" : "skipping"
            ,  (int) seg->idx
            ,  (int) yamStackIdx()
            ,  (int) prev_offset
            ,  (int) len
            ,  (int) seg->txt->len
            )
         ;  if (dofilter && len)
            fputs("  ,,|", stdout)
      ;  }

         if (len && dofilter && flts[fltidx](fd, seg->txt, prev_offset, len))
         break

      ;  if (seg && seg->flags & SEGMENT_INTERRUPT)
         break

      ;  if (tracing_g & ZOEM_TRACE_OUTPUT)
         fprintf
         (  stdout
         ,  "%s----^----^ output %s seg %d and stack %d\n"
         ,  dofilter && len ? "\n" : ""
         ,  done ? "finished" : "continuing"
         ,  (int) seg->idx
         ,  (int) yamStackIdx()
         )

      ;  if (done)
         {  seg  =  seg->prev
         ;  yamSegFree(&prev_seg)
      ;  }
         else
         {  yamSeg* newseg  = yamDoKey(seg)
         ;  if (newseg)
            seg = newseg
         ;  else
            {  seg->flags |= SEGMENT_ERROR
            ;  break
         ;  }
         }
      }

      sinkPop(sk)

   ;  if (seg)    /* thrown or error */
      {  mcxbits flags = seg->flags & SEGMENT_INTERRUPT
      ;  yamStackFree(&seg)
      ;  return flags
   ;  }

      return 0
;  }


mcxbits  yamDigest
(  mcxTing      *txtin
,  mcxTing      *txtout
,  yamSeg       *baseseg
)
   {  yamSeg     *seg
   ;  mcxbool     newscratch  =  txtin == txtout ? TRUE : FALSE
   ;  mcxbits     irupt = 0

   ;  mcxTing*    txt = newscratch ? NULL : txtout

   ;  if (!txtin)
      yamErrx(1, "yamDigest PBD", "void argument")

   ;  if (txtin == txtout && !strchr(txtin->str, '\\'))
      return 0

   ;  txt  =  mcxTingEmpty(txt, 30)

   ;  seg = yamStackPush(txtin)
   ;  seg->flags |=  SEGMENT_DIGEST

   ;  if (seg->flags & SEGMENT_ERROR)

   ;  else
      while(seg)
      {  int prev_offset   =  seg->offset
      ;  yamSeg* prev_seg  =  seg

      ;  int offset        =  yamFindKey(seg)
      ;  mcxbool done      =  offset == -1
      ;  mcxbool err       =  offset == -2
      ;  int len

      ;  if (err)
         break

      ;  if (done)
         offset = seg->txt->len

      ;  len = offset - prev_offset

      ;  if (tracing_g & ZOEM_TRACE_OUTPUT)
         fprintf
         (  stdout
         ,  "----.----. "
            "appending seg %d stack %d offset %d length %d text length %d%s"
         ,  (int) seg->idx
         ,  (int) yamStackIdx()
         ,  (int) prev_offset
         ,  (int) len
         ,  (int) seg->txt->len
         ,  len ? "\n(dat(" : "\n"
         )

      ;  if (len)
         mcxTingNAppend
         (  txt
         ,  seg->txt->str+prev_offset
         ,  len
         )

      ;  if (seg && seg->flags & SEGMENT_INTERRUPT)
         break

      ;  if (tracing_g & ZOEM_TRACE_OUTPUT)
         {  if (len)
            traceputlines(seg->txt->str+prev_offset, len)
         ;  fprintf
            (  stdout
            ,  "----^----^ digest %s seg %d and stack %d\n"
            ,  done ? "finished" : "continuing"
            ,  (int) seg->idx
            ,  (int) yamStackIdx()
            )
      ;  }

         if (done)
         {  seg  =  seg->prev
         ;  yamSegFree(&prev_seg)
      ;  }
         else
         {  yamSeg* newseg  = yamDoKey(seg)
         ;  if (newseg)
            seg = newseg
         ;  else
            {  seg->flags |= SEGMENT_ERROR
            ;  break
         ;  }
         }
      }

      if (newscratch)
      {  mcxTingWrite(txtin, txt->str)
      ;  mcxTingFree(&txt)
   ;  }

      if (seg)    /* thrown or error */
      {  irupt = seg->flags & SEGMENT_INTERRUPT
      ;  if (baseseg)
         baseseg->flags |= irupt
      ;  yamStackFree(&seg)
   ;  }

      return irupt
;  }


