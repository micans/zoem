/*   (C) Copyright 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include "ops-xtag.h"

#include <ctype.h>

#include "util.h"
#include "segment.h"
#include "digest.h"

#include "tingea/types.h"
#include "tingea/ding.h"
#include "tingea/list.h"


yamSeg* yamXtag
(  yamSeg*  seg
,  mcxTing* dev      /* if dev->len == 0, implicit closing tag */
,  mcxTing* arg
)
   {  mcxTing* txt = mcxTingEmpty(NULL, 80)
   ;  char* me =  arg ? "\\<>#2" : "\\<>#1"
   ;  mcxbool ok = TRUE
   ;  mcxbits flags = 0
   ;  lblStack* xmlStack = sinkGetXML()
   ;  sink* sk = sinkGet()    /* docme: is this (default) sink the one we want? */

   ;  if (yamDigest(dev,dev, seg))
      ok = FALSE

   ;  if (ok && arg && yamDigest(arg, arg, seg))
      ok = FALSE

   ;  if (!ok)
      NOTHING
            /*  we interpret, but keep syntax as is, so that the thing
             *  will pass again through this routine in OUTPUT mode.
            
             *  Why? because otherwise everything is stuffed in \@{<..>} scope
             *  (and invisible to (repeated) interpretation).
             *  Perhaps it'd be better to stuff it in \@{<} .. \@{>} scope.
            */
   ;  else if (seg->flags & SEGMENT_DIGEST)
      {  if (arg)
         mcxTingPrint(txt, "\\<%s>{%s}", dev->str, arg->str)
      ;  else
         mcxTingPrint(txt, "\\<%s>", dev->str)
      ;  flags |= SEGMENT_CONSTANT
   ;  }
      else
      while(1)
      {  ok = FALSE
                      /*  if branch: close tag, either explicit or implicit
                      */
      ;  if (*(dev->str) == '/' || !dev->len)
         {  const char* otag = NULL

         ;  if (arg)
            {  yamErr(me, "No arguments allowed with closing tag")
            ;  break
         ;  }

            if (!(otag = ting_stack_pop(xmlStack)))
            {  yamErr
               (  me
               ,  "XML syntactic sugar stack underflow at file <%s> tag </%s>"
               ,  sk->fname
               ,  dev->len ? dev->str+1 : ""
               )
            ;  break
         ;  }

            if (dev->len && strcmp(dev->str+1, otag))
            {  yamErr(me, "tag </%s> closes <%s>", dev->str+1, otag)
            ;  break
         ;  }
            mcxTingPrint(txt, "\\@{\\X\\N\\J</%s>\\N}", otag)
      ;  }

           /*  open tag.
            *  recognize <!....> and leave it alone.
            *  recognize <$....> and then ignore dollar.
           */
         else
         {  char* p           =  mcxStrChrIs(dev->str, isspace, -1)
         ;  unsigned char z   =  *(dev->str+dev->len-1)
         ;  mcxbool closing   =  dev->str[0] == '!' || dev->str[0] == '*' || dev->str[0] == '?' || z == '/'
         ;  unsigned delta    =  dev->str[0] == '*'
         ;  mcxbool opening   =  !arg && !closing
         ;  int taglen        =  (p ? (unsigned) (p - dev->str) : dev->len) - delta /* ends at white space */
         ;  int devlen        =  dev->len - delta  /* used in printf length specification, hence int */

         ;  if (z == '!')     /* this makes \<br!> work */
               devlen -= 1
            ,  yamErr(me, "<...!> syntax now deprecated")

         ;  if (closing && arg)               
            {  yamErr(me, "no args allowed with closing syntax <%s>", dev->str)
            ;  break
         ;  }

            if (opening)
            ting_stack_push(xmlStack, dev->str+delta, taglen)
               /* fixme check (alloc) error */

         ;  txt =
            arg
            ?  mcxTingPrint
               (  txt
               ,  "\\@{<%s>}%s\\@{</%.*s>}"
               ,  dev->str+delta
               ,  arg->str
               ,  taglen
               ,  dev->str+delta
               )
            :  opening
               ?  mcxTingPrint
                  (  txt
                  ,  "\\@{\\X\\N<%.*s>\\N\\I}"
                  ,  devlen
                  ,  dev->str+delta
                  )
               :  mcxTingPrint
                  (  txt
                  ,  "\\@{<%.*s>}"
                  ,  devlen
                  ,  dev->str+delta
                  )
            ;
         }
         ok = TRUE
      ;  break
   ;  }

      seg_check_ok(ok, seg)

   ;  mcxTingFree(&dev)
   ;  mcxTingFree(&arg)

   ;  return yamSegPushx(seg, txt, flags)
;  }


void mod_xtag_exit
(  void
)
   {
;  }

void mod_xtag_init
(  void
)
   {
;  }



