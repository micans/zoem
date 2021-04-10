/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <stdio.h>

#include "tingea/ting.h"


int ENTRY_SPLIT    =  1;
int ENTRY_EXXIT    =  2;
int ENTRY_DEBUG    =  4;
int ENTRY_STDIA    =  8;
int ENTRY_STDIN    =  16;
int ENTRY_STDOUT   =  32;

int SYSTEM_SAFE    =  13   ;
int SYSTEM_UNSAFE  =  17   ;
int SYSTEM_UNSAFE_SILENT = 19 ;

mcxTing* system_allow =  NULL;

int systemAccess  =  13    ;        /* for ops.c, system#3  */
int systemHonor   =  0     ;        /* for ops.c, system#3  */
   
int stressWrite   =  0     ;        /* for ops.c, write#3  */
size_t chunk_size =  1 << 20  ;     /* for entry.c and ops.c, dofile#1  */

int ztablength_g  =  0     ;        /* for filter.c, \I step size */
int readlineOK    =  1     ;



char *tracestrings[]
=
      {  "trace in long mode, do not truncate lines"
#define  TRACE_LONG        1
      ,  "print keys"
#define  TRACE_KEYS        2
      ,  "print arguments"
#define  TRACE_ARGS        4
      ,  "print key definitions"
#define  TRACE_DEFS        8
      ,  "print scopes (varargs)"
#define  TRACE_SCOPES      16
      ,  "print segment information"
#define  TRACE_SEGS        32
      ,  "trace filtering and output"
#define  TRACE_OUTPUT      64
      ,  "trace regexes (\\inspect#4)"
#define  TRACE_REGEX       128
      ,  "trace IO"
#define  TRACE_IO          256
      ,  "trace spawned system calls"
#define  TRACE_SYSTEM      512
      ,  "trace let (arithmetic module)"
#define  TRACE_LET         1024
      ,  "print hash table stats at exit"
#define  TRACE_HASH        2048
      ,  "trace primitives in user syntax"
#define  TRACE_PRIME       4096
      }  ;

int n_tracebits         = sizeof(tracestrings) / sizeof(char*);

#define  TRACE_PREVIOUS    -1
#define  TRACE_DEFAULT     TRACE_KEYS | TRACE_ARGS
#define  TRACE_ALL         ((1 << 11) - 2)
#define  TRACE_ALL_LONG    ((1 << 11) - 1)

int ZOEM_TRACE_LONG     =  TRACE_LONG    ;
int ZOEM_TRACE_KEYS     =  TRACE_KEYS    ;
int ZOEM_TRACE_ARGS     =  TRACE_ARGS    ;
int ZOEM_TRACE_DEFS     =  TRACE_DEFS    ;
int ZOEM_TRACE_SCOPES   =  TRACE_SCOPES  ;
int ZOEM_TRACE_SEGS     =  TRACE_SEGS    ;
int ZOEM_TRACE_OUTPUT   =  TRACE_OUTPUT  ;
int ZOEM_TRACE_REGEX    =  TRACE_REGEX   ;
int ZOEM_TRACE_HASH     =  TRACE_HASH    ;
int ZOEM_TRACE_IO       =  TRACE_IO      ;
int ZOEM_TRACE_SYSTEM   =  TRACE_SYSTEM  ;
int ZOEM_TRACE_LET      =  TRACE_LET     ;
int ZOEM_TRACE_PRIME    =  TRACE_PRIME   ;

int ZOEM_TRACE_PREVIOUS = TRACE_PREVIOUS;
int ZOEM_TRACE_DEFAULT  = TRACE_DEFAULT ;
int ZOEM_TRACE_ALL      = TRACE_ALL     ;
int ZOEM_TRACE_ALL_LONG = TRACE_ALL_LONG;


void showTracebits
(  void
)
   {  int i
   ;  for (i=0;i<n_tracebits;i++)
      {  fprintf(stdout, "%6d  %s\n", 1<<i, tracestrings[i])
   ;  }
   }


void mod_iface_exit
(  void
)
   {  mcxTingFree(&system_allow)
;  }


