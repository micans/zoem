/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>


#include "ops.h"
#include "ops-xtag.h"
#include "ops-constant.h"
#include "ops-grape.h"
#include "ops-env.h"
#include "key.h"
#include "source.h"
#include "read.h"
#include "iface.h"
#include "filter.h"
#include "key.h"
#include "digest.h"
#include "parse.h"
#include "util.h"
#include "version.h"
#include "../config.h"

#include "tingea/ting.h"
#include "tingea/io.h"
#include "tingea/types.h"
#include "tingea/ding.h"
#include "tingea/err.h"
#include "tingea/tr.h"

#define STRING_h(x) #x
#define STRING(x) STRING_h(x)

dim buckets_user = 1200;   /* 600 */
dim buckets_zoem = 1200;   /* 200 */



mcxTing* make_searchpath
(  const char* zip
)
   {  mcxTing* sp   =  mcxTingNew(zip)
   ;  mcxTing* zsp  =   NULL
   ;  char *q = NULL, *p = sp->str
   ;  mcxTingTr(sp, ":[:space:]", " [* *]", NULL, "[:space:]", 0)
   ;  while (p && p<sp->str+sp->len)
      {  while (isspace((unsigned char) *p))
         p++
      ;  q = mcxStrChrIs(p, isspace, -1)
      ;  if (q)
         *q = '\0'
      ;  zsp = mcxTingPrintAfter(zsp, "{%s}", p)
      ;  if (q)
         *q = ' '
      ;  p = q
   ;  }
      mcxTingFree(&sp)
   ;  return zsp
;  }


/*
 *    The file IO init stuff is still slightly kludgy and ad hoc
 *    (it used to be worse) - it is somewhat split over zoem.c and entry.c
 *    The reason is that it's a bit of if-else spaghetti for deciding
 *    what to do in the presence of various combinations of (mainly)
 *    -i, -o, -e, and -E, and whether stdin/stdout arguments are present. 
 *    Remember that when none of -i, -I, -o is specified, we do
 *    interactive mode.
 *    The rest of zoem is a lot cleaner, even if zoem is a dirty language :)
*/

mcxstatus yamEntry
(  const char* sfnentry
,  const char* sfnpath
,  const char* sfnbase
,  size_t      chunk_size
,  const char* sfnout
,  const char* sdevice
,  int         fltidx
,  mcxbits     trace_flags
,  mcxbits     entry_flags
,  mcxTing*    vars
,  mcxTing*    expr
)
   {  mcxTing*  fnentry    =  mcxTingNew(sfnentry)
   ;  mcxTing*  fnout      =  mcxTingNew(sfnout)
   ;  mcxTing*  fnpath     =  mcxTingNew(sfnpath)
   ;  mcxTing*  fnbase     =  mcxTingNew(sfnbase)
   ;  const char* zip      =  STRING(INCLUDEPATH)
   ;  mcxbool   debug      =  entry_flags & ENTRY_DEBUG
   ;  mcxbool   stdia      =  entry_flags & ENTRY_STDIA
   ;  mcxTing*  zsp

   ;  mcxIO *xfout = NULL
   ;  mcxTing *filetxt     =  NULL /* involved in interactive hackery */
   ;  const char* me       =  "zoem main"

   ;  mcxbits flag_zoem    =  0
   ;  mcxstatus status     =  STATUS_OK

   ;  if (!stdia)
      {  if (entry_flags & ENTRY_STDIN)
         fprintf(stderr, "=== reading from stdin\n")
      ;  if (entry_flags & ENTRY_STDOUT)
         fprintf(stderr, "=== writing to stdout\n")
      ;  filetxt = mcxTingEmpty(NULL, 1000)
   ;  }
     /* check enter_interactive should print something I guess */

      mod_key_init         ( buckets_user )
   ;  mod_read_init        (  5  )
   ;  mod_env_init         (  20 )
   ;  mod_grape_init       (     )
   ;  mod_ops_init         ( buckets_zoem )
   ;  mod_xtag_init        (     )
   ;  mod_source_init      (     )
   ;  mod_sink_init        (  10 )
   ;  mod_constant_init    (  50 )
   ;  mod_filter_init      (  10 )
   ;  mod_segment_init     (     )

   ;  mod_parse_init       (trace_flags)
   ;  mod_ops_digest       (     )           /* create \begin{} env */

   ;  yamKeyDef("__device__", *sdevice ? sdevice : "__none__")
   ;  yamKeyDef("__fnbase__", fnbase->str)
   ;  yamKeyDef("__fnpath__", fnpath->str)
   ;  yamKeyDef("__fnentry__", fnentry->str)
   ;  yamKeyDef("__version__", zoemDateTag)
   ;  yamKeyDef("__sysval__", "0")
   ;  yamKeyDef("__lc__", "{")
   ;  yamKeyDef("__split__",  (entry_flags & ENTRY_SPLIT) ? "1" : "0")
   ;  yamKeyDef("__rc__", "}")
   ;  yamKeyDef("__parmode__", "16")

   ;  zsp = make_searchpath(zip)
   ;  yamKeyDef("__searchpath__", zsp->str)

                                    /* fixme, enable setx  */
   ;  if (vars && vars->len)        /* fixme, hide details */
      {  char *p  = vars->str
      ;  char *r, *q
      ;  mcxbits  keybits = 0
      ;  int namelen    =  0

      ;  while ((q = strchr(p, '\036')))
         {  *q = '\0'
         ;  if ((r = strchr(p, '=')))
            *r = '\0'  
         ;  if
            (  strlen(p)
            != checkusrsig(p, strlen(p), NULL, &namelen, &keybits)
            )                                      /* mixed sign comparison */
            yamErrx(1, "init", "key <%s> not acceptable", p)

         ;  yamKeySet(p, r ? r+1 : "1")
         ;  p  = q+1
      ;  }
      }

     /* this works for -e and -E (expression) options,
      * because those arguments are embedded in \write#3,
      * which issues yamOutputNew also - so \__fnout__ is always set.
     */
      if (!(xfout = yamOutputNew(fnout->str)))
      yamErrx(1, "init", "no output stream")

   ;  sinkPush(xfout->usr, fltidx)  /* this sets up our default sink */

   ;  if (expr->len)
      {  flag_zoem = yamOutput(expr, xfout->usr, ZOEM_FILTER_DEVICE)
      ;  if (flag_zoem & (SEGMENT_THROW | SEGMENT_ERROR))
         {  mcxErr(me, "Expression: unwound on error")
         ;  if (debug)
            enter_interactive()
      ;  }
      }

      if (entry_flags & ENTRY_EXXIT)
                 /*
                  *  if e.g. zoem -i - -E foo,
                  *  foo gets printed only after <control D> is pressed.
                  *  the reason is we are hanging out here below.
                 */
   ;  else if (!flag_zoem && !stdia)
      {  mcxTing *fn = mcxTingNew(fnentry->str)
      ;  mcxbits  mode = SOURCE_DEFAULT_SINK | SOURCE_NO_SEARCH

      ;  if (debug)
         mode |= SOURCE_DEBUG

      ;  status = sourceAscend (fn, mode, chunk_size)    /* the big one */

      ;  if (status == STATUS_FAIL_OPEN)
         yamErr("zoem", "cannot open file <%s>", fnentry->str)
      ;  mcxTingFree(&fn)
   ;  }
      else if (stdia)
      enter_interactive()

   ;  if (trace_flags & ZOEM_TRACE_HASH)
      yamStats()

   ;  mod_registrees()
   ;  sinkPop(xfout->usr)

   ;  mod_env_exit()
   ;  mod_ops_exit()
   ;  mod_xtag_exit()
   ;  mod_key_exit()
   ;  mod_filter_exit()
   ;  mod_read_exit()
   ;  mod_source_exit()
   ;  mod_sink_exit()
   ;  mod_constant_exit()
   ;  mod_parse_exit()
   ;  mod_grape_exit()
   ;  mod_iface_exit()
   ;  mod_segment_exit()

   ;  mcxTingFree(&filetxt)
   ;  mcxTingFree(&fnbase)
   ;  mcxTingFree(&fnpath)
   ;  mcxTingFree(&fnentry)
   ;  mcxTingFree(&fnout)
   ;  mcxTingFree(&zsp)
   ;  return status
;  }

