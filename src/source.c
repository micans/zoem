/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *   (C) Copyright 2008, 2009, 2010, Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../config.h"
#include "filter.h"
#include "util.h"
#include "entry.h"
#include "key.h"
#include "source.h"
#include "read.h"
#include "digest.h"
#include "iface.h"

#include "tingea/hash.h"
#include "tingea/ting.h"
#include "tingea/types.h"
#include "tingea/io.h"
#include "tingea/minmax.h"

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#define HAVE_READLINE 1
#endif

const char* ia_sep = "<-------------- zoem -------------->";


typedef struct
{  long     plc      /* perceived line count */
;  int      delta    /* at perceived line count lc, add delta */
;
}  linecc   ;        /* line count compensator */


typedef struct
{  mcxTing*          fname
;  const mcxTing*    txt
;  long              linect      /* *current* line count while parsing */
;  mcxGrim*          src_lcc
;  mcxLink*          lst_lcc
;
}  inputHook         ;


inputHook inputHookDir[]
=   
{  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
,  {  NULL, NULL, 1, NULL, NULL  }
}  ;


#define MAX_FILES_NEST ((int) (sizeof inputHookDir / sizeof(inputHook)))

#define hd inputHookDir


static int hdidx_g =  -1;     /* Careful! should never be less than -1 */

void mod_source_init
(  void
)
   {  yamKeySet("__fnin__", "_nil_")
   ;  yamKeySet("__fnup__", "the_invisible_hand")
;  }


void mod_source_exit
(  void
)
   {  dim i
   ;  for (i=0;i<MAX_FILES_NEST;i++)
      {  mcxTing* t = hd[i].fname
      ;  if (t)
         mcxTingFree(&t)
   ;  }
;  }


mcxstatus sourcePop
(  void
)
   {  if (hdidx_g >= 0)
         mcxGrimFree(&(hd[hdidx_g].src_lcc))
      ,  mcxListFree(&(hd[hdidx_g].lst_lcc), NULL)

   ;  hdidx_g--

   ;  if (hdidx_g >= 0)
      yamKeySet("__fnin__", hd[hdidx_g].fname->str)

   ;  if (hdidx_g >= 1)
      yamKeySet("__fnup__", hd[hdidx_g-1].fname->str)

   ;  return STATUS_OK
;  }


mcxstatus sourcePush
(  const char*       str
,  const mcxTing*    txt
)
   {  linecc* lcc
   ;  hdidx_g++         /* now at least >= 0 */

   ;  if (hdidx_g >= MAX_FILES_NEST && hdidx_g--)
      return STATUS_FAIL

   ;  hd[hdidx_g].linect   =  1
   ;  hd[hdidx_g].txt      =  txt

   ;  hd[hdidx_g].src_lcc  =  mcxGrimNew(sizeof(linecc), 10, MCX_GRIM_GEOMETRIC)

   ;  lcc         =  mcxGrimGet(hd[hdidx_g].src_lcc)  
   ;  lcc->plc    =  1
   ;  lcc->delta  =  0

   ;  hd[hdidx_g].lst_lcc  =  mcxListSource(10, MCX_GRIM_GEOMETRIC)

            /*
             * Fixme/document: listSource is given a value;
             */
   ;  hd[hdidx_g].lst_lcc->val = lcc
            /*
             * Why use mcxGrimGet above?
            */

   ;  if (hd[hdidx_g].fname)
      mcxTingWrite(hd[hdidx_g].fname, str)
   ;  else
      hd[hdidx_g].fname    =   mcxTingNew(str)

   ;  yamKeySet("__fnin__", str)

   ;  if (hdidx_g >= 1)
      yamKeySet("__fnup__", hd[hdidx_g-1].fname->str)

   ;  return STATUS_OK
;  }


mcxbool sourceCanPush
(  void
)  {  return (hdidx_g+1) < MAX_FILES_NEST ? TRUE : FALSE
;  }


long sourceGetLc
(  void
)
   {  inputHook* hk = hdidx_g >= 0 ? hd+hdidx_g : hd+0
   ;  mcxLink* lk =  hk->lst_lcc
   ;  linecc* lcc =  lk->val
   ;  while (lk->prev && lcc->plc > hk->linect)
      {  lk = lk->prev
      ;  lcc = lk->val
   ;  }
      return hk->linect + lcc->delta
;  }


const char* sourceGetName
(  void
)
   {  return hdidx_g >= 0 ? hd[hdidx_g].fname->str : ""
;  }


mcxTing* sourceGetPath
(  void
)
   {  char* s
   ;  char* a = hdidx_g < 0 ? NULL : hd[hdidx_g].fname->str
   ;  if (!a)
      return NULL
   ;  if (!(s = strrchr(a, '/')))
      return NULL
   ;  return mcxTingNNew(a, s-a+1)
;  }


void sourceIncrLc
(  const mcxTing* txt
,  int d
)
   {  if (!txt || hd[hdidx_g].txt == txt)
      hd[hdidx_g].linect += d
   /* case !txt is used for inline file ___jump_lc___ hack */
;  }


void enter_interactive
(  void
)
   {  mcxTing* txt = mcxTingNew("\\zinsert{stdia}")
   ;  yamDigest(txt, txt, NULL)
   ;  mcxTingFree(&txt)
;  }


static void lcc_update
(  long n_deleted
,  long n_lines
,  void* data
)
   {  inputHook* hk  =  data
   ;  linecc*  lcc   =  mcxGrimGet(hk->src_lcc)
   ;  lcc->delta     =  (((linecc*) hk->lst_lcc->val)->delta) + n_deleted
   ;  lcc->plc       =  n_lines - lcc->delta
   ;  hk->lst_lcc    =  mcxLinkAfter(hk->lst_lcc, lcc)
;  }


mcxstatus sourceAscend
(  mcxTing    *fnsearch
,  mcxbits     modes
,  size_t      chunk_size
)
   {  int fltidx =   modes & SOURCE_DEFAULT_SINK
                  ?  ZOEM_FILTER_DEFAULT
                  :  ZOEM_FILTER_NONE

   ;  size_t   sz_file  =  0
   ;  size_t   sz_chunk =  0
   ;  mcxTing* filetxt  =  mcxTingEmpty(NULL, 1000)

   ;  mcxbool  use_searchpath = !(modes & SOURCE_NO_SEARCH)
   ;  mcxbool  allow_inline   = !(modes & SOURCE_NO_INLINE)
   ;  mcxbool  debug          =  (modes & SOURCE_DEBUG)

   ;  const mcxTing* inline_txt        =  NULL
   ;  const mcxTing** inline_txt_ptr   =  allow_inline ? &inline_txt : NULL
   ;  mcxIO *xf            =  yamTryOpen(fnsearch, inline_txt_ptr, use_searchpath)
   ;  mcxstatus stat_open  =  STATUS_OK
   ;  mcxstatus stat_read  =  STATUS_OK
   ;  mcxbits flag_zoem   =  0

   ;  if (inline_txt)
         mcxTingWrite(filetxt, inline_txt->str)
      ,  stat_read = STATUS_DONE
   ;  else if (xf)
      {  struct stat mystat
      ;  if (!xf->stdio)
         {  if (stat(xf->fn->str, &mystat))
            mcxErr("sourceAscend", "can not stat file <%s>", xf->fn->str)
         ;  else
            {  if (!chunk_size)
               sz_chunk = mystat.st_size
            ;  else if (chunk_size)
               sz_chunk = MCX_MIN(mystat.st_size, 1.1 * chunk_size)
            ;  sz_file = mystat.st_size
         ;  }
            if (sz_chunk)
            mcxTingEmpty(filetxt, sz_chunk)
      ;  }
      }
      else
      stat_open = STATUS_FAIL

   ;  if (stat_open == STATUS_OK)
      {  long n_lines_read = 0
      ;  long n_chunk = 0 
      ;  sourcePush(xf->fn->str, filetxt)

      ;  while(1)
         {  if (!inline_txt)
            {  stat_read
               =  yamReadChunk
                  (  xf
                  ,  filetxt
                  ,  0
                  ,  chunk_size
                  ,  &n_lines_read
                  ,  lcc_update
                  ,  hd+hdidx_g
                  )
            ;  if (stat_read != STATUS_OK && stat_read != STATUS_DONE)
               break
         ;  }

            if ((flag_zoem = yamOutput(filetxt, NULL, fltidx)))
            {  if (flag_zoem & SEGMENT_DONE)
               flag_zoem = 0
            ;  else
               {  mcxErr("zoem", "unwound on error/exception")
               ;  if (debug)         /* fixme; could also enter debug in caller? */
                  enter_interactive()
            ;  }
               break
         ;  }

            if (inline_txt)
            break

         ;  if
            (  xf
            && (  stat_read == STATUS_OK
               || (n_chunk && stat_read == STATUS_DONE)
               )
            )
            mcxTell("zoem", "read chunk of size <%ld>", (long) filetxt->len)
            
         ;  if (stat_read == STATUS_DONE)
            break

         ;  if (hdidx_g == 3)
            fprintf(stdout, "[%s]\n", filetxt->str)
         ;  n_chunk++
      ;  }

         sourcePop()
   ;  }

      mcxTingFree(&filetxt)
   ;  mcxIOfree(&xf)

   ;  if (stat_open)
      return STATUS_FAIL_OPEN
   ;  if (stat_read != STATUS_DONE)
      return STATUS_FAIL_READ
   ;  if (flag_zoem)
      return STATUS_FAIL_ZOEM

   ;  return STATUS_OK
;  }


void ia_separate
(  mcxTing* tmp
)
   {  mcxTing* ump = yamKeyGet(tmp)
   ;  fprintf(stdout, "%s\n", ump ? ump->str : ia_sep)
   ;  fflush(stdout)
;  }


void sourceStdia
(  mcxTing* filetxt
)
   {  mcxIO* xfin          =  mcxIOnew("-", "r")
   ;  mcxTing*  tmp        =  mcxTingNew("__parmode__")
   ;  mcxTing*  ump        =  yamKeyGet(tmp)
   ;  int       parmode    =  ump ? atoi(ump->str) : MCX_READLINE_DOT
   ;  mcxbool   use_readline = FALSE
   ;  const char* me       =  "stdia"
   ;  mcxbits flag_zoem = 0
   ;  sink *my_sink
   ;  mcxIO* xfout = NULL

#if HAVE_READLINE
   ;  use_readline = readlineOK
#endif

   ;  mcxTingWrite(tmp, "__ia__")

   ;  if
      (  mcxIOopen(xfin, RETURN_ON_FAIL)
      || !(xfout = yamOutputNew("-"))
      )
      yamErrx(1, "init PBD", "failure opening interactive session")

   ;  my_sink = xfout->usr
   ;  /*  sinkPush(xfout->usr, ZOEM_FILTER_DEVICE)
       *  No longer necessary with the new framework.
       *  Temporarily left as a reminder of old-times
      */

#define VERBOSE_IA 1
   ;  if (VERBOSE_IA)
      {  fprintf
         (  stdout
         ,  "%s%s%s"
         ,  "=== Interactive session, I should recover from errors.\n"
            "=== If I exit unexpectedly, consider sending a bug report.\n"
         ,     parmode & MCX_READLINE_DOT
           ?  "=== A single dot on a line of its own triggers interpretation.\n"
           :  ""
         ,    "=== Separator can be changed through key __ia__ .\n"
         )
      ;  if (use_readline)
         fprintf(stdout, "=== Readline enabled.\n")
      ;  fflush(stdout)
   ;  }

#if HAVE_READLINE
      if (use_readline)
      {  while (1)
         {  char* line = readline("")
         ;  if (!line)
            break
         ;  if (strcmp(".", line))
            {  mcxTingPrintAfter(filetxt, "%s\n", line)
            ;  add_history(line)
            ;  continue
         ;  }
         ;  if (VERBOSE_IA)
            ia_separate(tmp)
         ;  sourcePush(me, filetxt)
         ;  if ((flag_zoem = yamOutput(filetxt, my_sink, ZOEM_FILTER_DEFAULT)))
            {  const char* type
               =     flag_zoem & SEGMENT_THROW
                  ?  "exception"
                  :     flag_zoem & SEGMENT_DONE
                     ?  "EOP"
                     :  "error"
            ;  fprintf(stdout, "(interactive %s)\n", type)
         ;  }
            fflush(stdout)
         ;  sourcePop()

         ;  filterLinefeed(my_sink->fd, stdout)

         ;  if (xfin->ateof)
            break
         ;  if (VERBOSE_IA)
            ia_separate(tmp)
         ;  mcxTingEmpty(filetxt, 0)
      ;  }
      }
      else
#endif
      while (STATUS_OK == mcxIOreadLine(xfin, filetxt, parmode))
      {  if (VERBOSE_IA)
         ia_separate(tmp)
      ;  sourcePush(me, filetxt)
      ;  if ((flag_zoem = yamOutput(filetxt, my_sink, ZOEM_FILTER_DEFAULT)))
         {  const char* type
            =     flag_zoem & SEGMENT_THROW
               ?  "exception"
               :     flag_zoem & SEGMENT_DONE
                  ?  "EOP"
                  :  "error"
         ;  fprintf(stdout, "(interactive %s)\n", type)
      ;  }
         fflush(stdout)
      ;  sourcePop()

      ;  filterLinefeed(my_sink->fd, stdout)

      ;  if (xfin->ateof)
         break
      ;  if (VERBOSE_IA)
         ia_separate(tmp)
   ;  }

      /* sinkPop(xfout->usr)
      */
   ;  mcxTingFree(&tmp)
   ;  mcxIOfree(&xfin)
;  }


