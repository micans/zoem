/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <stdarg.h>

#include "util.h"

#include "segment.h"
#include "key.h"
#include "ops.h"
#include "source.h"
#include "digest.h"
#include "parse.h"

#include "tingea/ting.h"
#include "tingea/err.h"
#include "tingea/hash.h"
#include "tingea/types.h"
#include "tingea/alloc.h"
#include "tingea/list.h"
#include "tingea/io.h"


static FILE* fperr = NULL;
static u32 (*tinghash_g)(const void* str) = mcxTingFNVhash;

mcxstatus yamHashFie
(  const char* id
)
   {  if (!(tinghash_g = mcxTingHFieByName(id)))
      return STATUS_FAIL
   ;  return STATUS_OK
;  }


mcxHash* yamHashNew
(  dim n
)
   {  return mcxHashNew(n, tinghash_g, mcxTingCmp)
;  }


void yamErrorFile
(  FILE* fp
)
   {  fperr = fp
;  }


void yamStats
(  void
)
   {  fprintf(stdout, "Zoem user table stats\n")
   ;  yamKeyStats()
   ;  fprintf(stdout, "Zoem primitive table stats\n")
   ;  yamOpsStats()
   ;  fprintf(stdout, "Scratch lengths\n")
   ;  yamScratchStats()
;  }


void  yam_err_
(  const char  *caller
,  const char  *fmt
,  va_list     *args
)
   {  const char* fname = sourceGetName()
   ;  FILE* fp = fperr ? fperr : stderr
   ;  if (*fname)
      mcxErrf
      (  fp
      ,  "zoem"
      ,  "error around input line <%ld> in <%s>"
      ,  sourceGetLc()
      ,  fname
      )
   ;  else
      mcxErrf
      (  fp
      ,  "zoem"
      ,  "error occurred"
      )
   ;  if (key_g->str)
      mcxErrf(fp, NULL, "last key seen is <%s>", key_g->str)

   ;  if (caller)
      fprintf(fp, "___ [%s] ", caller)
   ;  else
      fprintf(fp, "___ ")
   ;  vfprintf(fp, fmt, *args)
   ;  fprintf(fp, "\n")
;  }


void yamErrx
(  int         status
,  const char  *caller
,  const char  *fmt
,  ...
)
   {  va_list  args

   ;  va_start(args, fmt)
   ;  yam_err_(caller, fmt, &args)
   ;  va_end(args)
   ;  if (status)
      mcxExit(status)
;  }


void yamErr
(  const char  *caller
,  const char  *fmt
,  ...
)
   {  va_list  args

   ;  va_start(args, fmt)
   ;  yam_err_(caller, fmt, &args)
   ;  va_end(args)
;  }


void ting_stack_push
(  lblStack*   st
,  char*    str
,  int      len
)
   {  mcxLink* lk = st->sp
   ;  lk    =     lk->next
               ?  lk->next
               :  mcxLinkAfter(lk, NULL)
   ;  lk->val = mcxTingNWrite(lk->val, str, len)   /* fixme (NULL return) */
   ;  st->sp = lk
;  }


const char* ting_stack_pop
(  lblStack*   st
)
   {  const char* str = NULL
   ;  mcxLink* lk = st->sp

   ;  if (lk->prev)
      {  str =  ((mcxTing*) lk->val)->str
      ;  lk = lk->prev
      ;  st->sp = lk
   ;  }
      return str
;  }


void ting_stack_free
(  lblStack** stackp
)
   {  lblStack* stack = *stackp
   ;  mcxLink* lk = stack->sp
   ;  mcxTing* t

   ;  while (lk->next)
      lk = lk->next

   ;  while (lk->prev)
         t = (mcxTing*) lk->val
      ,  mcxTingFree(&t)
      ,  lk = lk->prev

   ;  mcxListFree(&lk, NULL)
   ;  mcxFree(stack)
   ;  *stackp = NULL
;  }


lblStack* ting_stack_new
(  int capacity
)
   {  lblStack* stack = mcxAlloc(sizeof(stack), EXIT_ON_FAIL)
   ;  stack->sp = mcxListSource(capacity, MCX_GRIM_ARITHMETIC)
   ;  return stack
;  }


void ting_stack_init
(  lblStack* stack
,  int capacity
)
   {  stack->sp = mcxListSource(capacity, MCX_GRIM_ARITHMETIC)
;  }


/* TODO
*/

void ting_tilde_expand
(  mcxTing* pat
,  mcxbits  modes
)
   {  char* a = pat->str
   ;  char* b = a
   ;  mcxbool esc = FALSE
   ;  mcxbool ux = modes & YAM_TILDE_UNIX
   ;  mcxbool rx = modes & YAM_TILDE_REGEX

   ;  while (a < pat->str+pat->len)
      {  if (!esc && (unsigned char) *a == '~')
         {  esc = TRUE
         ;  a++
         ;  continue
      ;  }

         if (!esc)
         *b = *a
      ;  else
         {  mcxbool match = TRUE
         ;  if (ux)                          /* future: \C^A for control-A. */
            switch ((unsigned char) *a)
            {  case '~' : *b = '~'  ;                 break
            ;  case 'E' : *b = '\\' ;  *++b = '\\' ;  break
            ;  case 'I' : *b = '\\' ;  *++b = '{'  ;  break
            ;  case 'J' : *b = '\\' ;  *++b = '}'  ;  break
            ;  case 'e' : *b = '\\'                ;  break
            ;  case 'i' : *b = '{'                 ;  break
            ;  case 'j' : *b = '}'                 ;  break
            ;
               case 'a' :  case 'b' :  case 'f' :  case 'n' :  case 'r'
            :  case 't' :  case 'v'
            :  case '0' :  case '1' :  case '2' :  case '3'
            :  case 'x'                                        /* zoem feature */
            :           *b = '\\' ;  *++b = *a     ;  break
            ;  
               default
            :           match = FALSE ; break
         ;  }
            if (!match && rx)
            switch ((unsigned char) *a)
            {  case '^' :  case '.' :  case '[' :  case '$' :  case '('
            :  case ')' :  case '|' :  case '*' :  case '+' :  case '?'
            :
                        *b = '\\' ;  *++b = *a     ;  break
            ;  default
            :           *b = *a                    ;  break
         ;  }
            else if (!match)
            *b = *a

         ;  esc = FALSE
      ;  }
         a++
      ;  b++
   ;  }
      pat->len = b - pat->str
   ;  *b = '\0'
;  }


