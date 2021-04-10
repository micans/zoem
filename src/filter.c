/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <ctype.h>
#include <unistd.h>

#include "filter.h"
#include "curly.h"
#include "util.h"
#include "ops-constant.h"
#include "digest.h"
#include "sink.h"
#include "key.h"
#include "iface.h"
#include "parse.h"

#include "tingea/alloc.h"
#include "tingea/minmax.h"
#include "tingea/list.h"
#include "tingea/ding.h"

/* filterAt is called by:
 *    filterDevice
 *    yamputc in plain mode.
 *    yamPutConstant
 *    yamSpecialPut
 *
 * yamputc in plain mode is called by:
 *    filterDevice
 *
 * yamputc in at mode is called by:
 *    filterAt
 *
 * yamPutConstant is called by
 *    filterDevice
 *
 * yamSpecialPut is called by
 *    filterDevice
 *
 * filterTxt calls none of the above
 * filterCopy calls none of the above
*/


/* n_newlines is the number of newlines that form the trailing
 * part of the relevant output sink.
 * If the last character was not a newline, this count is zero.
 * If the last N characters were newlines, this count is N.
*/

struct filter
{  int            indent
;  int            n_newlines     /* currently flushed newline count */
;  int            n_spaces       /* currently *stacked* space count */
;  int            doformat
;  int            level
;  FILE*          fp
;
}  ;


mcxstatus filterDevice
(  filter* fd
,  mcxTing*       txt
,  int            offset
,  int            bound
)  ;

void yamputc
(  filter*   fd
,  unsigned char  c
,  int            atcall
)  ;



#define N_SPECIAL       259
#define SPECIAL_SPACE   256
#define SPECIAL_BREAK   257
#define SPECIAL_DASH    258

typedef struct yamSpecial
{  mcxTing*          txt
;  struct yamSpecial*up
;  
}  yamSpecial        ;


static yamSpecial specials[N_SPECIAL];
static mcxTing* zconstant = NULL;
static mcxTing* ampersand = NULL;
static mcxTing* semicolon = NULL;


int yamAtDirective
(  filter*    fd
,  char       c
)  ;


mcxstatus filterAt
(  filter*    fd
,  mcxTing*   txt
,  int        offset
,  int        length
)  ;


mcxstatus filterTxt
(  filter*    fd
,  mcxTing*   txt
,  int        offset
,  int        length
)  ;


mcxstatus filterStrip
(  filter*    fd
,  mcxTing*   txt
,  int        offset
,  int        length
)  ;


mcxstatus filterCopy
(  filter*    fd
,  mcxTing*   txt
,  int        offset
,  int        length
)  ;


fltfnc flts[6]
=  {  NULL
   ,  filterDevice
   ,  filterDevice
   ,  filterTxt
   ,  filterCopy
   ,  filterStrip
   }  ;


int ZOEM_FILTER_NONE   =   0;
int ZOEM_FILTER_DEFAULT=   1;
int ZOEM_FILTER_DEVICE =   2;
int ZOEM_FILTER_TXT    =   3;
int ZOEM_FILTER_COPY   =   4;
int ZOEM_FILTER_STRIP  =   5;


#define F_DEVICE     "device filter (customize with \\special#1)"
#define F_TXT        "interprets [\\\\][\\~][\\,][\\|][\\}][\\{]"
#define F_COPY       "identity filter (literal copy)"



void yamPutConstant
(  filter* fd
,  const char* p
,  int l
)  ;

void yamSpecialPut
(  filter* fd
,  unsigned int c
)  ;


/*    @  literal at newline in text filter.
 *    |  \| in text filter.
 *    .  literal plain newline in text or copy filter.
 *    %  format newline in at scope.
 *    +  literal plain newline in at scope.
 *    ~  literal plain newline in plain scope.
 *    >  als filtering aan staat en er een niet lege string gefilterd wordt.
*/

void yamputnl
(  const char *c
,  FILE* fp
)  ;


void filterFree
(  filter* fd
)
   {  mcxFree(fd)
;  }


filter* filterNew
(  FILE* fp
)
   {  filter* fd =  mcxAlloc(sizeof(filter), EXIT_ON_FAIL)
   ;  fd->indent        =  0
   ;  fd->n_newlines    =  1     /* count of flushed newlines */
   ;  fd->n_spaces      =  0     /* count of stacked spaces   */
   ;  fd->doformat      =  1
   ;  fd->level         =  1
   ;  fd->fp            =  fp
   ;  return fd
;  }


mcxstatus filterTxt
(  filter*    fd
,  mcxTing*          txt
,  int               offset
,  int               length
)
   {  int   esc   =  0
   ;  char* o     =  txt->str + offset
   ;  char* z     =  o + length
   ;  char* p
   ;  FILE* fp

                        /* fixme: should get default data.  so, default data
                         * should not be in stack.  no use in having fd in
                         * stack, as it is passed around.
                         * Why not pass filedata around? then you must always,
                         * and cannot rely on default value.
                        */
   ;  fd          =  fd ? fd : sink_default_g->fd
   ;  fp          =  fd->fp

   ;  for (p=o;p<z;p++)
      {
         if (esc)
         {
            switch(*p)
            {  
               case '@'
            :  {  int l
               ;  if ((unsigned char) p[1] == 'e')       /* (+below) uggggglllllyy, fix up sometime */
                  p++
               ;  l  =  yamClosingCurly(txt, offset + (p-o)+1, NULL, RETURN_ON_FAIL)

               ;  if (l<0 || offset+(p-o)+1+l >= offset + length)
                  {  yamErr("filterTxt PBD", "scope error!")
                  ;  return STATUS_FAIL
               ;  }

                  fputs("\\@", fp)
               ;  if ((uchar) p[0] == 'e')
                  fputc('e', fp)

               ;  while(--l && ++p)
                  {  if (*p == '\n')
                     yamputnl("@l", fp)             /* lit at nl in txt flt */
                  ;  else
                     fputc(*p, fp)
               ;  }

                  break
            ;  }

               case '\\'   :  fputc('\\', fp)   ;  break
            ;  case '|'    : yamputnl("\\|", fp);  break    /* \| in txt flt */
            ;  case '-'    :  fputc('-', fp)    ;  break
            ;  case '~'    :  fputc(' ', fp)    ;  break    /* wsm */
            ;  case '{'    :  fputc('{', fp)    ;  break
            ;  case '}'    :  fputc('}', fp)    ;  break
            ;  case ','    :                    ;  break
            ;  case '\n'   :                    ;  break
            ;  default     :  fputc(*p, fp)     ;  break
         ;  }
            esc = 0
      ;  }
         else if (*p == '\\')
         esc = 1
      ;  else
         {  if (*p == '\n')
            yamputnl("nl", fp)   /* lit plain nl in txt flt */
         ;  else
            fputc(*p, fp)
      ;  }
      }

   ;  return STATUS_OK
;  }


mcxstatus filterStrip
(  filter*    fd
,  mcxTing*   txt
,  int        offset
,  int        length
)
   {  return STATUS_OK
;  }


mcxstatus filterCopy
(  filter*    fd
,  mcxTing*   txt
,  int        offset
,  int        length
)
   {  char* p
   ;  FILE* fp

   ;  fd =  fd ? fd : sink_default_g->fd
   ;  fp =  fd->fp

   ;  for (p=txt->str+offset;p<txt->str+offset+length;p++)
      {  if (*p == '\n')
         yamputnl("nl", fp)       /* lit nl (anywhere) in copy flt */
      ;  else
         fputc(*p, fp)
   ;  }

      return STATUS_OK
;  }



enum
{  F_MODE_ATnESC = 1
,  F_MODE_AT
,  F_MODE_ESC
,  F_MODE_DEFAULT
}  ;


int yamAtDirective
(  filter*    fd
,  char              c
)
   {  FILE* fp = fd->fp

   ;  switch(c)
      {  case 'N'
         :  while (fd->n_newlines < 1)
            {  fd->n_newlines++
            ;  yamputnl("@N", fp) /* '%' nl in at scope */
         ;  }
            fd->n_spaces = 0     /* flush spaces */
         ;  break
      ;  case 'P'
         :  while (fd->n_newlines < 2)
            {  fd->n_newlines++
            ;  yamputnl("@P", fp) /* '%' nl in at scope */
         ;  }
            fd->n_spaces = 0     /* flush spaces */
         ;  break
      ;  case 'I'
         :  fd->indent++
         ;  break
      ;  case 'J'
         :  fd->indent = MCX_MAX(fd->indent-1, 0)
         ;  break
      ;  case 'n'
         :  yamputnl("@n", fp) /* '%' nl in at scope */
         ;  fd->n_newlines++     /* yanl */
         ;  fd->n_spaces = 0     /* flush spaces */
         ;  break
      ;  case 'S'
         :  fd->n_spaces++       /* stack a space */
         ;  break
      ;  case 's'
         :  fputc(' ', fp)
         ;  fd->n_spaces = 0     /* flush other spaces */
         ;  fd->n_newlines = 0   /* no longer at bol */
         ;  break
      ;  case 't'
         :  yamputc(fd, '\t', 1)
         ;  break
      ;  case 'C'
         :  fd->indent = 0
         ;  break
      ;  default
         :  yamErr("yamAtDirective", "unsupported '%%' option <%c>", c)
   ;  }
      return 1
;  }


/*
 *  We keep track of spaces and newlines. If we print by yamputc,
 *  yamputc does that for us. If we print by fputc, we do it ourselves.
 *
 *  we ignore *(txt->offset+length) (it will be '\0' or '}' or so).
*/

mcxstatus filterAt
(  filter*    fd
,  mcxTing*   txt
,  int        offset
,  int        length
)
   {  char* o           =  txt->str + offset
   ;  char* z           =  o + length
   ;  char* p           =  o
   ;  const char* me    =  "filterAt"

   ;  int fltmode       =  F_MODE_AT
   ;  mcxbool xml_sentinel = FALSE

   ;  while (p<z)
      {
         unsigned char c = (unsigned char) *p

      ;  switch(fltmode)
         {  
            case F_MODE_ATnESC
         :  switch(c)
            {
               case '}'
            :  case '{'
            :  case '\\'
            :  case '"'
            :     yamputc(fd, c, 1)
               ;  break
               ;
               case ','
            :  case '\n'
            :     break
               ;
               case '*'
            :     yamErr
                  (  me
                  ,  "constant keys not allowed in at scope - found <\\%s>"
                  ,  p
                  )
               ;  return STATUS_FAIL
               ;  
               case '&'
            :
                                          /* fixme; yamBlock? while(1) */
               {  int x =  yamClosingCurly
                           (txt, offset + p-o+1, NULL, RETURN_ON_FAIL)
               ;  mcxTing* stuff
               ;  if (x<0)
                  {  yamErr("filterAt PBD", "scope error!")
                  ;  return STATUS_FAIL
               ;  }
                  stuff =  mcxTingNNew(p+2, x-1)
               ;  if (yamDigest(stuff, stuff, NULL))
                  {  yamErr(me, "AND scope did not parse")
                  ;  mcxTingFree(&stuff)
                  ;  return STATUS_FAIL
               ;  }
                  if (filterAt(fd, stuff, 0, stuff->len))
                  {  yamErr(me, "AND scope did not filter")
                  ;  mcxTingFree(&stuff)
                  ;  return STATUS_FAIL
               ;  }
                  mcxTingFree(&stuff)
               ;  p += x+1
            ;  }
                  break
               ;
               case '+'
            :
               ;  if
                  (  p+3 >= z
                  || p[1] != '{'
                  || !isdigit((unsigned char) p[2])
                  || p[3] != '}'
                  )
                  yamErr(me, "+ requires {k} syntax, k in 0-9")
               ;  else
                  fd->level = ((unsigned char) p[2]) - '0'
               ;  p += 3
               ;  break
               ;
               case '~'
            :  case '|'
            :  case '-'
            :     yamErr
                  (  me
                  ,  "zoem glyphs not allowed in at scope - found <\\%c>"
                  ,  c
                  )
               ;  return STATUS_FAIL
               ;
               case 'X'
            :  xml_sentinel = TRUE
            ;  break
            ;

               case 'w'
            :  fd->doformat = 0
            ;  break
            ;  
               
               case 'W'
            :  fd->doformat = 1
            ;  break
            ;

               case 'N'
            :  case 'n'
            :  case 'P'
            :  case 'p'
            :  case 'C'
            :  case 'J'
            :  case 'I'
            :  case 's'
            :  case 'S'
            :     if (xml_sentinel && !fd->doformat)
                  NOTHING
               ;  else
                  yamAtDirective(fd, *p)
               ;  break
               ;
               default
            :     yamErr(me, "unknown escape <%c>", c)
         ;  }

            fltmode  =  F_MODE_AT
         ;  break
         ;

            case F_MODE_AT
         :  switch(c)
            {
               case '\\'
               :  fltmode = F_MODE_ATnESC
               ;  break
            ;  default
               :  yamputc(fd, c, 1)
         ;  }
            break
         ;
      ;  }
         p++
   ;  }

      return STATUS_OK
;  }



mcxstatus filterDevice
(  filter*      fd
,  mcxTing*     txt
,  int          offset
,  int          length
)
   {  char* o           =  txt->str + offset
   ;  char* z           =  o + length
   ;  char* p           =  o
   ;  const char* me    =  "filterDevice"
   ;  int   x

   ;  int fltmode       =  F_MODE_DEFAULT

   ;  fd                =  fd ? fd : sink_default_g->fd
   ;
  /*
   *  we must enter me in this mode by design, and this deserves more
   *  explanation. docme, unfbus.
   *
  */

      while (p<z)
      {
         unsigned char c = (unsigned char) *p

      ;  switch (fltmode)
         {
            case F_MODE_ESC
         :  switch(c)
            {
               case '@'
               :  p++
               ;  if ((uchar) p[0] == 'e')
                  p++
               ;  x = yamClosingCurly(txt, offset + p-o, NULL, RETURN_ON_FAIL)
               ;  if (x<0 || offset+(p-o)+x >= offset + length)
                  {  yamErr(me, "PBD scope error?!")
                  ;  return STATUS_FAIL
               ;  }
                                      /*  *(txt->str+offset+(p-o)+x) == '}' */
                  if ((uchar) p[-1] == 'e')
                  {  if (filterAt(fd, ampersand, 0, 1)) return STATUS_FAIL
                  ;  if (filterAt(fd, txt, offset + p-o+1, x-1)) return STATUS_FAIL
                  ;  if (filterAt(fd, semicolon, 0, 1)) return STATUS_FAIL
               ;  }
                  else if (filterAt(fd, txt, offset + p-o+1, x-1))
                  return STATUS_FAIL
               ;  p += x
               ;  break

            ;  case '*'
               :  {  int l = yamEOConstant(txt, offset + p-o)
                  ;  if (l<0 || offset+(p-o)+1+l > offset + length)
                     {  yamErr(me, "PBD const!")
                     ;  return STATUS_FAIL
                  ;  }
                     yamPutConstant(fd, p, l)
                  ;  p += l
                  ;  break
               ;  }

               case '}' :  case '{' :  case '\\'
               :  yamputc(fd, c, 0)
               ;  break
            ;  case '~'
               :  yamSpecialPut(fd, 256)
               ;  break
            ;  case '|'
               :  yamSpecialPut(fd, 257)
               ;  break
            ;  case '-'
               :  yamSpecialPut(fd, 258)
               ;  break
            ;  case ',' :  case '\n'
               :  break
               ;
            ;  default
               :  yamErr(me, "ignoring escape <\\%c>", c)
               ;  yamputc(fd, '\\', 0)    /* wsm */
               ;  yamputc(fd, c, 0)    /* wsm */
               ;  break
         ;  }
            fltmode  =  F_MODE_DEFAULT
         ;  break
         ;

            case F_MODE_DEFAULT
         :  switch(c)
            {
               case '\\'
               :  fltmode = F_MODE_ESC
               ;  break
            ;  default
               :  yamputc(fd, c, 0)    /* wsm */
         ;  }
            break
         ;

      ;  }
      ;  p++
   ;  }

      if (fltmode == F_MODE_ESC && length == 1)
      yamErr(me, "skipping dangling backslash (resulting from \\!)")
   ;  else if (fltmode == F_MODE_ESC)
      yamErr(me, "unexpected dangling backslash, please file bug report")

   ;  return STATUS_OK
;  }


mcxTing* specialGet
(  int level
,  unsigned int c
)
   {  yamSpecial* spec = &specials[c]

   ;  if (!level)
      return NULL

   ;  while (--level && spec->up)
      spec = spec->up
   ;  return spec->txt
;  }


void yamputc
(  filter*        fd
,  unsigned char  c
,  int            atcall
)
   {  FILE* fp = fd->fp
   ;  mcxTing* special
      =     atcall || !specials[c].txt
         ?  NULL
         :  specialGet(fd->level, c)

   ;  if (special && (c == ' ' || c == '\n'))
      {  filterAt(fd, special, 0, special->len)
        /* we don't do bookkeeping or squashing on mapped nl's and sp's
         * perhaps we could do so, but a design is needed first.
         *
         * fixme what if we want indent for '\n' ?
        */
   ;  }
      else if (fd->doformat)
      {
         if (c == ' ')
         {  if (!fd->n_newlines)
            fd->n_spaces++       /* do not stack spaces at bol */
      ;  }
         else if (c == '\n')
         {  if (fd->n_newlines < 1)
            {  fd->n_newlines++
      /* yamputnl,yamputc,doformat: @/. lit nl in at or plain scope */
            ;  yamputnl(atcall ? "@l" : "nl", fp)
         ;  }
            fd->n_spaces = 0     /* flush all spaces when newline */
      ;  }
         else
         {  if (fd->n_newlines)  /* \@{\S}foo -- the f passes through here */
            {  int i       =  0
            ;  for (i=0;i<ztablength_g*fd->indent;i++)
               fputc(' ', fp)
            ;  fd->n_newlines = 0
            ;  fd->n_spaces   = 0
         ;  }
            else if (fd->n_spaces)
            {  fputc(' ', fp)
            ;  fd->n_spaces = 0
         ;  }

            if (special)
            filterAt(fd, special, 0, special->len)
         ;  else
            fputc(c, fp)
      ;  }
      }
      else
      {  if (c == '\n')
         fd->n_newlines++     /* note: we still output below. */
      ;  else
         fd->n_newlines = 0

      ;  if (c != ' ' && fd->n_spaces)
            fd->n_spaces = 0
         ,  fputc(' ', fp)

      ;  if (special)
         filterAt(fd, special, 0, special->len)
      ;  else
         {  int i = 0
         ;  if (c == '\n')
            {  yamputnl(atcall ?  "@L" : "NL" , fp)
      /* yamputnl,yamputc,dontformat: +/~ lit nl in at or plain scope */
                  /* fixmefixme: the output below should only be done in case
                   * non-blank follows.
                  */
            ;  for (i=0;i<ztablength_g*fd->indent;i++)
               fputc(' ', fp)
         ;  }
            else
            fputc(c, fp)      /* wsm */
      ;  }
      }
   }


void yamPutConstant
(  filter* fd
,  const char* p     /*  p[0] == '*' */
,  int l
)
   {  mcxTing* txt
   ;  int delta = p[1] == '{' ? 2 : 1
   ;  static long n_star_deprecated = 0

   ;  mcxTingNWrite(zconstant, p+delta, l-delta)
   ;  txt = yamConstantGet(zconstant)

   ;  if (txt)
      filterAt(fd, txt, 0, txt->len)
   ;  else
      yamErr("yamPutConstant", "warning: constant *%s* not found", zconstant->str)

   ;  if (delta == 1 && !n_star_deprecated++)
      yamErr
      (  "zoem"
      ,  "deprecated: \\*%s*, use \\*{%s} instead"
      ,  zconstant->str
      ,  zconstant->str
      )
;  }


void yamSpecialPut
(  filter* fd
,  unsigned int c
)
   {  mcxTing*  spc = specialGet(fd->level, c)
   ;  if (c < N_SPECIAL && spc)
      filterAt(fd, spc, 0, spc->len)
;  }


void yamSpecialSet
(  long c
,  const char* str
)
   {  if (c < 0)
         c *= -1
      ,  c += 255
   ;  if (c >= N_SPECIAL)
      return
   ;  if (!specials[c].txt)
      specials[c].txt = mcxTingNew(str)
   ;  else
      {  yamSpecial* spec = &specials[c]
      ;  while (spec->up)
         spec = spec->up
      ;  spec->up       =  mcxAlloc(sizeof(yamSpecial), EXIT_ON_FAIL)
      ;  spec->up->txt  =  mcxTingNew(str)
      ;  spec->up->up   =  NULL
   ;  }
;  }


void yamSpecialClean
(  void
)
   {  int i

   ;  for (i=0;i<N_SPECIAL;i++)
      {  yamSpecial *spec = &specials[i]
      ;  while (spec)
         {  yamSpecial *up   = spec->up
         ;  mcxTingFree(&spec->txt)
         ;  if (spec != &specials[i])
            mcxFree(spec)
         ;  else
               spec->txt = NULL
            ,  spec->up  = NULL
         ;  spec = up 
      ;  }
      }
;  }


void mod_filter_exit
(  void
)
   {  yamSpecialClean()
   ;  mcxTingFree(&zconstant)
   ;  mcxTingFree(&ampersand)
   ;  mcxTingFree(&semicolon)
;  }


void mod_filter_init
(  int n
)
   {  int i
   ;  for (i=0;i<N_SPECIAL;i++)
         specials[i].txt   =  NULL
      ,  specials[i].up    =  NULL
   ;  zconstant = mcxTingEmpty(NULL, 20)
   ;  ampersand = mcxTingNew("&")
   ;  semicolon = mcxTingNew(";")
;  }


void filterList
(  const char* mode
)
   {  mcxbool listAll = strstr(mode, "all") != NULL
   ;  if (listAll || strstr(mode, "filter"))
{  fputs("\nFilter names available for the \\write#3 command\n", stdout)
;  fputs("device          device filter (customize with \\special#1\n", stdout)
;  fputs("txt             interprets [\\\\][\\~][\\,][\\|][\\}][\\{]\n", stdout)
;  fputs("copy            identity filter (literal copy)\n", stdout)
;  }
   }


void yamputnl
(  const char  *c
,  FILE* fp
)
   {  fputc('\n', fp)
   ;  if (tracing_g & ZOEM_TRACE_OUTPUT)
      {  fputs(c, fp)
      ;  fputs("==|", fp)
   ;  }
   }


void filterLinefeed
(  filter* fd
,  FILE*   fp
)
   {  while (fd->n_newlines < 1)
      {  fputc('\n', fp)
      ;  fd->n_newlines++
   ;  }
   }


void filterSetFP
(  filter* fd
,  FILE*   fp
)
   {  fd->fp = fp
;  }



