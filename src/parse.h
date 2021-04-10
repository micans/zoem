/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_parse_h__
#define zoem_parse_h__

#include "segment.h"

#include "tingea/ting.h"

#define  YAM_ARG_MAX 10

extern mcxTing      key_and_args_g[];

extern mcxTing*     key_g;

extern mcxTing*     arg1_g;
extern mcxTing*     arg2_g;
extern mcxTing*     arg3_g;
extern mcxTing*     arg4_g;
extern mcxTing*     arg5_g;
extern mcxTing*     arg6_g;
extern mcxTing*     arg7_g;
extern mcxTing*     arg8_g;
extern mcxTing*     arg9_g;
extern mcxTing*     arg10_g;

extern int          n_args_g;
extern int          tracing_g;

yamSeg*  yamDoKey
(  yamSeg *seg
)  ;


int yamFindKey
(  yamSeg*  seg
)  ;


int yamParseScopes
(  yamSeg*  seg
,  int      n
,  int      delta
)  ;

char* yamParseKV
(  yamSeg*   seg
)  ;

int yamCountScopes
(  mcxTing* txt
,  int offset      
)  ;

/* ****************************************************************** */

/* currently uses only KEY_ANON and KEY_PRIME */

yamSeg* yamExpandKey
(  yamSeg*  seg
,  mcxbits  keybits
)  ;

yamSeg*  yamFormatted1
(  yamSeg*  seg
,  const char* arg
)  ;

                                    /* fixme: unused clean up needed? */
#define  KEY_XML           1
#define  KEY_META          2
#define  KEY_DATA          4
#define  KEY_ANON          8
#define  KEY_PRIME        16
#define  KEY_DOLLAR       32
#define  KEY_QUOTE        64
#define  KEY_WORD        128
#define  KEY_GLOBAL      256


#define  KEY_CALLBACK (KEY_ANON | KEY_WORD | KEY_DOLLAR | \
                           KEY_QUOTE | KEY_PRIME | KEY_XML | KEY_WORD)
#define  KEY_ZOEMONLY (KEY_XML | KEY_META | KEY_DATA | KEY_PRIME)

/* return length of signature.
 * For some reason, k can be NULL (caller is not interested)
 * namelen_p and keybits_p must be valid.
*/
int checkusrsig
(  char* p
,  int   len
,  int*  k
,  int*  namelen_p
,  mcxbits* keybits_p
)  ;

int checkusrname
(  char* p
,  int   len
,  mcxbits*  keybits
)  ;

int checkusrtag
(  char* p
,  int   len
,  int*  k
)  ;

int checkusrcall
(  mcxTing* txt
,  int offset
)  ;

int seescope
(  char* p
,  int   len
)  ;


void mod_parse_init
(  int   traceflags
)  ;

void mod_parse_exit
(  void
)  ;

int yamTracingSet
(  int  traceflags
)  ;

void traceputlines
(  const char* s
,  int len
)  ;

void traceput
(  const char* s
,  const mcxTing* txt
)  ;

void yamScratchStats
(  void
)  ;

#endif

