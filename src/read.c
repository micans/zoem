/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <stdio.h>
#include <stdlib.h>                 /* getenv */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <string.h>

#include "filter.h"
#include "read.h"
#include "source.h"
#include "parse.h"
#include "iface.h"
#include "segment.h"
#include "curly.h"
#include "util.h"
#include "key.h"

#include "tingea/io.h"
#include "tingea/hash.h"
#include "tingea/types.h"
#include "tingea/getpagesize.h"
#include "tingea/err.h"
#include "tingea/ding.h"
#include "tingea/tr.h"

static   mcxHash*    mskTable_g        =  NULL;    /* masked input files*/


/*
 * This code also finds and stores inline files.
 * It is not error/memory clean and contains hard exits.
 *
 * fixme does line count compensation work when chunks land inbetween
 * \:/ lines? or do chunks never do that?
*/

#define DEBUG 0

mcxstatus  yamReadChunk
(  mcxIO          *xf               /* must be open! */
,  mcxTing        *filetxt
,  mcxbits        flags
,  int            chunk_size        /* if 0, try to read everything */
,  long           *n_lines_read
,  void           (*lcc_update)(long n_deleted, long n_lines, void* data)
,  void*          data
)
   {  mcxbool  am_inline     =  flags & READ_INLINE
   ;  mcxTing* line        =  mcxTingEmpty(NULL, 160)
   ;  int      level       =  0
   ;  size_t   bytes_read  =  0      
   /*  size_t   bytes_offset=  xf->bc */
   ;  const char* me       =  "zoem input"
   ;  mcxstatus stat       =  STATUS_DONE
   ;  int      n_deleted   =  0

   ;  filetxt = mcxTingEmpty(filetxt, 0)

   ;  while
      (  STATUS_OK == (stat = mcxIOreadLine(xf, line, MCX_READLINE_DEFAULT)))
      {  int discardcmt = 0, esc = 0
      ;  char* a = line->str, *p = a, *z = a + line->len, *P = NULL

      ;  mcxbool inline_file_token =  strncmp(line->str, "\\=", 2) ? 0 : 1

      ;  (*n_lines_read)++

      ;  if (!inline_file_token)
         {  while (p<z)
            {  int c = *p
            ;  if (esc)
               {  esc = 0
                                   /*  we don't recognize comments anymore
                                    *  case \\{!} (still parsing)
                                   */
               ;  if (discardcmt)
                  NOTHING
               ;  else             /* stuff below is quick+dirty */
                  {  if (c == ':')
                     {  if ((c = p[1]) == '{')
                        {  if (!strncmp(p+1, "{!}", 3))
                           discardcmt = 'E'        /* escape, big time */
                        ;  else if (!strncmp(p+1, "{/}", 3))
                           discardcmt = 'I'        /* inclusive */
                        ;  else
                           yamErrx(1, me, "unrecognized comment sequence")
                     ;  }
                        else if (c == '!')
                        discardcmt = 'e'           /* escape, small time */
                     ;  else if (c == '/')
                        discardcmt = 'i'           /* inclusive */
                     ;  else
                        discardcmt = 'r'           /* regular   */
                  ;  }                 /* note: *p == ':' */
                     if (discardcmt)
                     {  if (discardcmt == 'e' || discardcmt == 'E')
                        P = p      /*  must keep on parsing to account for curlies
                                    *  it's a special case hack, too bad
                                   */
                     ;  else
                        break
                  ;  }
                  }
               }
               else if (c == '\\')
               esc = 1
            ;  else if (c == '{')
               level++
            ;  else if (c == '}')
               level--

            ;  p++
         ;  }
                           /* Dangersign: splice, append
                            * may corrupt p, P
                            * so we cannot use them afterwards.
                           */
            if (discardcmt == 'e')             /* s/:!//       */
            mcxTingSplice(line, "", P-a, 2, 0)
         ;  else if (discardcmt == 'E')        /* s/:{!}//     */
            mcxTingSplice(line, "", P-a, 4, 0)
         ;  else if (discardcmt == 'i' || discardcmt == 'I')   /* s/\\:.*\n//    */
               mcxTingShrink(line, p-a-1)
            ,  n_deleted++
         ;  else if (discardcmt == 'r')        /* s/\\:.*\n/\n/  */
               mcxTingShrink(line, p-a-1)
            ,  mcxTingAppend(line, "\n")

         ;  mcxTingAppend(filetxt, line->str)
         ;  bytes_read += line->len
                  /* fixme: also need to do this in case chunk is finished.
                   * (is this fixme still relevant and what is it about?)
                  */
         ;  if (n_deleted && discardcmt != 2)  /* so if range has finished */
            {  if (lcc_update)
               lcc_update(n_deleted, *n_lines_read, data)
            ;  n_deleted = 0
         ;  }
         }

         if (inline_file_token && !am_inline)
         {  mcxKV    *kv
         ;  mcxTing *fname = NULL,  *masked = mcxTingEmpty(NULL, 1000)
         ;  long n_lines_read_inline = 0
         ;  int l = 0

         ;  fname = yamBlock(line, 2, &l, NULL)

         ;  if (!fname)
            yamErrx(1, me, "format err in \\={fname} specification")

         ;  kv = mcxHashSearch(fname, mskTable_g, MCX_DATUM_INSERT)
         ;  if (kv->key != fname)
            yamErrx(1, me, "file <%s> already masked", fname->str)

         ;  mcxTell(me, "=== reading am_inline file <%s>", fname->str)

         ;  if
            (  STATUS_DONE
            == yamReadChunk
               (xf, masked, READ_INLINE, 0, &n_lines_read_inline, NULL, NULL)
            )
            {  kv->val = masked
            ;  lcc_update
               (n_lines_read_inline+1, *n_lines_read+n_lines_read_inline+1, data)
            ;  *n_lines_read += n_lines_read_inline
            ;  continue
         ;  }
            else
            yamErrx(1, me, "error reading am_inline file <%s>", fname->str)
      ;  }

         else if (inline_file_token && am_inline)
         {  mcxTell(me, "=== done reading am_inline file")

         ;  if (line->str[2] != '=')
            yamErrx(1, me, "expecting closing '\\==' at line <%d>",xf->lc)

         ;  mcxTingFree(&line)
         ;  return STATUS_DONE   /* fixme; move this return to end */
      ;  }

         if (level < 0)
            mcxErr
            (  me
            ,  "Curly underflow in file <%s> at line <%ld>"
            ,  xf->fn->str
            ,  (long) xf->lc
            )
         ,  level = 0

      ;  if
         (  !am_inline
         && chunk_size
         && bytes_read >= chunk_size
         && discardcmt != 2
         && level <= 0
         )
         {  stat = STATUS_OK
         ;  if (n_deleted)                /* tidy up */
            lcc_update(n_deleted, *n_lines_read, data)
         ;  break
      ;  }
      }
      mcxTingFree(&line)

#if DEBUG
;  fprintf(stderr, "read <%ld> bytes\n", (long) xf->bc)
#endif

   ;  if (am_inline)
      yamErrx(1, me, "inline file scope not closed!\n")

   ;  return stat
;  }


void mod_read_init
(  int n
)
   {  mskTable_g = yamHashNew(n)
;  }


void mod_read_exit
(  void
)
   {  if (mskTable_g)
      mcxHashFree(&mskTable_g, mcxTingRelease, mcxTingRelease)
;  }


const mcxTing* yamInlineFile
(  mcxTing* fname
)
   {  mcxKV* kv =    mskTable_g
                  ?  mcxHashSearch(fname, mskTable_g, MCX_DATUM_FIND)
                  :  NULL
   ;  return kv ? (mcxTing*)kv->val : NULL
;  }


void yamUnprotect
(  mcxTing* txt
)
   {  char* p  =  txt->str
   ;  char* q  =  txt->str
   ;  int   esc=  0  
   
   ;  while(p<txt->str+txt->len)
      {
         if (esc)
         {  if (*p == '\\' || *p == '{' || *p == '}')
            *(q++) = *p
         ;  else
            {  *(q++) = '\\'
            ;  *(q++) = *p
         ;  }
            esc = 0
      ;  }
         else if (*p == '\\')
         esc = 1
      ;  else
         *(q++) = *p

      ;  p++
   ;  }

      *q = '\0'
   ;  txt->len = q-txt->str
;  }


mcxTing*  yamProtect
(  mcxTing* data
)
   {  dim      i     =   0
   ;  dim      j     =   0
   ;  int      n     =   data->len + (data->len/8)

   ;  mcxTing* protected   =  mcxTingEmpty(NULL, n)

   ;  for(i=0;i<data->len;i++)
      {  char c      =  *(data->str+i)
      ;  int esc     =  (c == '\\' || c == '{' || c == '}') ? 1 : 0

      ;  if (j+esc >= protected->mxl)
         mcxTingEnsure(protected, protected->mxl + (protected->mxl/3) + 2)

      ;  if (esc)
         *(protected->str+j++) = '\\'

      ;  *(protected->str+j++) = c
   ;  }

      protected->len =  j
   ;  *(protected->str+j) = '\0'
   ;  return protected
;  }


mcxTing*  yamReadData
(  mcxIO          *xf
,  mcxTing        *filetxt
)
#define BUFLEN 1024
   {  char     cbuf[BUFLEN]
   ;  short    c
   ;  int      i        =  0

   ;  if (!xf->fp)
         mcxIOerr(xf, "yamReadData PBD", "is not open")
      ,  mcxExit(1)

   ;  filetxt = mcxTingEmpty(filetxt, 0)

   ;  while
      (  c = mcxIOstep(xf)
      ,  c != EOF
      )
      {  if (c == '\\' || c == '{' || c == '}')
         cbuf[i++] =  (char) '\\'

      ;  cbuf[i] = (char) c

      ;  if (i+2 >= BUFLEN)      /* if not, no room for double bs */
         {  mcxTingNAppend(filetxt, cbuf, i+1)
         ;  i = 0
         ;  continue
      ;  }
         i++
   ;  }

      if (i)               /* nothing written at this position */
      mcxTingNAppend(filetxt, cbuf, i)

   ;  xf->ateof   =  1
   ;  return filetxt
#undef BUFLEN
;  }


mcxIO* yamRead__open_file__
(  const char* name
)
   {  mcxIO *xf = mcxIOnew(name, "r")
   ;  if (mcxIOopen(xf, RETURN_ON_FAIL) == STATUS_OK)
      return xf
   ;  else
      mcxIOfree(&xf)
   ;  return NULL
;  }



/*
 *    I had some trouble getting the details right.  These are the different
 *    cases that once caused trouble: Assume a file t.azm containing
 *
 *    \write{bar}{txt}{1 2 3 4 hoedje van papier}
 *    \system{date}{}{}
 *    \system{date}{}{}
 *    \system{date}{}{}
 *    \system{date}{}{}
 *
 *    and consider the command lines
 *
 *    zoem -i t -o -
 *    zoem -i t -o foo
 *    zoem -i t -o - > foo
 *
 *    All these cases were buggy under different circumstances; pipe ends need
 *    to be closed, data needs to be flushed, and open file descriptors
 *    need to be closed.
*/

mcxTing* yamSystem
(  char* cmd
,  char* argv[]
,  mcxTing* data
)
#define LDB (tracing_g & ZOEM_TRACE_SYSTEM)
#define  RB_SIZE 4096
   {  int   stat1, stat2, xstat, k
   ;  int   fx[2]
   ;  int   fy[2]
   ;  pid_t cp1, cp2
   ;  char  rb[RB_SIZE]

   ;  int statusx =  pipe(fx)
   ;  int statusy =  pipe(fy)

   ;  if (statusx || statusy)
      {  yamErr("\\system#3", "pipe error")
      ;  perror("\\system#3")
      ;  return NULL
   ;  }

      yamUnprotect(data)
   ;  if (fflush(NULL))
      perror("flush")

   ;  cp1 = fork()

   ;  if (cp1 == -1)       /* error */
      {  yamErr("\\system#3", "fork error")
      ;  perror("\\system#3")
      ;  return NULL
   ;  }
      else if (cp1 == 0)  /* child */
      {  if (LDB) printf(" sys| CHILD closing x1=%d and y0=%d\n", (int) fx[1], (int) fy[0])

      ;  if (close(fx[1])<0) perror("close fx[1]")  /* close parent output */
      ;  if (close(fy[0])<0) perror("close fy[0]")  /* close child input  */

      ;  if (LDB)
         {  printf(" sys| CHILD mapping %d to %d\n",(int)fx[0], (int) fileno(stdin))
         ;  printf(" sys| CHILD mapping %d to %d\n",(int)fy[1], (int) fileno(stdout))
      ;  }

         dup2(fx[0], fileno(stdin))
      ;  dup2(fy[1], fileno(stdout))

      ;  if (close(fx[0])<0) perror("close fx[0]")
      ;  if (close(fy[1])<0) perror("close fy[1]")

      ;  switch ((cp2 = fork()))
         {  case -1
         :  perror("fork")
         ;  exit(1)
         ;

            case 0               /* grandchild */
         :  execvp(cmd, argv)    /* do the actual stuff */
         ;  yamErrx(1, "\\system#3", "exec <%s> failed (bummer)", cmd)
         ;  break
         ;

            default              /* child */
         :
if(0)fprintf(stderr, "START child\n")
         ;  if (waitpid(cp2, &stat2, 0)< 0 || !WIFEXITED(stat2))
            {  if (0)                  /* unreachcode */
               perror("child")
         ;  }

            while (0 && (k = read(fileno(stdin), rb, RB_SIZE)))
            {  if (LDB)
               fprintf
               (  stderr
               ," sys| CHILD discarding %d bytes\n", (int) k
               )
         ;  }
           /* The discard loop is for this:
            * \system{cat}{{-#}}{abcdef} (i.e. failing command)
            * we need someone to read the stdin, lest we get a broken pipe.
            * (and we want \system never to fail in zoem).
           */

           /*  Do I need the close below? e.g. wrt 'zoem -i t -o - > foo'.
            *  Not too clear on these issues, but it seems best
            *  practice to close all pipe ends when done.
           */
         ;  if (close(fileno(stdin))<0)
            perror("close fileno(stdin)")
         ;  if (close(fileno(stdout))<0)
            perror("close fileno(stdout)")
         ;  if (stat2)
            yamErr("\\system#3", "child <%s> failed", cmd)
         ;  exit(stat2 ? 1 : 0)
      ;  }
      }
      else /* parent */
      {  mcxTing* buf   =  mcxTingEmpty(NULL, 80)
      ;  mcxTing* res   =  NULL
      ;  static int ct = 0

      ;  if (LDB)
         printf
         (" sys| PARENT closing x0=%d and y1=%d\n", (int) fx[0], (int) fy[1])

      ;  close(fy[1])                  /* close output     */
      ;  close(fx[0])                  /* close input      */
      ;  if (data->len)                /* mq, must not exceed buffer size */
         {  fprintf(stderr, "<%d> bytes written\n", (int) data->len)
         ;  write(fx[1], data->str, data->len)
      ;  }
         close(fx[1])

      ;  ct++

      ;  while ((k = read(fy[0], rb, RB_SIZE)))
         {  if (k<0)
            {  perror("read")
            ;  break
         ;  }
            if (buf->len + k > buf->mxl)
            mcxTingEnsure(buf, buf->mxl  + buf->mxl/2)
         ;  mcxTingNAppend(buf, rb, k)
      ;  }
         close(fy[0])
      ;  if ((waitpid(cp1, &stat1, 0)) < 0 || !WIFEXITED(stat1))
         perror("waiting parent")
      ;  xstat = WIFEXITED(stat1) ? WEXITSTATUS(stat1) : 1
      ;  yamKeySet("__sysval__", xstat ? "1" : "0")

      ;  if (!xstat)
         res = yamProtect(buf)
      ;  else
         res = NULL

      ;  mcxTingFree(&buf)
      ;  if (res)
         {  while
            (  res && res->len
            && isspace((unsigned char) res->str[res->len-1])
            )
            res->len -= 1
         ;  res->str[res->len] = '\0'
      ;  }

      ;  return res
   ;  }
      return NULL
;  }
#undef LDB
#undef RB_SIZE


mcxIO* yamTryOpen
(  mcxTing*  fname
,  const mcxTing** inline_txt_ptr
,  mcxbool useSearchPath
)
#define LDB (tracing_g & ZOEM_TRACE_IO)
   {  mcxIO *xf = NULL
   ;  mcxTing* file = NULL
   ;  char* envp
   ;  char* wherefound = "nowhere"

   ;  if (fname->len > 1024)
      {  yamErr("yamTryOpen", "input file name expansion too long (>1024)")
      ;  *inline_txt_ptr = NULL
      ;  return NULL
   ;  }

   /* .  first check inline file
    * .  then check file name as is.
    * .  then check $ZOEMSEARCHPATH
    * .  then check __searchpath__
    * .  then check sourceGetPath (last file openened)
   */

                           /* .. then we may look for inline files */
      if (inline_txt_ptr)
      {  const mcxTing* filetxt = yamInlineFile(fname)
      ;  if (filetxt)
         {  *inline_txt_ptr = filetxt
         ;  if (LDB)
            mcxTell(NULL, "=== using inline file <%s>", fname->str)
         ;  return mcxIOnew(fname->str, "r")
      ;  }
      }

      if (LDB)
      printf("<IO>| trying (cwd) <%s>\n", fname->str)
   ;  xf = yamRead__open_file__(fname->str)
   ;  if (xf)
      wherefound = "in current working directory"

   /*
    * Try to find a readable file in a few more places.
   */

   ;  if (!xf && useSearchPath && (envp = getenv("ZOEMSEARCHPATH")))
      {  mcxTing* zsp = mcxTingNew(envp)
      ;  char *q, *p = zsp->str
      ;  mcxTingTr(zsp, ":[:space:]", " [* *]", NULL, "[:space:]", 0)
      ;  file  = mcxTingEmpty(file, 30)
      ;  while (p)
         {  while (isspace((unsigned char) *p))
            p++
         ;  q = mcxStrChrIs(p, isspace, -1)
         ;  if (q)
            mcxTingNWrite(file, p, q-p)
         ;  else
            mcxTingWrite(file, p)

         ;  if (file->len && file->str[file->len-1] != '/')
            mcxTingAppend(file, "/")

         ;  mcxTingAppend(file, fname->str)

         ;  if (LDB)
            printf("<IO>| trying (env) <%s>\n", file->str)
         ;  if ((xf = yamRead__open_file__(file->str)))
            {  mcxTingWrite(fname, file->str)
            ;  wherefound = file->str
            ;  break
         ;  }
            p = q
      ;  }
         mcxTingFree(&zsp)
   ;  }

      if (!xf && useSearchPath)
      {  mcxTing* tmp = mcxTingNew("__searchpath__")
      ;  mcxTing* sp  = yamKeyGet(tmp)
      ;  yamSeg*  spseg = sp ? yamStackPushTmp(sp) : NULL

      ;  file  = mcxTingEmpty(file, 30)

      ;  while (sp && yamParseScopes(spseg, 1, 0) == 1)
         {  mcxTingPrint(file, "%s/%s", arg1_g->str, fname->str)
         ;  if (LDB)
            printf("<IO>| trying (var) <%s>\n", file->str)
         ;  if ((xf = yamRead__open_file__(file->str)))
            {  mcxTingWrite(fname, file->str)
            ;  wherefound = file->str
            ;  break
         ;  }
         }
         mcxTingFree(&tmp)
      ;  yamStackFreeTmp(&spseg)
   ;  }

      if (!xf)
      {  mcxTingFree(&file)      /* ugly hack ensuring memcleannes */
      ;  file = sourceGetPath()
      ;  if (file)
         {  mcxTingAppend(file, fname->str)
         ;  if (LDB)
            printf
            ("<IO>| trying (his) <%s> path stripped from last include\n"
            ,  file->str
            )
         ;  if ((xf = yamRead__open_file__(file->str)))
               mcxTingWrite(fname, file->str)
            ,  wherefound = file->str
      ;  }
      }

      if (LDB)
      printf("<IO>| found %s\n", wherefound)

   ;  mcxTingFree(&file)
   ;  return xf
;  }


