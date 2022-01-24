/*   (C) Copyright 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of zoem. You can redistribute and/or modify zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with zoem, in the file COPYING.
*/


/* TODO
 *    should with lines return value follow input convention
 *    about presence/absence of last newline?
*/

#include "op-inspect.h"

#include <ctype.h>
#include <stdlib.h>
#include <regex.h>

#include "util.h"
#include "digest.h"
#include "key.h"
#include "curly.h"
#include "segment.h"
#include "read.h"
#include "parse.h"
#include "iface.h"

#include "tingea/ting.h"
#include "tingea/ding.h"
#include "tingea/err.h"
#include "tingea/list.h"


#define MATCH_MAX             10

#define REX_KNMP                 1
#define REX_INTERPOLATE          2
#define REX_MATCH_ONCE           4

#define ISP_DISCARD_NIL_OUT     32
#define ISP_DISCARD_MISS        64
#define ISP_RETURN_BOOL        128

struct inspect_dir
{  regex_t     reg
;  regmatch_t  matches[MATCH_MAX]
;  mcxTing*    data
;  mcxTing*    result
;  dim         offset
;  int         n_args
;  int         n_hits

;  mcxbits     opts
;
}  ;


mcxstatus do_rex
(  struct inspect_dir*  dir
,  yamSeg* seg  
)
   {  regex_t* reg         =  &dir->reg
   ;  regmatch_t *matches  =  dir->matches
   ;  mcxTing* data        =  dir->data
   ;  dim offset           =  dir->offset
   ;  mcxbits  opts        =  dir->opts
   ;  mcxTing* result      =  dir->result

   ;  int match_so, match_eo, match_len, n_hits = 0
   ;  mcxbool match
   ;  int status           =  REG_NOMATCH

   ;  mcxbool interpolate  =  opts & REX_INTERPOLATE
   ;  mcxbool knmp         =  opts & REX_KNMP

   ;  int i

   ;  const char* me = "\\inspect#4"
   ;  char* A, *Z, *p

   ;  dir->n_hits          =  0

   ;  A  =  data->str
   ;  Z  =  data->str + data->len
   ;  p  =  A

   ;  while (offset < data->len)
      {  status = regexec(reg, data->str+offset, MATCH_MAX, dir->matches, 0)
      ;  p           =  data->str+offset

      ;  match       =  !status
      ;  match_so    =  match ? matches[0].rm_so : -1
      ;  match_eo    =  match ? matches[0].rm_eo : -1
      ;  match_len   =  match_eo - match_so

                              /* if no match, possibly output final nmp */
                              /* ignore if !n_hits */
      ;  if (!match && knmp)
         mcxTingAppend(result, p)

      ;  else if (match)
         {  n_hits++  
         ;  if (knmp)
            mcxTingNAppend(result, p, match_so)

         ;  if (interpolate)
            {  yamSeg* rowseg = NULL
            ;  for (i=1;i<=dir->n_args;i++)
               {  if (matches[i].rm_so < 0)
                     mcxWarn(me, "no atom <%d>", i)
                  ,  mcxTingEmpty(key_and_args_g+i+1, 0)
               ;  else
                  mcxTingNWrite
                  (  key_and_args_g+i+1
                  ,  p+matches[i].rm_so
                  ,  matches[i].rm_eo - matches[i].rm_so
                  )
            ;  }
               if ((rowseg = yamExpandKey(seg, KEY_ANON)))
                  mcxTingAppend(result,rowseg->txt->str)
               ,  yamSegFree(&rowseg)
            ;  else
               mcxErr(me, "unexpected setback")
         ;  }
            else
            mcxTingAppend(result, key_g->str)
      ;  }

         if (match)
         {  if (opts & REX_MATCH_ONCE)
            {  if (knmp)
               mcxTingAppend(result, data->str + offset + match_eo)
            ;  offset = data->len
         ;  }
            else if (match_len)
            offset +=   match_eo
         ;  else
            {  offset += match_eo + 1
            ;  if ((*p+match_so) && knmp)
               mcxTingNAppend(result, p+match_so, 1)
         ;  }
         }
         else
         offset = data->len
   ;  }

      dir->n_hits = n_hits
   ;  return STATUS_OK
;  }


int chunk_it_up
(  mcxLink* lk
,  mcxTing* data
,  mcxbits piecewise
)
   {  int ct = 0, x = 0
   ;  if (!piecewise)
      {  mcxTing* tg = mcxTingNew(data->str)
      ;  mcxLinkAfter(lk, tg)
      ;  return 1
   ;  }
      else if (piecewise & 1)     /* arg-wise */
      {  yamSeg* tmpseg = yamStackPushTmp(data)

      ;  while (1 == (x = yamParseScopes(tmpseg, 1, 0)))
         {  mcxTing* tg = mcxTingNew(key_and_args_g[1].str)
         ;  lk = mcxLinkAfter(lk, tg)
         ;  ct++
      ;  }
         yamStackFreeTmp(&tmpseg)
   ;  }
      else if (piecewise & 2)
      {  const char* s = data->str, *t = NULL
      ;  while ((t = strchr(s, '\n')))
         {  mcxTing* tg = mcxTingNNew(s, t-s)
         ;  lk = mcxLinkAfter(lk, tg)
         ;  ct++
         ;  s = t+1
      ;  }
         if (*s)           /* last line not newline separated */
         {  mcxTing* tg = mcxTingNew(s)
         ;  lk = mcxLinkAfter(lk, tg)
         ;  ct++
      ;  }
      }
      return ct
;  }



/* POSIX regex metachars are ^.[$()|*+?{\
*/

yamSeg*  yamInspect4
(  yamSeg* seg
)
   {  mcxTing* mods        =  mcxTingNew(arg1_g->str)
   ;  mcxTing* pat         =  mcxTingNew(arg2_g->str)
   ;  mcxTing* sub         =  mcxTingNew(arg3_g->str)
   ;  mcxTing* data        =  mcxTingNew(arg4_g->str)
   ;  mcxTing* result      =  mcxTingEmpty(NULL, data->len)

   ;  mcxbits  piecewise   =  0           /* 1=vargwise 2=linewise  */
   ;  mcxbits  opts        =  REX_KNMP
   ;  mcxbool  ok          =  FALSE
   ;  mcxbool  esc_slash   =  FALSE       /* NOT USED */
   ;  yamSeg*  modseg      =  NULL

   ;  mcxLink* args        =  mcxListSource(20, MCX_GRIM_ARITHMETIC)
   ;  mcxLink* arg         =  NULL
   ;  const char* me       =  "\\inspect#4"
   ;  mcxbool reg_ok       =  FALSE
   ;  int n_args =  0, rc, k = 0, x = 0

   ;  struct inspect_dir dir

   ;  int regflags         =  REG_EXTENDED

/* fixme: single point of failure */

   ;  dir.result  =  mcxTingEmpty(NULL, 80)
   ;  dir.data    =  mcxTingEmpty(NULL, 80)

   ;  while (1)
      {  if
         (  yamDigest(mods, mods, seg)
         || yamDigest(data, data, seg)
         || yamDigest(mods, mods, seg)
         || yamDigest(pat, pat, seg)
         || yamDigest(sub,sub, seg)
         )
         break

      ;  modseg = yamStackPushTmp(mods)

      ;  while (2 == (x = yamParseScopes(modseg, 2, 0)))
         {  const char* val = arg2_g->str    /* danger arg2_g */
         ;  if (strcmp(arg1_g->str, "mods"))
            continue

         ;  if (strstr(val, "icase"))
            regflags |= REG_ICASE
         ;  if (!strstr(val, "dotall"))
            regflags |= REG_NEWLINE
         ;  if (strstr(val, "iter-lines"))
            piecewise = 2
         ;  if (strstr(val, "iter-args"))
            piecewise = 1
         ;  if (strstr(val, "discard-nmp"))
            opts ^= REX_KNMP
         ;  if (strstr(val, "discard-nil-out"))
            opts |= ISP_DISCARD_NIL_OUT
         ;  if (strstr(val, "discard-miss"))
            opts |= ISP_DISCARD_MISS
         ;  if (strstr(val, "count-matches"))
            opts |= ISP_RETURN_BOOL
         ;  if (strstr(val, "unprotect-slash"))
            esc_slash = TRUE
         ;  if (strstr(val, "match-once"))
            opts |= REX_MATCH_ONCE
      ;  }

         if (x < 0)
         break

      ;  if ((n_args = chunk_it_up(args, data, piecewise) < 0))
         break

      ;  ting_tilde_expand(pat, YAM_TILDE_UNIX | YAM_TILDE_REGEX)

      ;  if (tracing_g & ZOEM_TRACE_REGEX)
         mcxTellf(stdout, me, "using regex <%s>", pat->str)

#  if 0                              /* debug section */
      ;  {  mcxLink* lk = args->next
         ;  while (lk)
            {  mcxTing* tg = lk->val
            ;  lk = lk->next
         ;  }
         }
#  endif

      ;  if (regcomp(&dir.reg, pat->str, regflags))
         {  yamErr(me, "regex <%s> did not compile", pat->str)
         ;  break
      ;  }
         else
         reg_ok = TRUE

      ;  if
         (  !strncmp(sub->str, "_#", 2)
         && isdigit((unsigned char) sub->str[2])
         )
         {  opts |= REX_INTERPOLATE
         ;  k  =  (unsigned char) *(sub->str+2) - '0'
         ;  rc =  yamClosingCurly(sub, 3, NULL, RETURN_ON_FAIL)

         ;  if (rc < 0)
            {  yamErr(me, "unexpected error in anonymous sub part")
            ;  break
         ;  }
            else if (k > 9 || rc+4 != sub->len)
            {  yamErr(me, "anonymous sub <%s> not ok", sub->str)
            ;  break
         ;  }

            mcxTingNWrite(key_g, sub->str, 3)
         ;  n_args_g = k+1
         ;  mcxTingNWrite(arg1_g, sub->str+4, rc-1)
      ;  }
         else
         mcxTingWrite(key_g, sub->str)

      ;  mcxTingFree(&mods)
      ;  mcxTingFree(&pat)

     /* ******************************************************************* */
     /*     so far so good, let's rumble                                    */
     /* ******************************************************************* */

      ;  dir.opts    =  opts
      ;  dir.n_args  =  k

      ;  arg = args->next

      ;  while (arg)
         {  mcxstatus status
         ;  mcxTing* tg = arg->val

         ;  if (!tg)
            mcxDie(1, me, "ka-ching")

         ;  mcxTingWrite(dir.data, ((mcxTing*) arg->val)->str)
         ;  dir.offset  =  0
         ;  dir.n_hits  =  0

         ;  if ((status = do_rex(&dir, seg)))
            {  mcxErr(me, "regex failed")
            ;  break
         ;  }

            if (opts & ISP_RETURN_BOOL)
            mcxTingInteger(dir.result, dir.n_hits)

         ;  if (!dir.n_hits && opts & ISP_DISCARD_MISS)
            NOTHING
         ;  else if (!dir.result->len && opts & ISP_DISCARD_NIL_OUT)
            NOTHING
         ;  else if (piecewise & 1)
            mcxTingPrintAfter(result, "{%s}", dir.result->str)
         ;  else if (piecewise & 2)
            mcxTingPrintAfter(result, "%s\n", dir.result->str)
         ;  else
            mcxTingAppend(result, dir.result->str)
         ;  mcxTingEmpty(dir.result, 0)
         ;  arg = arg->next
      ;  }

         if (!arg)
         ok = TRUE
      ;  break
   ;  }

      seg_check_ok(ok, seg)

   ;  {  mcxLink* lk = args->next
      ;  mcxTingFree(&data)
      ;  mcxTingFree(&mods)
      ;  mcxTingFree(&pat)
      ;  mcxTingFree(&sub)
      ;  mcxTingFree(&dir.result)
      ;  mcxTingFree(&dir.data)
      ;  if (reg_ok)
         regfree(&dir.reg)
      ;  while (lk)
         {  mcxTingFree((mcxTing**) &(lk->val))    /* dirty */
         ;  lk = lk->next
      ;  }
         mcxListFree(&args, mcxTingFree_v)
   ;  }

      return yamSegPush(seg, result)
;  }


