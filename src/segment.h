/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_segment_h__
#define zoem_segment_h__

#include "tingea/ting.h"
#include "tingea/types.h"

#define SEGMENT_CONSTANT   1
#define SEGMENT_DIGEST     2
#define SEGMENT_ERROR      4
#define SEGMENT_THROW      8
#define SEGMENT_DONE      16
#define SEGMENT_INTERRUPT  (SEGMENT_ERROR | SEGMENT_THROW | SEGMENT_DONE)
#define SEGMENT_CATCH      (SEGMENT_ERROR | SEGMENT_THROW)


typedef struct yamSeg
{  mcxTing           *txt
;  int               offset   /* consider txt only from offset onwards  */
;  int               idx      /* index of seg itself */
;  struct yamSeg*    prev
;  int               flags    /* interrupt, constant, digest */
;
}  yamSeg            ;

void yamSegUlimit
(  int   segment
,  int   stack
)  ;


/*
 * If caller proceeds with return value, it must check
 * whether SEGMENT_ERROR is set, and if so, abort further processing.
*/

yamSeg* yamSegPush
(  yamSeg*     prev_seg
,  mcxTing      *txt
)  ;

yamSeg* yamSegPushx
(  yamSeg*  prev_seg
,  mcxTing*  txt
,  int      flags
)  ;

/*
 * This one is used to make sure output is properly flushed
 * before escalating exceptions/errors.
 * This needs better documentation though.
*/
yamSeg*  yamSegPushEmpty
(  yamSeg*  prevseg
)  ;

yamSeg* yamStackPush
(  mcxTing*  txt
)  ;

/*
 * Use this e.g. for temporary use with yamParseScopes, and when it
 * is certain that no unbounded recursion of yamStackPushTmp can occur.
*/

yamSeg* yamStackPushTmp
(  mcxTing*  txt
)  ;

void  yamStackFreeTmp
(  yamSeg   **segpp
)  ;


void yamSegInit
(  yamSeg*  this_seg
,  mcxTing*  txt
)  ;

void  yamSegFree
(  yamSeg   **segpp
)  ;

void seg_check_ok
(  mcxbool ok
,  yamSeg* seg
)  ;

void seg_check_flag
(  mcxbits flag_zoem
,  yamSeg* seg
)  ;

void yamStackFree
(  yamSeg   **segpp
)  ;

int yamStackIdx
(  void
)  ;

typedef yamSeg* (*xpnfnc)(yamSeg* seg);

void mod_segment_init
(  void
)  ;

void mod_segment_exit
(  void
)  ;

#endif

