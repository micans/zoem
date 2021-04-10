/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <ctype.h>
#include <stdio.h>

#include "parse.h"

#include "ops.h"
#include "source.h"
#include "iface.h"
#include "curly.h"
#include "segment.h"
#include "util.h"
#include "key.h"

#include "tingea/minmax.h"
#include "tingea/ding.h"
#include "tingea/inttypes.h"


static   const char* arg_padding_g[10] =  {  "#0" , "#1" , "#2"
                                          ,  "#3" , "#4" , "#5"
                                          ,  "#6" , "#7" , "#8"
                                          ,  "#9"
                                          }  ;

            int         tracing_g      =  0;
static      int         tracect_g      =  0;


mcxTing      key_and_args_g[YAM_ARG_MAX+1];

mcxTing*     key_g                     =  key_and_args_g+0;

mcxTing*     arg1_g                    =  key_and_args_g+1;
mcxTing*     arg2_g                    =  key_and_args_g+2;
mcxTing*     arg3_g                    =  key_and_args_g+3;
mcxTing*     arg4_g                    =  key_and_args_g+4;
mcxTing*     arg5_g                    =  key_and_args_g+5;
mcxTing*     arg6_g                    =  key_and_args_g+6;
mcxTing*     arg7_g                    =  key_and_args_g+7;
mcxTing*     arg8_g                    =  key_and_args_g+8;
mcxTing*     arg9_g                    =  key_and_args_g+9;
mcxTing*     arg10_g                   =  key_and_args_g+10;

int          n_args_g;     /* not a dim, because it engages in a few int computations */
                           /* no plans to support > 2147483648 (or > 32768) arguments */


mcxstatus yamParsekey
(  yamSeg    *line
,  mcxbits   *keybits
)  ;

static int tagoffset
(  const mcxTing      *txt
,  int         offset
)  ;

/* 
 *  -*=+H+=*-+-*=+H+=*-+-*=+H+=*-+-*=+H+=*-+-*=+H+=*-+-*=+H+=*-+-*=+H+=*-
 *
 *    all zoem primitives are handled in the same way, by an expand routine.
 *    expandUser handles all user macros.
 *    expansion will not construct  an illegal key because bsbs remains bsbs.
 *
 *    If anon is true, the k from '#k' is off by one and is ignored.
*/

yamSeg* expandUser
(  yamSeg*  seg
,  const mcxTing* user
,  mcxbool  anon
)
   {  yamSeg*  newseg   =  NULL
   ;  mcxTing* repl     =  mcxTingEmpty(NULL, 30)

   ;  int      po       =  0                    /* previous offset   */
   ;  int      o        =  0
   ;  int      delta    =  anon ? 1 : 0

   ;  if (n_args_g)
      {
         while( (o = tagoffset(user, o)) >= 0)
         {
            int   i  =  (uchar) *(user->str+o+1) - '0'

         ;  mcxTingNAppend(repl, user->str+po, o-po)   /* skip \k  */

         ;  if (i < 1 || i+delta > n_args_g)
            {  yamErr("expand", "argument \\%d out of range", i)
            ;  mcxTingFree(&repl)
            ;  break
         ;  }

            mcxTingAppend(repl, (key_and_args_g+delta+i)->str)
         ;  o       +=  2
         ;  po       =  o
      ;  }
         if (o == -2)            /* no closing curly */
         mcxTingFree(&repl)
   ;  }

      if (repl)
      mcxTingNAppend(repl, user->str+po, user->len-po)

   ;  newseg = repl ? yamSegPush(seg, repl) : NULL
   ;  return newseg
;  }



/*
 *    returns l such that txt->str[offset+l] starts "\\k", k = argnum.
 *    It skips any bang scope, i.e. \!{....}
 *    I am not entirely sure whether this is a feature ...
 *    Must have to do with funky k-stage nested application of keys.
*/

static int tagoffset
(  const mcxTing      *txt
,  int         offset
)
   {  char* o     =  txt->str + offset
   ;  char* p     =  o
   ;  char* z     =  txt->str + txt->len

   ;  int   esc   =  p && (*p == '\\')
   ;  int   cc

   ;  while (++p < z)
      {  if (esc)
         {  if (*p >= '1' && *p <= '9')
            return (offset + (p-o-1))
         ;  else if (*p == '!')           /* skip tags in delay scope */
            {  while (*++p == '!')
               NOTHING
            ;  if (*p == '{')
               {  cc = yamClosingCurly(txt, offset+(p-o), NULL, RETURN_ON_FAIL)
               ;  if (cc<0)
                  {  yamErr("tagoffset", "unable to close \\! scope\n")
                  ;  return -2
               ;  }
                  p += cc
            ;  }
               p--      /* fixme; now *before* '}' ?? */
         ;  }
            esc = 0
      ;  }
         else if (*p == '\\')
         esc = 1
   ;  }
      return -1
;  }



/*
 * returns index of '{' if it is first non-white space character,
 * -1 otherwise.
*/

int seescope
(  char* p
,  int   len
)
   {  char* o = p
   ;  char* z = p + len
   ;  while(isspace((uchar) *p) && p < z)
      ++p
   ;  if (*p == '{')
      return (p-o)
   ;  return -1
;  }


/*
 * returns length of key found, -1 if error occurred.
*/

int checkusrsig
(  char* p
,  int   len
,  int*  kp
,  int*  namelen_p
,  mcxbits* keybits_p
)
   {  int   namelen  =  -1
   ;  int   taglen   =  -1

   ;  namelen = checkusrname(p, len, keybits_p)
   ;  *namelen_p = namelen

   ;  if (namelen <= 0)
      return -1
   ;  else if (namelen == len || *(p+namelen) != '#')    /* a simple key */
      {  if (kp)
         *kp = 0
      ;  return namelen
   ;  }

      taglen = checkusrtag(p+namelen, len-namelen, kp)

   ;  if (taglen < 0)
      return -1

   ;  return (namelen + taglen)
;  }


/*
 * returns length of tag found, -1 if error occurred.
*/

int checkusrtag
(  char* p
,  int   len
,  int*  kp
)
   {  char*  o = p         /* offset */
   ;  uchar p0 = p[0]
   ;  uchar p1 = len ? p[1] : 0
   ;  char*  z = p + len
   ;  int    k = 0

   ;  if (p0 != '#' || !isdigit(p1) || p1 == '0')
      return -1

   ;  while (++p < z && isdigit((uchar) *p))
      {  k *= 10
      ;  k += (uchar) *p - '0'
   ;  }

      if (kp)
      *kp = k

   ;  return (p-o)
;  }


/*
 * returns length of block found, -1 if error occurred.
*/

int checkblock
(  mcxTing* txt
,  int offset
)
   {  int cc = yamClosingCurly(txt, offset, NULL, RETURN_ON_FAIL)
   ;  return cc > 0 ? cc + 1 : -1
;  }


/*
 *  fixme this is used for let callback.
 *  xml shorthand is accepted as well (which is odd).
 *  simple additional check will remedy this.
 *  anon routines work.
*/

int checkusrcall
(  mcxTing* txt
,  int offset
)
   {  char* a = txt->str+offset
   ;  char* p = a
   ;  char* z = txt->str+txt->len
   ;  mcxbits keybits = 0
   ;  int len

   ;  if (*p++ != '\\')
      return -1

   ;  if ((len = checkusrname(p, z-p, &keybits)) < 0)
      return -1
   ;  else
      p+=len

   ;  if (!strncmp(a, "\\_", 2) && *p == '#' && isdigit((uchar) *(p+1)))
      p+= 2

   ;  while (1)
      {  if (*p == '{')
         {  if ((len = checkblock(txt, p-txt->str)) < 0)
            return -1
         ;  else
            p+=len
      ;  }
         else
         break
   ;  }
      return p-a
;  }


/*
 * returns length of name found, < 0 if error occurred.
*/

int checkusrname
(  char* p
,  int   len
,  mcxbits*  keybits
)
/* if meta set length to 1 */
   {  char*  o = p         /* offset */
   ;  char*  z = p + len
   ;  uchar p0 = p[0]
   ;  uchar p1 = len ? p[1] : 0 

                  /* XML META DATA PRIME QUOTE DOLLAR (underscore) */
   ;  if (len <= 0 || (!isalpha(p0) && !strchr("<!%'\"$_", p0)))
      return -2

   ;  if (isalnum(p0) || p0 == '_' || p0 == '$')
      {  while (p<z && *++p && (isalnum((uchar) *p) || *p == '_'))
         /* BE HAPPY */
      ;  if (p0 == '_' && p-o == 1)
         *keybits |= KEY_ANON
      ;  else if (p0 == '$')
         *keybits |= KEY_DOLLAR
      ;  else
         *keybits |= KEY_WORD
      ;  return p-o
   ;  }

      else if (p0 == '"')
      {  *keybits |= KEY_QUOTE
      ;  while (++p<z && *p != '"')
         {  if (*p == '\\' || *p == '{' || *p == '}')
            return -2
      ;  }
         return *p == '"' ? p-o+1 : -2
   ;  }

      else if (p0 == '<')
      {  *keybits |= KEY_XML
      ;  while (++p<z && *p != '>')
         if (*p == '<')                /* nesting not allowed */
         return -2
      ;  return *p == '>' ? p-o+1 : -2
   ;  }

      else if (p0 == '%')
      {  *keybits |= KEY_DATA
      ;  while (p++<z && isalnum((uchar) *p))
         ;
      ;  return (p-o)
   ;  }

      else if (p0 == '\'')
      {  if (p1 == '\'')
         {  uchar p2 = p[2]
         ;  *keybits |= KEY_GLOBAL
         ;  if (isalpha(p2) || p2 == '"' || p2 == '_')
            return 2 + checkusrname(p+2, len-2, keybits)
      ;  }
         else
         {  *keybits |= KEY_PRIME
         ;  if (isalpha(p1) || p1 == '"' || p1 == '$' || p1 == '_')
            return 1 + checkusrname(p+1, len-1, keybits)
         ;  return -2
      ;  }
      }

      else if (p0 == '!')
      {  *keybits |= KEY_META
      ;  while (*++p == '!')
         ;
         return p-o
   ;  }
      return -2
;  }



/* Sets seg offset to slash introducing keyword if present,
 * leaves it alone otherwise.
*/

int yamFindKey
(  yamSeg    *seg
)
   {  mcxTing*  txt  =  seg->txt
   ;  int   offset   =  seg->offset
   ;  int   x

   ;  char*    a     =  txt->str
   ;  char*    o     =  a + offset
   ;  char*    p     =  a + offset
   ;  char*    z     =  a + txt->len

   ;  int   esc      =  p && (*p == '\\')
   ;  int   lc       =  p && (*p == '\n') ? 1 : 0   /* *p may be '\n' indeed */
   ;  int   found    =  0
   ;  const char* me =  "yamFindKey"

   ;  if (seg->flags & SEGMENT_CONSTANT)
      return -1

   ;  while (++p < z)
      {
         if (*p == '\n')
         lc++

      ;  if (esc)                           /* a backslash, that is */
         {
            if
            (  isalpha((uchar) *p)
            || *p == '$'
            || *p == '_'
            || *p == '"'
            || *p == '!'
            || *p == '<'
            || *p == '>'
            || *p == '%'
            || *p == '\''
            )
            {  found =  1
            ;  seg->offset     =  offset + (p-o-1)
            ;  break
         ;  }
            else
            switch(*p)
            {
               case '\\'
            :  case '}'
            :  case '{'
            :  case '~'             /* \~ encodes &nbsp;          */
            :  case '|'             /* \| encodes <br>            */
            :  case '-'             /* \- encodes &emdash;        */
            :  case ','             /* \, is atom separator       */
            :  case '\n'            /* newline is ignored         */
            :  case '+'             /* FIXME; hacklet; at filter level */
            :  {  esc   =  0
               ;  break
            ;  }
                                    /* NOTE: \& only allowed within '@' mode,
                                     * so no need to case it here
                                    */
               case '@'
            :  {  p++
               ;  if ((uchar) p[0] == 'e')
                  p++
               ;  if ((x=yamClosingCurly(txt,p-a, &lc, RETURN_ON_FAIL)) < 0)
                  {  sourceIncrLc(txt, lc)
                  ;  yamErr("yamFindKey", "error while skipping at scope")
                  ;  return -2
               ;  }
                  p    +=  x     /* now *p == '}', while() will skip it */
               ;  esc   =  0
               ;  break
            ;  }

               case '*'
            :  {  if ((x =  yamEOConstant(txt, p-a)) < 0)
                  {  yamErr(me, "format error in constant expression")
                  ;  return -2
               ;  }
                  p    +=  x     /* now *p == '*', while() will skip it */
               ;  esc   =  0
               ;  break
            ;  }

               default
            :  {  if (*p >= '0' && *p <= '9')
                  {  esc   =  0
                  ;  break
               ;  }
                  else
                  {  sourceIncrLc(txt, lc)
                  ;  yamErr(me, "illegal escape sequence <\\%c>", *p)
                  ;  return -2
               ;  }
               }
            }
      ;  }
         else if (*p == '\\')
         esc = 1
   ;  }

      sourceIncrLc(txt, lc)
   ;  return (found ? seg->offset : -1)
;  }



/* Expects offset to match a slash introducing a keyword.  Sets seg offset
 * beyond keyword + args.  sets n_args_g, and fills key_and_args_g.  padds
 * key with the number of arguments.
 *
 * I chose not to unify with yamParseScopes for various reasons.  -
 * parsescopes interface would need an extra skipspace boolean, it would
 * probably need magic n=0 behaviour, and it would be unclear how to do the
 * n>9 error handling. If you need to reconsider this, think before coding.
 *
 * Caller must have set *keybits_p to zero.
*/

mcxstatus  yamParseKey
(  yamSeg    *seg
,  mcxbits   *keybits_p
)
   {  int  offset       =  seg->offset
   ;  mcxTing* txt      =  seg->txt

   ;  char* o           =  txt->str + offset
   ;  char* z           =  txt->str + txt->len
   ;  char* p           =  o+1

   ;  int n_args        =  0
   ;  int n_anon        =  0
   ;  int lc            =  0
   ;  const char* me    =  "yamParseKey"
   ;  int namelen       =  checkusrname(p, z-o-1, keybits_p)
   ;  int keybits       =  *keybits_p

   ;  if (namelen < 1)
      {  yamErr(me, "invalid key")
      ;  return STATUS_FAIL
   ;  }

      p += namelen

   ;  if (keybits & KEY_ANON && *p == '#')
      {  if (*(p+1) < '1' || *(p+1) > '9' || *(p+2) != '{')
         {  yamErr(me, "Anonymous key not ok")
         ;  return STATUS_FAIL
      ;  }
         n_anon = (uchar) *(p+1) - '0'
      ;  p += 2
   ;  }

      if (keybits & KEY_XML)       /* below  hacks \> and \<{..} in */
      {  mcxTingWrite(key_g, "<>")
      ;  mcxTingNWrite(arg1_g, o+2, namelen >= 2 ? namelen-2 : 0)
      ;  lc += mcxStrCountChar(arg1_g->str, '\n', arg1_g->len)
      ;  n_args++
   ;  }
      else if (keybits & KEY_META)
      {  mcxTingWrite(key_g, "!")
      ;  mcxTingNWrite(arg1_g, o+1, namelen)
      ;  n_args++
   ;  }
      else if (keybits & KEY_DATA)
      {  mcxTingNWrite(key_g, o+1, namelen)
      ;  mcxTingWrite(arg1_g, "")         /* initialize; append below */
   ;  }
      else if (keybits & KEY_PRIME)
      mcxTingNWrite(key_g,o+2, namelen-1)
   ;  else if (keybits & KEY_GLOBAL)
      mcxTingNWrite(key_g,o+3, namelen-2)
   ;  else
      mcxTingNWrite(key_g, o+1, namelen)

   ;  while (p<z && *p == '{')
      {  int c = yamClosingCurly(txt, offset + p-o, &lc, RETURN_ON_FAIL)
      ;  if (c < 0)
         {  yamErr(me, "no closing scope")
         ;  return STATUS_FAIL
      ;  }

         if (++n_args > 9)
         {  yamErr(me, "too many arguments for key %s\n", key_g->str)
         ;  return STATUS_FAIL
      ;  }

         if (keybits & KEY_DATA)
         mcxTingNAppend(arg1_g, p, c+1)  /* do write curlies */
      ;  else
         mcxTingNWrite((key_and_args_g+n_args),p+1, c-1)

      ;  p =  p+c+1     /* position beyond closing curly */

      ;  if (keybits & KEY_META)  /* so \!{foo}{bar} only parses  \!{foo} */
         break
   ;  }

      sourceIncrLc(txt, lc)
   ;  lc = 0

   ;  if (n_anon && n_anon + 1 != n_args)
      {  yamErr
         (  me
         ,  "found anon _#%d{%s} with %d arguments"
         ,  n_anon
         ,  arg1_g->str
         ,  n_args - 1
         )
      ;  return STATUS_FAIL
   ;  }

      else if (keybits & KEY_DATA)
      mcxTingAppend(key_g, "#1")

   ;  else if (n_args)
      mcxTingAppend(key_g, arg_padding_g[n_args])

   ;  n_args_g          =  n_args
   ;  seg->offset       =  offset + (p-o)

   ;  return 0
;  }



yamSeg*  yamDoKey
(  yamSeg *seg
)
   {  mcxbits keybits = 0
   ;  if (yamParseKey(seg, &keybits))
      return NULL
   ;  return yamExpandKey(seg, keybits)
;  }


int yamCountScopes
(  mcxTing* txt
,  int offset      
)
   {  char* p           =  txt->str+offset
   ;  char* o           =  p
   ;  char* z           =  txt->str+txt->len

   ;  char* q           =  mcxStrChrAint(o, isspace, z-o)
   ;  int   count       =  0

   ;  if (q && '{' == (uchar) *q)
      while(1)
      {  int cc   /* closing curly */
      ;  if (!(p = mcxStrChrAint(p, isspace, z-p)))
         p = z
      ;  if (p==z || *p != '{')
         break
      ;  if ((cc=yamClosingCurly(txt, offset + p-o, NULL, RETURN_ON_FAIL)) < 0)
         {  yamErr("yamCountScopes", "cannot close scope")
         ;  return -2
      ;  }
         count++
      ;  p +=  cc + 1
   ;  }
      else
      p = z

   ;  if (q && !count)
      count = -1

   ;  if (p != z)
      {  /*  yamErr("yamCountScopes", "found stuff trailing") */
      ;  return -2
   ;  }

      return count
;  }


/*
*/

char* yamParseKV
(  yamSeg*  segkv
)
   {  yamSeg* segops
   ;  mcxTing* val
   ;  char* key
   ;  int x, y

   ;  x = yamParseScopes(segkv, 2, 0)
   ;  n_args_g = 0

   ;  if (x <= 0)
      return NULL

   ;  key = mcxStrDup(arg1_g->str)
   ;  if (x == 1)
      return key           /* caller should check n_args_g */

   ;  val = mcxTingNNew(arg2_g->str, arg2_g->len)

   ;  segops = yamStackPushTmp(val)
   ;  y = yamParseScopes(segops, YAM_ARG_MAX, 0)

   ;  if (!y || y < 0)
      n_args_g = 0

   ;  yamStackFreeTmp(&segops)
   ;  mcxTingFree(&val)

   ;  return key
;  }


/*
 *    returns number of scopes found, possibly less than n.
*/

int yamParseScopes
(  yamSeg*  seg
,  int      n
,  int      delta
)
   {  int   offset      =  seg->offset
   ;  mcxTing* txt      =  seg->txt
   ;  char* o           =  txt->str + offset
   ;  char* p           =  o
   ;  char* z           =  txt->str + txt->len
   ;  char* q           =  mcxStrChrAint(o, isspace, z-o)
   ;  int   count       =  0

   ;  if (delta + n > YAM_ARG_MAX)
      yamErrx(1, "yamParseScopes PBD", "not that many (%d) scopes allowed", n)

   ;  if (tracing_g & (ZOEM_TRACE_SCOPES))
      printf(" sco| parsing up to %d scopes\n", n)

   ;  if (q && '{' == (uchar) *q)
      while(count < n)
      {  int cc   /* closing curly */
      ;  if (!(p = mcxStrChrAint(p, isspace, z-p)))
         p = z
      ;  if (p==z || *p != '{')
         break

      ;  if ((cc=yamClosingCurly(txt, offset + p-o, NULL, RETURN_ON_FAIL)) < 0)
         {  yamErr("yamParseScopes", "cannot close scope")
         ;  return -1                   /* mq: seg->offset ok? */
      ;  }

         mcxTingNWrite(key_and_args_g+(delta + ++count), p+1, cc-1)
      ;  p +=  cc + 1

      ;  if (tracing_g & (ZOEM_TRACE_SCOPES))
         traceput("sco", key_and_args_g+delta+count)
   ;  }
      else if (q)
      {  mcxTingNWrite(key_and_args_g+(delta + ++count), o, z-o)
      ;  p = z
      ;  if (tracing_g & (ZOEM_TRACE_SCOPES))
         traceput("sco", key_and_args_g+delta+count)
   ;  }

      if (tracing_g & (ZOEM_TRACE_SCOPES))
      printf(" sco| found %d scopes\n", count)

   /* caller should fill key_g */
   ;  n_args_g       =  count + delta
   ;  seg->offset    =  offset + p-o
   ;  return count
;  }


/*
 *  Is called after yamParseKey in yamDoKey. yamParseKey fills key_and_args_g
 *  and updates seg->offset.
 *  Pops a new seg onto the stack.
*/

yamSeg*  yamExpandKey
(  yamSeg *seg
,  mcxbits keybits
)
   {  const char* composite = NULL
   ;  mcxTing* user
      =     keybits & KEY_ANON
         ?  arg1_g
         :     keybits & KEY_PRIME
            ?  NULL
            :     keybits & KEY_GLOBAL
               ?  yamKeyGetGlobal(key_g)
               :  yamKeyGet(key_g)

   ;  xpnfnc yamop = NULL

   ;  if (tracing_g)
      {  int i
      ;  tracect_g++

      ;  if (tracing_g & ZOEM_TRACE_KEYS)
         printf
         (  "%s|%-32s seg %2d stack %2d nr %6d\n"
         ,  user ? "#  #" : " ## "
         ,  key_g->str
         ,  seg->idx
         ,  yamStackIdx()
         ,  tracect_g
         )

      ;  if (user && (tracing_g & (ZOEM_TRACE_DEFS)))
         traceput("def", user)

      ;  if (tracing_g & (ZOEM_TRACE_ARGS))
         {  for (i=1;i<=n_args_g;i++)
            traceput("arg", key_and_args_g+i)
      ;  }
   ;  }

      if (user)
      return expandUser(seg, user, keybits & KEY_ANON)

   ;  if (!(keybits & KEY_GLOBAL))
      yamop = yamOpGet(key_g, &composite)

                     /* docme wth is this */
   ;  if
      (  tracing_g & ZOEM_TRACE_PRIME
      && !(keybits & KEY_PRIME)
      && !strchr("<>!%", (uchar) key_g->str[0])
      )
      traceput("zum", key_g)

   ;  if (yamop)
      return yamop(seg)

                     /* fixme: initialize tings somewhere */
   ;  else if (composite)
      {  mcxTing* comp = mcxTingNew(composite)
      ;  yamSeg* newseg =  expandUser(seg, comp, FALSE)
      ;  mcxTingFree(&comp)
      ;  return newseg
   ;  }

      else
      yamErr
      (  "expand"
      ,  "no definition found for %s <%s>"
      ,  keybits & KEY_PRIME ? "primitive" : "key"
      ,  key_g->str
      )

   ;  return NULL
;  }


void mod_parse_exit
(  void
)
   {  int i

   ;  for(i=0;i<YAM_ARG_MAX+1;i++)
      mcxTingRelease(key_and_args_g+i)
;  }


void mod_parse_init
(  int   traceflags
)
   {  int i

   ;  for(i=0;i<YAM_ARG_MAX+1;i++)
         mcxTingInit(key_and_args_g+i)
      ,  mcxTingWrite(key_and_args_g+i, "_at_start_")

   ;  tracing_g = traceflags
;  }


int yamTracingSet
(  int   traceflags
)
   {  int prev       =  tracing_g

   ;  if (traceflags == -2)
      tracing_g = ZOEM_TRACE_ALL_LONG
   ;  else if (traceflags == -1)
      tracing_g = ZOEM_TRACE_ALL
   ;  else
      tracing_g = traceflags

   ;  return prev
;  }


/*
 *    '{'   for scopes
 *    '|'   for arguments
 *    '<'   for key defs
 *    '['   for segments
*/

void traceput
(  const char* c
,  const mcxTing* txt
)
   {  const char* s  =  txt->str
   ;  int l          =  txt->len
   ;  char* nl       =  strchr(s, '\n')
   ;  int i_nl       =  nl ? nl - s : l
   ;  int LEN        =  50
   ;  char* cont
   ;  int n

   ;  n = MCX_MAX(0, MCX_MIN(i_nl, MCX_MIN(l, LEN)))

   ;  cont =   nl && i_nl < LEN
               ?  "(\\n)"
               :  nl && i_nl >= LEN
                  ?  "(..\\n)"
                  :  n < l
                     ?  "(..)"
                     :  ""

   ;  if (tracing_g & (ZOEM_TRACE_LONG))
      {  printf("(%s(", c)
      ;  traceputlines(s, -1)
   ;  }
      else
      printf("(%s)%.*s%s [%d]\n", c, n, s, cont, l)
;  }


void traceputlines
(  const char* s
,  int len
)
   {  const char* z = len >= 0 ? s + len : NULL

   ;  while (*s && (!z || s < z))
      {  if (*s == '\n')
         printf("\n    |")
      ;  else
         putc(*s, stdout)
      ;  s++
   ;  }
      fputs("$\n)   )\n", stdout)
;  }


void yamScratchStats
(  void
)
   {  int i
   ;  fprintf(stderr, "key .......: <%lu>\n", (ulong) key_g->mxl)
   ;  for (i=1;i<10;i++)
      fprintf(stderr, "arg <%d> ...: <%lu>\n", i, (ulong) (key_and_args_g+i)->mxl)
;  }


yamSeg* yamFormatted1
(  yamSeg* seg
,  const char* arg
)
   {  mcxTing*  txt        =  mcxTingNew(arg)
   ;  char* o              =  txt->str
   ;  char* p              =  o
   ;  char* q              =  o
   ;  char* z              =  o + txt->len
   ;  mcxbool formatting   =  TRUE
   ;  mcxbool ok           =  TRUE

   ;  int   esc            =  0

   ;  while (p < z)
      {
         if (esc)
         {
            if ((uchar) p[0] == '@')          /* fixme uggggglyyyyyy */
            {  int l
            ;  if ((uchar) p[1] == 'e')
               p++
            ;  l = yamClosingCurly(txt, p+1-o, NULL, RETURN_ON_FAIL)
            ;  if (l<0)
               {  ok = FALSE
               ;  goto done
            ;  }
               *(q++)   =  '\\'
            ;  *(q++)   =  '@'
            ;  if ((uchar) p[0] == 'e')
               *(q++)   =  'e'
            ;  while (l-- && ++p)   /* write '{' upto-exclusive '}' */
               *(q++)   =  *p
            ;  *(q++)   =  *++p     /* writes the '}' */
         ;  }
            else if (*p == '<')     /* fixme UGGGGGLYYYYYY */
            {  int l    =  yamClosingCube(txt, p-o, NULL, RETURN_ON_FAIL)
            ;  if (l<0)
               {  ok = FALSE
               ;  goto done
            ;  }
               *(q++)   =  '\\'
            ;  *(q++)   =  '<'
            ;  while (l-- && ++p)   /* write after '<' upto-inclusive '>' */
               *(q++)   =  *p
         ;  }
            else if (*p == '`')     /* ugly branch, still supporting old syntax */
            {  const char* pp = p+1
            ;  int x = 0
            ;  if ((uchar) pp[0] == '{')
               x = yamClosingCurly(txt,pp-txt->str, NULL, RETURN_ON_FAIL)
            ;  if (x < 0)
               {  ok = FALSE
               ;  goto done
            ;  }
               else if (x > 0)
               p = p+1

            ;  while (1)
               {  ++p
               ;  if
                  (  (x > 0 && p-pp >= x)
                  || p >= z
                  || (x == 0 && *(p) == '`')
                  )
                  break

               ;  switch(*p)
                  {  case 's' : case ' '  :  *(q++) = ' '   ;  break
                  ;  case 'n' : case '\n' :  *(q++) = '\n'  ;  break
                  ;  case 't' : case '\t' :  *(q++) = '\t'  ;  break
                  ;  case '[' : case '<'  :  formatting = FALSE    ;  break
                  ;  case ']' : case '>'  :  formatting = TRUE     ;  break
                  ;  default  :
                        yamErr("\\formatted1", "illegal character <%c>", *p)
                     ;  ok = FALSE
                     ;  goto done
               ;  }
               }

               if (x == 0)
               {  static long n_bq_deprecated = 0
               ;  if (p == z)
                  {  yamErr("\\formatted1", "missing close symbol")
                  ;  goto done
               ;  }
                  if (!n_bq_deprecated++)
                  yamErr
                  (  "zoem"
                  ,  "deprecated: \\`%.*s`, use \\`{%.*s} instead"
                  ,  (int) (p-pp)
                  ,  pp
                  ,  (int) (p-pp)
                  ,  pp
                  )
            ;  }
            }
            else if (*p == ':')
            do { p++; } while (p<z && *p != '\n')
         ;  else
            {  *(q++) = '\\'
            ;  *(q++) = *p
         ;  }

            esc   =  0
      ;  }
         else if (formatting && isspace((uchar) *p))
      ;  else if (*p == '\\')
         esc = 1
      ;  else
         *(q++) = *p

      ;  p++
   ;  }

 done :

      seg_check_ok(ok, seg)
   ;  *q = '\0'
   ;  txt->len = q-o
   ;  return yamSegPush(seg, txt)
;  }


