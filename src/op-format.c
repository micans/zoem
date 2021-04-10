/*   (C) Copyright 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/


/* TODO
 *    remove idiotic justify shift twitch
*/

#include "op-format.h"

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include "digest.h"
#include "key.h"
#include "curly.h"
#include "segment.h"
#include "read.h"
#include "parse.h"
#include "iface.h"

#include "tingea/alloc.h"
#include "tingea/ting.h"
#include "tingea/ding.h"
#include "tingea/err.h"


/* TODO
 *
 * sth to specify no padding at right side for centered
 * and substring alignment.  (to remove trailing spaces)
 *
 * for padding, apply length key - tricky with truncation.
*/


long includein
(  long num
,  long lft
,  long rgt
)
   {  if (num < lft)
      return lft
   ;  if (num > rgt)
      return rgt
   ;  return num
;  }


enum
{  PADPATTERN = 0
,  DELIMLEFT
,  DELIMRIGHT
,  ATPIVOT
,  ATNUM
,  LENKEY
,  LENARG
,  SUBS
,  BORDERLEFT
,  BORDERRIGHT
,  N_FORMAT_PARAMS         /* this provides interface for loads of directives */
}  ;

enum
{  SUB_ARG = 0
,  SUB_DELIMLEFT
,  SUB_DELIMRIGHT
,  N_FORMAT_SUBS           /* interface for arguments to {subs} directive */
}  ;

#define parlen(x) (par+x)->len
#define parstr(x) (par+x)->str
#define thepar(x) (par+x)  /* we pass/name par across multiple routines, beware */

#define thesub(x) (sub+x)



static int ptp             /* a.k.a. or s.f. parsethepart */
(  mcxTing* fmt
,  int      offs
,  ofs*     justify
,  ofs*     width
,  int*     repeat
,  mcxTing* scr
,  mcxTing* par            /* size N_FORMAT_PARAMS */
)
   {  char* q              /* find first % occurrence which is not %% */
   ;  char* p           =  fmt->str+offs
   ;  mcxTing*  msg     =  NULL
   ;  mcxTing*  part    =  NULL

                     /* general fixme: convoluted success logic */
   ;  *justify = 4;  /* fixme: insane justify logic (1,2,4,12, >> 3) */

   ;  mcxTingEmpty(scr, 0)

   ;  while((q = strchr(p, '%')))
      {  if ((uchar) q[1] == '%')
         {  mcxTingNAppend(scr, p, q+1-p)   /* include one percent sign .. */
         ;  p = q+2                         /* .. and skip the second one  */
         ;  continue
      ;  }
         else if ((uchar) q[1] == '{')
         {  int cc = yamClosingCurly(fmt, q+1-fmt->str, NULL, RETURN_ON_FAIL)
         ;  if (cc < 0)
            {  msg = mcxTingPrint(NULL, "no closing scope")  
            ;  break
         ;  }
            mcxTingNAppend(scr, p, q-p)
         ;  part =   mcxTingNNew(q+2, cc-1)
         ;  offs =   q+1+cc+1 - fmt->str
      ;  }
         else
         msg = mcxTingPrint(NULL, "format sequence not recognized")
      ;  break
   ;  }

      if (!q)
      {  mcxTingNAppend(scr, p, fmt->len - (p - fmt->str))
      ;  return 0
   ;  }

      if (part)
      {  yamSeg* segpart =  yamStackPushTmp(part)
      ;  char* key
      ;  while ((key = yamParseKV(segpart)))
         {  if (!strcmp(key, "align") && n_args_g == 1)
            {  if (strstr(arg1_g->str, "left"))
               *justify |= 1
            ;  else if (strstr(arg1_g->str, "right"))
               *justify |= 2
            ;  else if (strstr(arg1_g->str, "center"))
               *justify = 12
            ;  else
               yamErr("ignoring align mode <%s>", arg1_g->str)
         ;  }
            else if (!strcmp(key, "padding") && n_args_g == 1)
            mcxTingNWrite(thepar(PADPATTERN), arg1_g->str, arg1_g->len)
         ;  else if (!strcmp(key, "delimit"))
            {  if (n_args_g == 2)
               {  mcxTingNWrite(thepar(DELIMLEFT), arg1_g->str, arg1_g->len)
               ;  mcxTingNWrite(thepar(DELIMRIGHT), arg2_g->str, arg2_g->len)
            ;  }
               else if (n_args_g == 1)
               {  mcxTingNWrite(thepar(DELIMLEFT), arg2_g->str, arg2_g->len)
               ;  mcxTingNWrite(thepar(DELIMRIGHT), arg2_g->str, arg2_g->len)
            ;  }
         ;  }
            else if (!strcmp(key, "border") && n_args_g == 2)
            {  mcxTingNWrite(thepar(BORDERLEFT), arg1_g->str, arg1_g->len)
            ;  mcxTingNWrite(thepar(BORDERRIGHT), arg2_g->str, arg2_g->len)
         ;  }
            else if (!strcmp(key, "alignat") && n_args_g == 2)
            {  mcxTingNWrite(thepar(ATPIVOT), arg1_g->str, arg1_g->len)
            ;  mcxTingNWrite(thepar(ATNUM), arg2_g->str, arg2_g->len)
         ;  }
            else if (!strcmp(key, "double") && n_args_g == 1)
            {  mcxTingNWrite(thepar(SUBS), arg1_g->str, arg1_g->len)
                     /* subs takes a vararg
                      * {arg}{foo}{left}{bar}{right}{zut}
                      * et cetera.
                     */
         ;  }
            else if (!strcmp(key, "length") && n_args_g >= 0)
            {  int i
            ;  mcxTingNWrite(thepar(LENKEY), arg1_g->str, arg1_g->len)
            ;  for (i=2;i<=n_args_g;i++)     /* mixed sign comparison */
               mcxTingPrintAfter
               (thepar(LENARG), "{%s}", key_and_args_g[i].str)
         ;  }
            else if (!strcmp(key, "width") && n_args_g == 1)
            *width = atoi(arg1_g->str)
         ;  else if (!strcmp(key, "reuse") && n_args_g == 1)
            {  if (!strcmp(arg1_g->str, "*"))
               *repeat = -1
            ;  else
               *repeat = atoi(arg1_g->str)
         ;  }
            else
            yamErr
            (  "\\format#2"
            ,  "skipping unrecognized directive {%s} with %d arguments"
            ,  key
            ,  (int) n_args_g
            )
         ;  mcxFree(key)
         ;  if (msg)
            break
      ;  }
         yamStackFreeTmp(&segpart)
      ;  mcxTingFree(&part)
   ;  }

      if (msg)
      {  yamErr("\\format#2", msg->str)
      ;  offs = -1
   ;  }

      mcxTingFree(&msg)
   ;  return offs
;  }


      /* if lenkey && lenkey->len use it, otherwise use primitive length#1.
       * if (virarg && virarg->len) || !arg use virarg, otherwise use arg
      */
static dim get_length
(  const mcxTing* arg
,  const mcxTing* virarg
,  const mcxTing* lenkey
,  const mcxTing* lenarg
,  yamSeg* seg
)
   {  mcxTing* tmp
      =  mcxTingPrint
         (  NULL
         ,  "\\%s{%s}%s"
         ,  (lenkey && lenkey->len ? lenkey->str : "length")
         , ((!arg || (virarg && virarg->len))  ? virarg->str : arg->str)
         ,  (lenkey && lenkey->len ? lenarg->str : "")
         )
   ;  dim l = 1         /* defensive (given seg->flags) non-0 non-BIG value */

   ;  if (yamDigest(tmp, tmp, seg))
      NOTHING
   ;  else
      l = atoi(tmp->str)

   ;  mcxTingFree(&tmp)
   ;  return l
;  }


static mcxTing* unprotect_arg
(  const mcxTing* arg_in
)
   {  mcxTing* arg = mcxTingNNew(arg_in->str, arg_in->len)
   ;  yamUnprotect(arg)
   ;  return arg
;  }


static void pad_left
(  mcxTing*       scr
,  const mcxTing* pattern_arg
,  dim            width
)
   {  mcxTing* pattern = unprotect_arg(pattern_arg)
   ;  mcxTing* scr2 = NULL, *scr3 = NULL

   ;  scr2 = mcxTingKAppend(NULL, pattern->str, (width + pattern->len) / pattern->len)
   ;  mcxTingShrink(scr2, width)

   ;  scr3 = yamProtect(scr2)
   ;  mcxTingNAppend(scr, scr3->str, scr3->len)
   ;  mcxTingFree(&pattern)
   ;  mcxTingFree(&scr2)
   ;  mcxTingFree(&scr3)
;  }


static void pad_right
(  mcxTing* scr
,  const mcxTing* pattern_arg
,  dim      width
)
   {  mcxTing* pattern = unprotect_arg(pattern_arg)
   ;  mcxTing* scr2 = NULL, *scr3 = NULL

   ;  scr2 = mcxTingAppend(NULL, pattern->str + pattern->len - (width % pattern->len))
   ;  mcxTingKAppend(scr2, pattern->str, width / pattern->len)

   ;  scr3 = yamProtect(scr2)
   ;  mcxTingNAppend(scr, scr3->str, scr3->len)
   ;  mcxTingFree(&pattern)
   ;  mcxTingFree(&scr2)
   ;  mcxTingFree(&scr3)
;  }


#define SUBS_HAVE_ARG         1 << 0
#define SUBS_HAVE_DELIMLEFT   1 << 1
#define SUBS_HAVE_DELIMRIGHT  1 << 2


mcxbits fill_subs
(  mcxTing*    spec
,  mcxTing*    sub      /* size N_FORMAT_SUBS */
)
   {  yamSeg* tmpseg = yamStackPushTmp(spec)
   ;  mcxbits subs_on = 0
   ;  while (2 == yamParseScopes(tmpseg, 2, 0))
      {  if (!strcmp(arg1_g->str, "arg"))
            mcxTingNWrite(thesub(SUB_ARG), arg2_g->str, arg2_g->len)
         ,  subs_on |= SUBS_HAVE_ARG
      ;  else if (!strcmp(arg1_g->str, "delimit-left"))
            mcxTingNWrite(thesub(SUB_DELIMLEFT), arg2_g->str, arg2_g->len)
         ,  subs_on |= SUBS_HAVE_DELIMLEFT
      ;  else if (!strcmp(arg1_g->str, "delimit-right"))
            mcxTingNWrite(thesub(SUB_DELIMRIGHT), arg2_g->str, arg2_g->len)
         ,  subs_on |= SUBS_HAVE_DELIMRIGHT
   ;  }
      yamStackFreeTmp(&tmpseg)
   ;  return subs_on
;  }


yamSeg* yamFormat2
(  yamSeg*  seg
)
   {  mcxTing* fmt      =  mcxTingNew(arg1_g->str)
   ;  mcxTing* args     =  mcxTingNew(arg2_g->str)
   ;  mcxTing* out      =  mcxTingEmpty(NULL, 2*args->len)
   ;  mcxTing* scr1     =  mcxTingEmpty(NULL, 2*args->len)
   ;  mcxTing* scr2     =  mcxTingEmpty(NULL, args->len)
   ;  mcxTing* arg      =  mcxTingEmpty(NULL, args->len)

   ;  yamSeg* tmpseg    =  NULL
   ;  int x             =  0
   ;  int offs          =  0
   ;  mcxbool ok        =  FALSE    /* somewhat convoluted ok logic */
   ;  int repeat        =  0
   ;  int justify_c     =  0        /* cache */
   ;  int ll_field_c    =  0        /* cache */
   ;  int ll_align_offset_c =  0    /* cache */
   ;  int subs_on_c     =  0        /* cache */

   ;  mcxTing* par
      =  mcxNAlloc(N_FORMAT_PARAMS, sizeof(mcxTing), mcxTingInit, RETURN_ON_FAIL)

   ;  mcxTing* sub
      =  mcxNAlloc(N_FORMAT_SUBS, sizeof(mcxTing), mcxTingInit, RETURN_ON_FAIL)

   ;  do
      {  if (!par || !sub)
         break

      ;  if
         (  (yamDigest(args, args, seg))
         || (yamDigest(fmt, fmt, seg))
         )                          /* NOTE: fmt digested */
         break

      ;  offs = 0
      ;  tmpseg = yamStackPushTmp(args)

      ;  while (1)                  /* block longish, but it's not too bad */
         {  ofs justify = 0, ll_field = 0, ll_align_offset = 0
         ;  mcxbits subs_on = 0

         ;  ofs ll_arg              /* logical length */
         ;  ofs ll_dlm_left
         ;  ofs ll_dlm_right

         ;  ofs lo_dlm_left = 0     /* logical offset */

         ;  ok = FALSE
         ;  mcxTingEmpty(scr1, 0)
         ;  mcxTingEmpty(scr2, 0)

         ;  if (!repeat)
            {  mcxTingEmpty(thepar(PADPATTERN), 0)
            ;  mcxTingEmpty(thepar(DELIMLEFT), 0)
            ;  mcxTingEmpty(thepar(DELIMRIGHT), 0)
            ;  mcxTingEmpty(thepar(ATPIVOT), 0)
            ;  mcxTingEmpty(thepar(ATNUM), 0)
            ;  mcxTingEmpty(thepar(LENKEY), 0)
            ;  mcxTingEmpty(thepar(LENARG), 0)
            ;  mcxTingEmpty(thepar(SUBS), 0)
            ;  mcxTingEmpty(thepar(BORDERLEFT), 0)
            ;  mcxTingEmpty(thepar(BORDERRIGHT), 0)
                                    /* ^ perhaps mcxNApply is called for*/

            ;  mcxTingEmpty(thesub(SUB_ARG), 0)
            ;  mcxTingEmpty(thesub(SUB_DELIMLEFT), 0)
            ;  mcxTingEmpty(thesub(SUB_DELIMRIGHT), 0)

                                    /* scr1: write non-format part */
            ;  offs = ptp(fmt, offs, &justify, &ll_field, &repeat, scr1, par)

            ;  if (!parlen(PADPATTERN))
               mcxTingNWrite(thepar(PADPATTERN), " ", 1)

            ;  if (!offs)           /* trailing format part, no spec */
               {  ok = TRUE
               ;  mcxTingNAppend(out, scr1->str, scr1->len)
               ;  break
            ;  }
               else if (offs < 0)   /* error */
               break

            ;  if (parlen(SUBS))
               subs_on = fill_subs(thepar(SUBS), sub)

            ;  if (parlen(ATNUM))
               ll_align_offset = atoi(parstr(ATNUM))

            ;  ll_field = includein(ll_field, 0, 4096)
            ;  ll_align_offset = includein(ll_align_offset, 0, 4096)

            ;  if (repeat)
                  ll_field_c  =  ll_field
               ,  ll_align_offset_c = ll_align_offset
               ,  justify_c   =  justify
               ,  subs_on_c   =  subs_on
         ;  }
            else
            {  repeat--
            ;  ll_field =  ll_field_c
            ;  justify  =  justify_c
            ;  ll_align_offset=  ll_align_offset_c
            ;  subs_on  =  subs_on_c
         ;  }

            if ((x = yamParseScopes(tmpseg, 1, 0)) != 1)
            {  if (repeat < 0)      /* we are exhausting arguments */
               ok = TRUE
            ;  break
         ;  }

            mcxTingNWrite(arg, arg1_g->str, arg1_g->len)

         ;  ll_arg
            =  get_length
               (  subs_on & SUBS_HAVE_ARG ? NULL : arg
               ,  thesub(SUB_ARG)
               ,  thepar(LENKEY)
               ,  thepar(LENARG)
               ,  seg
               )

         ;  ll_dlm_left
            =  get_length
               (  subs_on & SUBS_HAVE_DELIMLEFT ? NULL : thepar(DELIMLEFT)
               ,  thesub(SUB_DELIMLEFT)
               ,  thepar(LENKEY)
               ,  thepar(LENARG)
               ,  seg
               )

         ;  ll_dlm_right
            =  get_length
               (  subs_on & SUBS_HAVE_DELIMRIGHT ? NULL : thepar(DELIMRIGHT)
               ,  thesub(SUB_DELIMRIGHT)
               ,  thepar(LENKEY)
               ,  thepar(LENARG)
               ,  seg
               )

         ;  if (seg->flags & SEGMENT_INTERRUPT)
            break

/* --> = --> = --> = --> = --> = --> = --> = --> = --> = --> = --> = --> = --> */
                                    /* case 1: align at a string */

                        /* alignat{pivot}{num} */
         ;  if (ll_align_offset)
            {  const mcxTing* thisarg = thesub(SUB_ARG)->len ? thesub(SUB_ARG) : arg
            ;  char* alignat  = strstr(thisarg->str, parstr(ATPIVOT))
                                    /* futurefixme: strstr
                                     * It should work even with \{ etc,
                                     * as long as caller is consistent.
                                    */

            ;  int padsize =  ll_field - ll_arg - (ll_dlm_left + ll_dlm_right)
            ;  ofs ll_prefix = 0

            ;  if (!alignat)
               alignat = thisarg->str + thisarg->len

                        /* say   thisarg->str =  123::456 then prefix will be 123::
                        */
            ;  {  mcxTing* prefix
                  = mcxTingNWrite(NULL, thisarg->str, alignat-thisarg->str+parlen(ATPIVOT))
               ;  ll_prefix = get_length(prefix, NULL, thepar(LENKEY), thepar(LENARG), seg)
               ;  mcxTingFree(&prefix)
            ;  }

               if (seg->flags & SEGMENT_INTERRUPT)
               break

            ;  if (padsize > 0)
               lo_dlm_left = ll_align_offset - ll_prefix - ll_dlm_left
         ;  }

                                    /* case 2: left align */
            else if (justify & 1)
            lo_dlm_left = 0

                                    /* case 3: right align */
         ;  else if (justify & 2)
            lo_dlm_left = ll_field - ll_arg - ll_dlm_left - ll_dlm_right


                                    /* case 4: center */
         ;  else
            {  int padsize = ll_field - ll_arg - (ll_dlm_left + ll_dlm_right)
            ;  int delta = ((justify >> 3) + ll_field) % 2
            ;  lo_dlm_left = ((padsize+delta) / 2)
         ;  }

/* --> = --> = --> = --> = --> = --> = --> = --> = --> = --> = --> = --> = --> */

            lo_dlm_left = includein(lo_dlm_left, 0, ll_field)

         ;  {  int ll      /* logical length */
            ;  if (lo_dlm_left > 0)
               pad_left(scr2, thepar(PADPATTERN), lo_dlm_left)

            ;  mcxTingNAppend(scr2, parstr(DELIMLEFT), parlen(DELIMLEFT))
            ;  mcxTingNAppend(scr2, arg->str, arg->len)
            ;  mcxTingNAppend(scr2, parstr(DELIMRIGHT), parlen(DELIMRIGHT))

            ;  ll = lo_dlm_left + ll_dlm_left + ll_arg + ll_dlm_right

            ;  if (ll < ll_field)
               pad_right(scr2, thepar(PADPATTERN), ll_field - ll)
         ;  }

            mcxTingNAppend(out, scr1->str, scr1->len)
         ;  mcxTingNAppend(out, parstr(BORDERLEFT), parlen(BORDERLEFT))
         ;  mcxTingNAppend(out, scr2->str, scr2->len)
         ;  mcxTingNAppend(out, parstr(BORDERRIGHT), parlen(BORDERRIGHT))
         ;  ok = TRUE
      ;  }
      }
      while (0)

                                 /* fixme or docme */
   ;  if (!ok && !(seg->flags & SEGMENT_INTERRUPT))
      seg->flags |= SEGMENT_ERROR

   ;  yamStackFreeTmp(&tmpseg)
   ;  mcxTingFree(&scr1)
   ;  mcxTingFree(&scr2)
   ;  mcxTingFree(&arg)

   ;  mcxNFree(par, N_FORMAT_PARAMS, sizeof(mcxTing), mcxTingRelease)
   ;  mcxNFree(sub, N_FORMAT_SUBS, sizeof(mcxTing), mcxTingRelease)

   ;  mcxTingFree(&args)
   ;  mcxTingFree(&fmt)

   ;  return yamSegPush(seg, out)
;  }



