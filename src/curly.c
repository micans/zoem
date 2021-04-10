/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include "curly.h"
#include "util.h"
#include "source.h"

#include "tingea/ting.h"
#include "tingea/types.h"
#include "tingea/compile.h"


mcxTing* yamBlock
(  const mcxTing* txt
,  int            offset
,  int*           lengthp
,  mcxTing*       dst
)
   {  int l = yamClosingCurly(txt, offset, NULL, RETURN_ON_FAIL)

   ;  if (lengthp)
      *lengthp = l

   ;  if (l <= 0)
      return NULL

   ;  return mcxTingNWrite(dst, txt->str+offset+1, l-1)
;  }


/*
 *    txt->str[offset] must be '{'.
 *    returns l such that txt->str[offset+l] == '}'.
*/

int yamClosingCurly
(  const mcxTing     *txt
,  int         offset
,  int*        linect
,  mcxOnFail   ON_FAIL
)
   {  char* o     =  txt->str + offset
   ;  char* p     =  o
   ;  char* z     =  txt->str + txt->len

   ;  int   n     =  1           /* 1 open bracket */
   ;  int   lc    =  0
   ;  int   esc   =  0
   ;  int   q

   ;  if (*p != '{')
      q = CURLY_NOLEFT
   ;  else
      {  while(++p < z)
         {  if (*p == '\n')
            lc++
         ;  if (esc)
            {  esc   =  0           /* no checks for validity */
            ;  continue
         ;  }
            switch(*p)
            {  case '\\'  :  esc = 1;  break
            ;  case '{'   :  n++    ;  break
            ;  case '}'   :  n--    ;  break
         ;  }
            if (!n)
            break
      ;  }
         q = n ? CURLY_NORIGHT : p-o
   ;  }

      if (linect)
      *linect += lc

   ;  if (q<0 && ON_FAIL == EXIT_ON_FAIL)
      yamScopeErr(NULL, "yamClosingCurly", q)

   ;  return q
;  }


void  yamScopeErr
(  yamSeg      *seg_unused cpl__unused
,  const char  *caller
,  int         error
)
   {  if (error == CURLY_NORIGHT)
      yamErr
      (  caller
      ,  "unable to close scope (starting around line <%ld> in file <%s>)"
      ,  sourceGetLc()
      ,  sourceGetName()
      )
   ;  else if (error == CURLY_NOLEFT)
         yamErr
         (  caller
         ,  "scope error (around input line <%ld> in file <%s>)"
         ,  sourceGetLc()
         ,  sourceGetName()
         )
      ,  yamErr(caller, "expected to see an opening '{'.")
   ;  mcxExit(1)
;  }



/*
 *    txt->str[offset] must be '{'.
 *    returns l such that txt->str[offset+l] == '}'.
*/

int yamClosingCube
(  const mcxTing     *txt
,  int         offset
,  int*        linect
,  mcxOnFail   ON_FAIL
)
   {  char* o     =  txt->str + offset
   ;  char* p     =  o
   ;  char* z     =  txt->str + txt->len

   ;  int   lc    =  0
   ;  int   q

   ;  if (*p != '<')
      q = CUBE_NOLEQ
   ;  else
      {  while(++p < z)
         {  if (*p == '\n')
            lc++
         ;  else if (*p == '<' || *p == '>')
            break
      ;  }
         q = *p == '<' ? CUBE_NEST : *p != '>' ? CUBE_NOGEQ : p-o
   ;  }

      if (linect)
      *linect += lc

   ;  if (q<0 && ON_FAIL == EXIT_ON_FAIL)
      yamScopeErr(NULL, "yamClosingGeq", q)

   ;  return q
;  }


int yamEOConstant
(  mcxTing     *txt
,  int         offset
)
   {  char* o     =  txt->str + offset
   ;  char* p     =  o
   ;  char* z     =  txt->str + txt->len

   ;  if (*p != '*')
      return(CONSTANT_NOLEFT)

   ;  if (p[1] == '{')
      {  int x = yamClosingCurly(txt,offset+1, NULL, RETURN_ON_FAIL)
      ;  return x > 0 ? x+1 : CONSTANT_NORIGHT
   ;  }
      
      while(++p < z)
      {
         if (*p == '\\' || *p == '{' || *p == '}')
         return (CONSTANT_ILLCHAR)

      ;  if (*p == '*')
         break
   ;  }
      return(*p == '*' ? p-o : CONSTANT_NORIGHT)
;  }


