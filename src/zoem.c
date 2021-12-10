/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entry.h"
#include "iface.h"
#include "util.h"
#include "ops.h"
#include "version.h"
#include "key.h"
#include "sink.h"
#include "filter.h"

#include "tingea/io.h"
#include "tingea/opt.h"
#include "tingea/ting.h"
#include "tingea/types.h"
#include "tingea/alloc.h"
#include "tingea/err.h"
#include "tingea/minmax.h"

const char *zoemVersion
=
"zoem %s\n"
"(c) Copyright 2001 Stijn van Dongen.\n"
"(c) Copyright 2021 Stijn van Dongen.\n"
"zoem comes with NO WARRANTY to the extent permitted by law.\n"
"You may redistribute copies of zoem under the terms\n"
"of the GNU General Public License.\n"
;

const char* syntax = "Usage: zoem [-i fname[.azm]] [-d device] [options]";

enum
{  MY_OPT_OUTPUT
,  MY_OPT_INPUT
,  MY_OPT_input
                     ,  MY_OPT_DEVICE
,  MY_OPT_SET        =  MY_OPT_DEVICE + 2
,  MY_OPT_EXPR
,  MY_OPT_EXPR_EXIT
,  MY_OPT_SPLIT
,  MY_OPT_CHUNK
                     ,  MY_OPT_X
,  MY_OPT_HELP       =  MY_OPT_X + 2
,  MY_OPT_APROPOS
                     ,  MY_OPT_LIST
,  MY_OPT_ALLOW      =  MY_OPT_LIST + 2
,  MY_OPT_UNSAFE
,  MY_OPT_UNSAFE_SILENT
                        ,  MY_OPT_SYSTEM_HONOR
,  MY_OPT_STRESS_WRITE  =  MY_OPT_SYSTEM_HONOR + 2
,  MY_OPT_ERR_OUT
,  MY_OPT_TRACE
,  MY_OPT_TTRACE
,  MY_OPT_VERSION
,  MY_OPT_NORL
,  MY_OPT_TL
,  MY_OPT_TRACE_KEYS
,  MY_OPT_TRACE_REGEX
,  MY_OPT_TRACE_ALL_SHORT
,  MY_OPT_TRACE_ALL_LONG
                     ,  MY_OPT_STATS
,  MY_OPT_NSEGMENT   =  MY_OPT_STATS + 2
,  MY_OPT_NSTACK
,  MY_OPT_NUSER
                     ,  MY_OPT_NENV
,  MY_OPT_AMOIXA     =  MY_OPT_NENV + 2
,  MY_OPT_AMCS
,  MY_OPT_AMIC
,  MY_OPT_HF
,  MY_OPT_BUSER
,  MY_OPT_BZOEM
}  ;


mcxOptAnchor options[] =
{  {  "-o"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_OUTPUT
   ,  "<fname>"
   ,  "specify output file name, explicitly" 
   }
,  {  "-i"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_input
   ,  "<fname[.azm]>"
   ,  "specify 'azm' suffixed input file"
   }
,  {  "-I"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_INPUT
   ,  "<fname>"
   ,  "specify input file"
   }
,  {  "-d"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_DEVICE
   ,  "<device>"
   ,  "set zoem key \\__device__ to <device>)"
   }
,  {  "-s"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_SET
   ,  "<key=val>"
   ,  "set zoem key <key> to <val> (repeated use allowed)"
   }
,  {  "-e"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_EXPR_EXIT
   ,  "<any>"
   ,  "evaluate any (stdout output) and exit"
   }
,  {  "-E"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_EXPR
   ,  "<any>"
   ,  "evaluate any (stdout output) and proceed"
   }
,  {  "-x"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_X
   ,  NULL
   ,  "if error occurs, enter interactive mode"
   }
,  {  "-l"
   ,  MCX_OPT_HASARG | MCX_OPT_INFO
   ,  MY_OPT_LIST
   ,  NULL
   ,  "-l {all|zoem|legend|trace|builtin|session|filter|parmode} (list and exit)"
   }
,  {  "-nsegment"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_NSEGMENT
   ,  "<int>"
   ,  "maximum segment depth"
   }
,  {  "-nstack"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_NSTACK
   ,  "<int>"
   ,  "maximum stack depth"
   }
,  {  "-bzoem"
   ,  MCX_OPT_HASARG | MCX_OPT_HIDDEN
   ,  MY_OPT_BZOEM
   ,  "<int>"
   ,  "initial nrof zoem dictionary buckets"
   }
,  {  "-buser"
   ,  MCX_OPT_HASARG | MCX_OPT_HIDDEN
   ,  MY_OPT_BUSER
   ,  "<int>"
   ,  "initial nrof user dictionary buckets"
   }
,  {  "-nuser"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_NUSER
   ,  "<int>"
   ,  "size of user dictionary stack"
   }
,  {  "-nenv"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_NENV
   ,  "<int>"
   ,  "size of environment dictionary stack"
   }
,  {  "--err-out"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_ERR_OUT
   ,  NULL
   ,  "redirect verbosity/error output to stdout"
   }
,  {  "--stress-write"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_STRESS_WRITE
   ,  NULL
   ,  "for stress-testing zoem via write#3 (testing only)"
   }
,  {  "-allow"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_ALLOW
   ,  "<prog-name>[:<prog-name>]*"
   ,  "program invocations to allow (by name only)"
   }
,  {  "--unsafe"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_UNSAFE
   ,  NULL
   ,  "allow system calls, prompt for confirmation"
   }
,  {  "--unsafe-silent"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_UNSAFE_SILENT
   ,  NULL
   ,  "allow system calls silently"
   }
,  {  "--system-honor"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_SYSTEM_HONOR
   ,  NULL
   ,  "require system calls to succeed"
   }
,  {  "--split"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_SPLIT
   ,  NULL
   ,  "prepare for split output"
   }
,  {  "--readline-off"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_NORL
   ,  NULL
   ,  "turn off readline in interactive mode"
   }
,  {  "-amcs"
   ,  MCX_OPT_HASARG | MCX_OPT_HIDDEN
   ,  MY_OPT_AMCS
   ,  "<int>"
   ,  "alloc maximum chunk size"
   }
,  {  "-amic"
   ,  MCX_OPT_HASARG | MCX_OPT_HIDDEN
   ,  MY_OPT_AMIC
   ,  "<int>"
   ,  "alloc maximum invocation count"
   }
,  {  "-hf"
   ,  MCX_OPT_HASARG | MCX_OPT_HIDDEN
   ,  MY_OPT_HF
   ,  "<label>"
   ,  "hash function to use (testing only)"
   }
,  {  "-chunk-size"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_CHUNK
   ,  "<int>"
   ,  "read chunk size (0 for entire files)"
   }
,  {  "-tl"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_TL
   ,  "<int>"
   ,  "tablength"
   }
,  {  "--trace"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_TTRACE
   ,  NULL
   ,  "default trace"
   }
,  {  "--trace-all-long"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_TRACE_ALL_LONG
   ,  NULL
   ,  "long/verbose trace"
   }
,  {  "--trace-all-short"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_TRACE_ALL_SHORT
   ,  NULL
   ,  "terse trace"
   }
,  {  "--trace-regex"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_TRACE_REGEX
   ,  NULL
   ,  "trace regexes"
   }
,  {  "--trace-keys"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_TRACE_KEYS
   ,  NULL
   ,  "trace keys"
   }
,  {  "-trace"
   ,  MCX_OPT_HASARG
   ,  MY_OPT_TRACE
   ,  "<k>"
   ,  "use mask k for tracing"
   }
,  {  "--stats"
   ,  MCX_OPT_DEFAULT
   ,  MY_OPT_STATS
   ,  NULL
   ,  "when done, print key table statistics"
   }
,  {  "--amoixa"
   ,  MCX_OPT_INFO | MCX_OPT_HIDDEN
   ,  MY_OPT_AMOIXA
   ,  NULL
   ,  "^_^"
   }
,  {  "--apropos"
   ,  MCX_OPT_INFO
   ,  MY_OPT_APROPOS
   ,  NULL
   ,  "list synopsis of all options"
   }
,  {  "-h"
   ,  MCX_OPT_INFO
   ,  MY_OPT_HELP
   ,  NULL
   ,  "list synopsis of all options"
   }
,  {  "--version"
   ,  MCX_OPT_INFO
   ,  MY_OPT_VERSION
   ,  NULL
   ,  "list version number"
   }
,  {  NULL, 0, 0, NULL, NULL }
}  ;


int sort_it_out
(  int      entry_flags
,  mcxTing* fnbase
,  mcxTing* fnpath
,  mcxTing* fnentry
,  mcxTing* fnout
,  mcxTing* device
)  ;


int main
(  int   argc
,  char* argv[]
)
   {  mcxTing     *fnbase     =  mcxTingEmpty(NULL, 20)
   ;  mcxTing     *fnpath     =  mcxTingEmpty(NULL, 20)
   ;  mcxTing     *fnout      =  mcxTingEmpty(NULL, 20)
   ;  mcxTing     *fnentry    =  mcxTingEmpty(NULL, 20)
   ;  mcxTing     *device     =  mcxTingEmpty(NULL, 20)
   ;  mcxTing     *xxpr       =  mcxTingEmpty(NULL, 20)
   ;  mcxTing     *listees    =  mcxTingEmpty(NULL, 1)
   ;  mcxTing     *vars       =  mcxTingEmpty(NULL, 30)
   ;  mcxbits     trace_flags =  0
   ;  mcxbits     entry_flags =  0
   ;  int stack_depth = -1, segment_depth = -1
      ,  n_user_scopes=-1, n_dollar_scopes=-1
   ;  const char* me = "zoem"
   ;  long amcs = 0
   ;  long amic = 0
   ;  mcxstatus parseStatus = STATUS_OK
   ;  mcxstatus status = STATUS_OK

   ;  mcxOption* opts, *opt

   ;  mcxOptAnchorSortById(options, sizeof(options)/sizeof(mcxOptAnchor) -1)
   ;  opts = mcxOptParse(options, (char**) argv, argc, 1, 0, &parseStatus)

   ;  if (!opts)
      exit(0)

   ;  for (opt=opts;opt->anch;opt++)
      {  mcxOptAnchor* anch = opt->anch

      ;  switch(anch->id)
         {  case MY_OPT_HELP
         :  case MY_OPT_APROPOS
         :  mcxOptApropos(stdout, me, syntax, 20, MCX_OPT_DISPLAY_SKIP, options)
         ;  return 0
         ;

            case MY_OPT_AMOIXA
         :  mcxOptApropos
            (  stdout
            ,  me
            ,  syntax
            ,  20
            ,  MCX_OPT_DISPLAY_HIDDEN | MCX_OPT_DISPLAY_SKIP
            ,  options
            )
         ;  return 0
         ;

            case MY_OPT_DEVICE
         :  mcxTingWrite(device, opt->val)
         ;  if (!strcmp(opt->val, "html"))
            mcxTingInsert
            (  xxpr
            ,  "\\special{{60}{&lt;}{62}{&gt;}{38}{&amp;}{-1}{&nbsp;}{-2}{<br>}{-3}{&mdash;}}"
            ,  0
            )
         ;  break
         ;

            case MY_OPT_BZOEM
         :  buckets_zoem = atoi(opt->val)
         ;  break
         ;

            case MY_OPT_BUSER
         :  buckets_user = atoi(opt->val)
         ;  break
         ;

            case MY_OPT_TL
         :  {  int t = atoi(opt->val)
            ;  t = MCX_MIN(4, t)
            ;  ztablength_g = MCX_MAX(0, t)
            ;  break
         ;  }

            case MY_OPT_CHUNK
         :  chunk_size = strtol(opt->val, NULL, 10)  /* could check */
         ;  break
         ;

            case MY_OPT_NENV
         :  n_dollar_scopes = atoi(opt->val)
         ;  break
         ;

            case MY_OPT_NUSER
         :  n_user_scopes = atoi(opt->val)
         ;  break
         ;

            case MY_OPT_NSTACK
         :  stack_depth = atoi(opt->val)
         ;  break
         ;

            case MY_OPT_NSEGMENT
         :  segment_depth = atoi(opt->val)
         ;  break
         ;

            case MY_OPT_HF
         :  if (yamHashFie(opt->val))
               mcxErr(me, "hash function <%s> not supported", opt->val)
            ,  mcxExit(1)
         ;  break
         ;

            case MY_OPT_AMIC
         :  amic = atol(opt->val)
         ;  break
         ;

            case MY_OPT_AMCS
         :  amcs = atol(opt->val)
         ;  break
         ;

            case MY_OPT_NORL
         :  readlineOK = 0
         ;  break
         ;

            case MY_OPT_SPLIT
         :  mcxTingWrite(fnout, "-")
         ;  entry_flags |= ENTRY_SPLIT
         ;  break
         ;

            case MY_OPT_EXPR
         :  mcxTingPrintAfter(xxpr, "%s\n\\@{\\N}", opt->val)
         ;  break
         ;

            case MY_OPT_EXPR_EXIT
         :  mcxTingPrintAfter(xxpr, "%s\n\\@{\\N}", opt->val)
         ;  entry_flags  |= ENTRY_EXXIT
         ;  break
         ;

            case MY_OPT_SET
         :  mcxTingAppend(vars, opt->val)
         ;  mcxTingAppend(vars, "\036")
         ;  break
         ;

            case MY_OPT_TTRACE
         :  trace_flags |= ZOEM_TRACE_DEFAULT
         ;  break
         ;

            case MY_OPT_X
         :  entry_flags |= ENTRY_DEBUG
         ;  break
         ;

            case MY_OPT_UNSAFE
         :  systemAccess = SYSTEM_UNSAFE
         ;  break
         ;

            case MY_OPT_UNSAFE_SILENT
         :  systemAccess = SYSTEM_UNSAFE_SILENT
         ;  break
         ;

            case MY_OPT_ALLOW
         :  system_allow
            =  system_allow
            ?  mcxTingPrintAfter(system_allow, "%s:", opt->val)
            :  mcxTingPrint(system_allow, ":%s:", opt->val)
         ;  break
         ;

            case MY_OPT_SYSTEM_HONOR
         :  systemHonor = 1
         ;  break
         ;

            case MY_OPT_STRESS_WRITE
         :  stressWrite = 1
         ;  break
         ;

            case MY_OPT_ERR_OUT
         :     yamErrorFile(stdout)
            ,  mcxErrorFile(stdout)
         ;  break
         ;

            case MY_OPT_TRACE_ALL_LONG
         :  trace_flags |= ZOEM_TRACE_ALL_LONG
         ;  break
         ;

            case MY_OPT_TRACE_ALL_SHORT
         :  trace_flags |= ZOEM_TRACE_ALL
         ;  break
         ;

            case MY_OPT_TRACE_KEYS
         :  trace_flags |= ZOEM_TRACE_KEYS
         ;  break
         ;

            case MY_OPT_TRACE_REGEX
         :  trace_flags |= ZOEM_TRACE_REGEX
         ;  break
         ;

            case MY_OPT_STATS
         :  trace_flags |= ZOEM_TRACE_HASH
         ;  break
         ;

            case MY_OPT_VERSION
         :  fprintf(stdout, zoemVersion, zoemDateTag)
         ;  exit(0)
         ;  break
         ;

            case MY_OPT_TRACE
         :  {  int k = atoi(opt->val)
            ;  if (k == -1)
               trace_flags = ZOEM_TRACE_ALL
            ;  else if (k == -2)
               trace_flags = ZOEM_TRACE_ALL_LONG
            ;  else
               trace_flags |= k
         ;  }
            break
         ;

            case MY_OPT_INPUT
         :  mcxTingWrite(fnbase, opt->val)
         ;  mcxTingWrite(fnentry, opt->val)
         ;  break
         ;

            case MY_OPT_input
         :  mcxTingWrite(fnbase, opt->val)
         ;  break
         ;

            case MY_OPT_OUTPUT
         :  mcxTingWrite(fnout, opt->val)
         ;  break
         ;

            case MY_OPT_LIST
         :  mcxTingAppend(listees, opt->val)
         ;  mcxTingAppend(listees, ";")
         ;  break
      ;  }
      }

      mcxOptFree(&opts)

   ;  if (amcs || amic)
      mcxAllocLimits(amcs, amic)

   ;  if (listees->len > 0)
      {  yamOpList(listees->str)
      ;  filterList(listees->str)
      ;  yamKeyList(listees->str)
      ;  if (strstr(listees->str, "parmode"))
         {  mcxIOlistParmodes()
         ;  fprintf
            (  stdout
            ,  "Zoem's default mode is %d, modes can be or'ed\n"
            ,  MCX_READLINE_DOT
            )
      ;  }
         if (strstr(listees->str, "trace"))
         showTracebits()
      ;  mcxExit(0)
   ;  }

      entry_flags
      = sort_it_out(entry_flags, fnbase, fnpath, fnentry, fnout, device)

   ;  if (stack_depth > 0 || segment_depth > 0)
      yamSegUlimit(segment_depth, stack_depth)

   ;  if (n_user_scopes > 0)
      keyULimit(n_user_scopes)

   ;  if (n_dollar_scopes > 0)
      sinkULimit(n_dollar_scopes)

   ;  status = yamEntry
      (  fnentry->str
      ,  fnpath->str
      ,  fnbase->str
      ,  chunk_size     /* it's a global var, but that's for dofile#2 */
      ,  fnout->str
      ,  device->str
      ,  ZOEM_FILTER_DEVICE
      ,  trace_flags
      ,  entry_flags
      ,  vars
      ,  xxpr
      )

   ;  mcxTingFree(&vars)
   ;  mcxTingFree(&xxpr)
   ;  mcxTingFree(&fnbase)
   ;  mcxTingFree(&fnpath)
   ;  mcxTingFree(&fnout)
   ;  mcxTingFree(&fnentry)
   ;  mcxTingFree(&device)
   ;  mcxTingFree(&listees)

   ;  return status
;  }



int sort_it_out
(  int      entry_flags
,  mcxTing* fnbase
,  mcxTing* fnpath
,  mcxTing* fnentry
,  mcxTing* fnout
,  mcxTing* device
)
   {  if (!fnbase->len && !fnout->len)
      {  mcxTingWrite(fnbase, "-")
      ;  mcxTingWrite(fnentry, "-")
      ;  mcxTingWrite(fnout, "-")      /* should *not* be used */
      ;  entry_flags |= ENTRY_STDIA
   ;  }
     /* all three set in this case */

      if (!fnbase->len && !fnentry->len)
         mcxTingWrite(fnbase, "-")
      ,  mcxTingWrite(fnentry, "-")
   ;  else if (fnentry->len)
      mcxTingWrite(fnbase, fnentry->str)

        /*
         * now fnbase surely has a value;
         * fnentry perhaps not (fnbase->len && !fnentry->len)
        */
   ;  else if (!fnentry->len)
      {  if (!strcmp(fnbase->str, "-") || !strcmp(fnbase->str, "stdin"))
         mcxTingWrite(fnentry, fnbase->str)
      ;  else
         {  if (strstr(fnbase->str, ".azm") == fnbase->str + fnbase->len -4)
            mcxTingDelete(fnbase, -5, 4)
         ;  mcxTingPrint(fnentry, "%s.azm", fnbase->str)
      ;  }
      }

        /*
         * now construct fnpath and truncate fnbase
        */
      {  const char* s = strrchr(fnbase->str, '/')
      ;  if (!s)
         mcxTingWrite(fnpath, "")
      ;  else
         {  mcxTingNWrite(fnpath, fnbase->str, s-fnbase->str+1)
         ;  mcxTingSplice(fnbase, "", 0, fnpath->len, 0)
      ;  }
      }

      if
      (  (  !strcmp(fnentry->str, "-")
         || !strcmp(fnentry->str, "stdin")
         )
      && !(entry_flags & ENTRY_EXXIT)
      )
      entry_flags |= ENTRY_STDIN

   ;  if (!fnout->len)
      {  if (entry_flags & (ENTRY_STDIN | ENTRY_STDIA))
         mcxTingWrite(fnout, "-")
      ;  else
         mcxTingPrint
         (fnout,"%s.%s", fnbase->str, device->len ? device->str : "ozm")
   ;  }

      if
      (  (  !strcmp(fnout->str, "-")
         || !strcmp(fnout->str, "stdout")
         )
      && !(entry_flags & ENTRY_EXXIT)
      )
      entry_flags |= ENTRY_STDOUT

   ;  return entry_flags
;  }

