/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

/* TODO
 *    inspect#4 is bugly and uggy.
*/

#include "ops.h"

#include <ctype.h>
#include <stdlib.h>
#include <regex.h>
#include <limits.h>
#include <float.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>

#include "ops-xtag.h"
#include "ops-grape.h"
#include "ops-constant.h"
#include "ops-env.h"
#include "op-inspect.h"
#include "op-format.h"

#include "util.h"
#include "digest.h"
#include "key.h"
#include "filter.h"
#include "sink.h"
#include "source.h"
#include "segment.h"
#include "parse.h"
#include "read.h"
#include "curly.h"
#include "iface.h"

#include "tingea/ting.h"
#include "tingea/ding.h"
#include "tingea/hash.h"
#include "tingea/io.h"
#include "tingea/minmax.h"
#include "tingea/rand.h"
#include "tingea/err.h"
#include "tingea/types.h"
#include "tingea/tr.h"
#include "tingea/let.h"
#include "tingea/minmax.h"
#include "tingea/compile.h"

static  double precision_g     =  1e-8;

long roundnear (double f) { return (long) floor(f + 0.5) ; }


#define arg(ting)   mcxTingNNew(ting->str, ting->len)


#define I_BANG_1     "{any}"
#define J_BANG_1     "strip curlies, put any"

#define I_BANG_0     ""
#define J_BANG_0     "make slash"

#define I_XTAG_1     "{_any_}"
#define J_XTAG_1     "xml tag sugar"

#define I_XTAG_2     "{_any_}{_any_}"
#define J_XTAG_2     "xml tag+content sugar"

#define I_DEF_2      "{ks}{any}"
#define J_DEF_2      "define key; complain if key exists"

#define I_FORMAT_2   "{syn}{_va_}"
#define J_FORMAT_2   "format varargs according to format syntax"

#define I_DEFX_2     "{ks}{_any_}"
#define J_DEFX_2     "define key; expand before storing"

#define I_LET_1      "{algebra}"
#define J_LET_1      "evaluate arithmetic expression"

#define I_VANISH_1   "{any}"
#define J_VANISH_1   "process any for side effects only"

#define I_TRY_1      "{any}"
#define J_TRY_1      "write status/result in __zoemstat__/__zoemput__"

#define I_SET_2      "{ks}{any}"
#define J_SET_2      "do not complain if key exists"

#define I_SET_3      "{{modes}{acgltuvwx}}{ks}{any}"
#define J_SET_3      "set key with access to all modes"

#define I_SETX_2      "{ks}{_any_}"
#define J_SETX_2      "expand definition before storing"

#define I_THROW_2     "{error|towel}{_any_}"
#define J_THROW_2     "throw error/towell, print msg _any_"

#define I_REGISTER_2  "{tag}{any}"
#define J_REGISTER_2  "register any with tag (e.g. END)"

#define I_TEXTMAP_2   "{va}{_any_}"
#define J_TEXTMAP_2   "apply va-encoded map to any"

#define I_SEQALONG_3  "{key}{_ks|ak_}{_va_}"
#define J_SEQALONG_3  "like apply, increment key"

#define I_APPLY_2     "{_ks|ak_}{_va_}"
#define J_APPLY_2     "apply (possibly anon) key ex to vararg"

#define I_LENGTH_1     "{_any_}"
#define J_LENGTH_1     "evaluate and put length in place"

#define I_WHILST_2   "{_int_}{_any_}"
#define J_WHILST_2   "eval and output any while int nonzero"

#define I_WHILE_2     "{_int_}{_any_}"
#define J_WHILE_2     "eval any while int nonzero"

#define I_DOWHILE_2   "{_any_}{_int_}"
#define J_DOWHILE_2   "eval any until int nonzero"

#define I_TABLE_5     "{_int_}{lft}{sep}{rgt}{_va_}"
#define J_TABLE_5     "create array-like structure"

#define I_SYSTEM_3    "{cmd}{_va_}{_any_}"
#define J_SYSTEM_3    "pipe any through cmd with arguments va"

#define I_ENV_4       "{lbl}{any}{any}{any}"
#define J_ENV_4       "associate strings with env lbl"

#define I_BEGIN_2     "{lbl}{va}"
#define J_BEGIN_2     "begin env with args"

#define I_END_1       "{lbl}"
#define J_END_1       "end env"

#define I_TRACE_1     "{_int_}"
#define J_TRACE_1     "set trace flags"

#define I_WRITE_3     "{_fn_}{nm}{_any_}"
#define J_WRITE_3     "write any via filter nm (copy|txt|device) to file"

#define I_DOFILE_2    "{_fn_}{chr(!?)chr(+-)}"
#define J_DOFILE_2    "read file with input/existence modes"

#define I_PROTECT_1  "{_any_}"
#define J_PROTECT_1  "escape contents"

#define I_FINSERT_1  "{_fn_}"
#define J_FINSERT_1  "put escaped contents in place"

#define I_NARGS_1    "{any}"
#define J_NARGS_1    "count number of scopes in any"

#define I_ZINSERT_1  "{_fn_}"
#define J_ZINSERT_1  "put unmodified contents in place"

#define I_INSPECT_4   "{_va_}{_reg_}{_any|ak_}{_any_}"
#define J_INSPECT_4   "apply regex to any"

#define I_TR_2       "{chr(cds)*{list}{list}}{_any_}"
#define J_TR_2       "translate chars in any"

#define I_LINE_0     ""
#define J_LINE_0     "put current line number in current file"

#define I_REDIRECT_1 "{_fn_}"
#define J_REDIRECT_1 "change default output file"

#define I_QUIT_0     ""
#define J_QUIT_0     "quit parsing current stack in current file"

#define I_ERROR_1     "_any_"
#define J_ERROR_1     "expand any, print error message, raise error"

#define I_EVAL_1     "_any_"
#define J_EVAL_1     "expand any, pass it on for further expansion"

#define I_EXIT_0     ""
#define J_EXIT_0     "out through the door without opening it"

#define I_PUSH_1     "{tag|$tag}"
#define J_PUSH_1     "push a new user/dollar dictionary named tag or $tag"

#define I_GET_2      "{dict-name}{ks}"
#define J_GET_2      "retrieve key from named dictionary"

#define I_POP_1      "{tag|$tag}"
#define J_POP_1      "pop the top dictionary; tag|$tag must be name"

#define I_FV_2       "{nm}{_va_}"
#define J_FV_2       "apply operator nm to ops"

#define I_F_3        "{nm}{_num_}{_num_}"
#define J_F_3        "apply operator nm to ops"

#define I_F_2        "{nm}{_num_}"
#define J_F_2        "apply operator nm to op"

#define I_CMP_3      "{nm}{_any_}{_any_}"
#define J_CMP_3      "apply nm (lt|lq|eq|gq|gt|ne|cp) to ops; put int"

#define I_EQT_3      "{nm}{_num_}{_num_}"
#define J_EQT_3      "apply nm (lt|lq|eq|gq|gt|ne|cp) to ops; put int"

#define I_IF_3       "{_int_}{any}{any}"
#define J_IF_3       "if int any else any"

#define I_DEFINED_2  "{nm}{_any_}"
#define J_DEFINED_2  "test for (key|lkey|data|primitive|builtin|zoem|ENV)"

#define I_UNDEF_1    "{ks}"
#define J_UNDEF_1    "delete key"

#define I_SWITCH_2   "{_any_}{va}"
#define J_SWITCH_2   "use pivot to select from va"

#define I_BRANCH_1   "{va}"
#define J_BRANCH_1   "va contains condition/branch pairs"

#define I_DSET_2     "{_any|va_}{any|va}"
#define J_DSET_2     "access sequence, value or values"

#define I_DSETX_2    "{_any|va_}{_any|va_}"
#define J_DSETX_2    "access sequence, value or values"

#define I_GRAPE1_1     "{_any|va_}"
#define J_GRAPE1_1     "access sequence for getting"

#define I_GRAPE2_1     "{_any|va_}"
#define J_GRAPE2_1     "access sequence for freeing"

#define I_GRAPE3_1     "{_any|va_}"
#define J_GRAPE3_1     "access sequence for dumping"

#define I_DFREE_1    "{_any|va_}"
#define J_DFREE_1    "access sequence"

#define I_DPRINT_1   "{_any|va_}"
#define J_DPRINT_1   "access sequence"

#define I_SPECIAL_1  "{va}"
#define J_SPECIAL_1  "pairs of ascii num/mapping"

#define I_CONSTANT_1 "{va}"
#define J_CONSTANT_1 "pairs of label/mapping"

#define I_CATCH_2   "{towel|error}{any}"
#define J_CATCH_2   "catch exception=towel and/or error"

#define I_DOLLAR_2   "{str}{any}"
#define J_DOLLAR_2   "put any if \\__device__==str"

#define I_FORMATTED_1 "{any}"
#define J_FORMATTED_1 "remove ws in any, translate \\%nst<>%"


#define F_DEVICE     "device filter (customize with \\special#1)"
#define F_TXT        "interprets [\\\\][\\~][\\,][\\|][\\}][\\{]"
#define F_COPY       "identity filter (literal copy)"


const char *strLegend[]
=  {
""
"L e g e n d",
"Ab   Meaning            Examples/explanation",
"--| |----------------| |---------------------------------------------------|",
"ak   anonymous key      e.g. <_#2{foo{\\1}\\bar{\\1}{\\2}}>",
"any  anything           anything",
"chr  character          presumably a switch of some kind",
"fn   file name          a string that will be used as name of file",
"int  integer            e.g. '123', '-6', no arithmetic expressions allowed",
"ks   key signature      e.g. <foo#2> or <\"+\"#2> (without the <>)",
"lbl  label              names for constants, env",
"nm   name               name; one of a fixed set",
"str  string             presumably a label of some kind",
"syn  syntax             argument satisfies special syntax",
"va   vararg             of the form {any1} {any2} {any3}",
"",
"[]-enclosed stuff is optional",
"()-enclosed stuff denotes a choice between alternatives",
"* denotes zero or more occurrences",
"_-enclosed arguments are first expanded by the primitive.",
"So, _int_ denotes an arbitrary expression that should evaluate to an integer",
NULL
   }
;

static   mcxHash*    yamTable_g        =  NULL;    /* primitives        */
static   mcxTing*    devtxt_g          =  NULL;    /* "__device__"    */

static   mcxLink*    reg_end_g         =  NULL;


typedef struct
{  const char*       name
;  const char*       tag
;  const char*       descr
;  yamSeg*           (*yamfunc)(yamSeg* seg)    /* either this */
;  const char*       composite                  /* or that but not both */
;
}  cmdHook           ;


yamSeg* expandBang1     (  yamSeg*  seg)  ;
yamSeg* expandBang2     (  yamSeg*  seg)  ;
yamSeg* expandXtag1     (  yamSeg*  seg)  ;
yamSeg* expandXtag2     (  yamSeg*  seg)  ;
yamSeg* expandDollar2   (  yamSeg*  seg)  ;
yamSeg* expandGrapeDump (  yamSeg*  seg)  ;
yamSeg* expandGrapeFree (  yamSeg*  seg)  ;
yamSeg* expandGrapeGet  (  yamSeg*  seg)  ;
yamSeg* expandApply2    (  yamSeg*  seg)  ;
yamSeg* expandBegin2    (  yamSeg*  seg)  ;
yamSeg* expandBranch1   (  yamSeg*  seg)  ;
yamSeg* expandCatch2    (  yamSeg*  seg)  ;
yamSeg* expandCmp3      (  yamSeg*  seg)  ;
yamSeg* expandConstant1 (  yamSeg*  seg)  ;
yamSeg* expandDef2      (  yamSeg*  seg)  ;
yamSeg* expandDefined2  (  yamSeg*  seg)  ;
yamSeg* expandDefx2     (  yamSeg*  seg)  ;
yamSeg* expandDofile2   (  yamSeg*  seg)  ;
yamSeg* expandDowhile2  (  yamSeg*  seg)  ;
yamSeg* expandDprint1   (  yamSeg*  seg)  ;
yamSeg* expandEnd1      (  yamSeg*  seg)  ;
yamSeg* expandEnv4      (  yamSeg*  seg)  ;
yamSeg* expandEqt3      (  yamSeg*  seg)  ;
yamSeg* expandEval1     (  yamSeg*  seg)  ;
yamSeg* expandExit0     (  yamSeg*  seg)  ;
yamSeg* expandF2        (  yamSeg*  seg)  ;
yamSeg* expandF3        (  yamSeg*  seg)  ;
yamSeg* expandFinsert1  (  yamSeg*  seg)  ;
yamSeg* expandFormat2   (  yamSeg*  seg)  ;
yamSeg* expandFormatted1(  yamSeg*  seg)  ;
yamSeg* expandFv2       (  yamSeg*  seg)  ;
yamSeg* expandGet2      (  yamSeg*  seg)  ;
yamSeg* expandIf3       (  yamSeg*  seg)  ;
yamSeg* expandInspect4  (  yamSeg*  seg)  ;
yamSeg* expandJumpLc1   (  yamSeg*  seg)  ;
yamSeg* expandLength1   (  yamSeg*  seg)  ;
yamSeg* expandLet1      (  yamSeg*  seg)  ;
yamSeg* expandLine      (  yamSeg*  seg)  ;
yamSeg* expandSHenv     (  yamSeg*  seg)  ;
yamSeg* expandNargs1    (  yamSeg*  seg)  ;
yamSeg* expandPop1      (  yamSeg*  seg)  ;
yamSeg* expandProtect1  (  yamSeg*  seg)  ;
yamSeg* expandPush1     (  yamSeg*  seg)  ;
yamSeg* expandWriteto1  (  yamSeg*  seg)  ;
yamSeg* expandRegister2 (  yamSeg*  seg)  ;
yamSeg* expandSeqalong3 (  yamSeg*  seg)  ;
yamSeg* expandSet2      (  yamSeg*  seg)  ;
yamSeg* expandSet3      (  yamSeg*  seg)  ;
yamSeg* expandSetx2     (  yamSeg*  seg)  ;
yamSeg* expandSpecial1  (  yamSeg*  seg)  ;
yamSeg* expandSwitch2   (  yamSeg*  seg)  ;
yamSeg* expandSystem3   (  yamSeg*  seg)  ;
yamSeg* expandTable5    (  yamSeg*  seg)  ;
yamSeg* expandTest3     (  yamSeg*  seg)  ;
yamSeg* expandTestFile2 (  yamSeg*  seg)  ;
yamSeg* expandTextmap2  (  yamSeg*  seg)  ;
yamSeg* expandThrow2    (  yamSeg*  seg)  ;
yamSeg* expandTr2       (  yamSeg*  seg)  ;
yamSeg* expandTrace1    (  yamSeg*  seg)  ;
yamSeg* expandTry1      (  yamSeg*  seg)  ;
yamSeg* expandUndef1    (  yamSeg*  seg)  ;
yamSeg* expandVanish1   (  yamSeg*  seg)  ;
yamSeg* expandWhile2    (  yamSeg*  seg)  ;
yamSeg* expandWhilst2   (  yamSeg*  seg)  ;
yamSeg* expandWrite3    (  yamSeg*  seg)  ;
yamSeg* expandZinsert1  (  yamSeg*  seg)  ;

static  cmdHook  cmdHookDir[] =
{  {  "!#1"        ,  I_BANG_0        ,  J_BANG_0        ,  expandBang1       ,  NULL  }
,  {  "!#2"        ,  I_BANG_1        ,  J_BANG_1        ,  expandBang2       ,  NULL  }
,  {  "$#2"        ,  I_DOLLAR_2      ,  J_DOLLAR_2      ,  expandDollar2     ,  NULL  }
,  {  "%#1"        ,  I_GRAPE1_1      ,  J_GRAPE1_1      ,  expandGrapeGet    ,  NULL  }
,  {  "%free#1"    ,  I_GRAPE2_1      ,  J_GRAPE2_1      ,  expandGrapeFree   ,  NULL  }
,  {  "%dump#1"    ,  I_GRAPE3_1      ,  J_GRAPE3_1      ,  expandGrapeDump   ,  NULL  }
,  {  "<>#1"       ,  I_XTAG_1        ,  J_XTAG_1        ,  expandXtag1       ,  NULL  }
,  {  "<>#2"       ,  I_XTAG_2        ,  J_XTAG_2        ,  expandXtag2       ,  NULL  }
,  {  "apply#2"    ,  I_APPLY_2       ,  J_APPLY_2       ,  expandApply2      ,  NULL  }
,  {  "begin#2"    ,  I_BEGIN_2       ,  J_BEGIN_2       ,  expandBegin2      ,  NULL  }
,  {  "branch#1"   ,  I_BRANCH_1      ,  J_BRANCH_1      ,  expandBranch1     ,  NULL  }
,  {  "catch#2"    ,  I_CATCH_2       ,  J_CATCH_2       ,  expandCatch2      ,  NULL  }
,  {  "cmp#3"      ,  I_CMP_3         ,  J_CMP_3         ,  expandCmp3        ,  NULL  }
,  {  "constant#1" ,  I_CONSTANT_1    ,  J_CONSTANT_1    ,  expandConstant1   ,  NULL  }
,  {  "def#2"      ,  I_DEF_2         ,  J_DEF_2         ,  expandDef2        ,  NULL  }
,  {  "defined#2"  ,  I_DEFINED_2     ,  J_DEFINED_2     ,  expandDefined2    ,  NULL  }
,  {  "defx#2"     ,  I_DEFX_2        ,  J_DEFX_2        ,  expandDefx2       ,  NULL  }
,  {  "dofile#2"   ,  I_DOFILE_2      ,  J_DOFILE_2      ,  expandDofile2     ,  NULL  }
,  {  "dowhile#2"  ,  I_DOWHILE_2     ,  J_DOWHILE_2     ,  expandDowhile2    ,  NULL  }
,  {  "end#1"      ,  I_END_1         ,  J_END_1         ,  expandEnd1        ,  NULL  }
,  {  "env#4"      ,  I_ENV_4         ,  J_ENV_4         ,  expandEnv4        ,  NULL  }
,  {  "eqt#3"      ,  I_EQT_3         ,  J_EQT_3         ,  expandEqt3        ,  NULL  }
,  {  "eval#1"     ,  I_EVAL_1        ,  J_EVAL_1        ,  expandEval1       ,  NULL  }
,  {  "exit"       ,  I_EXIT_0        ,  J_EXIT_0        ,  expandExit0       ,  NULL  }
,  {  "f#2"        ,  I_F_2           ,  J_F_2           ,  expandF2          ,  NULL  }
,  {  "f#3"        ,  I_F_3           ,  J_F_3           ,  expandF3          ,  NULL  }
,  {  "finsert#1"  ,  I_FINSERT_1     ,  J_FINSERT_1     ,  expandFinsert1    ,  NULL  }
,  {  "format#2"   ,  I_FORMAT_2      ,  J_FORMAT_2      ,  expandFormat2     ,  NULL  }
,  {  "formatted#1",  I_FORMATTED_1   ,  J_FORMATTED_1   ,  expandFormatted1  ,  NULL  }
,  {  "fv#2"       ,  I_FV_2          ,  J_FV_2          ,  expandFv2         ,  NULL  }
,  {  "get#2"      ,  I_GET_2         ,  J_GET_2         ,  expandGet2        ,  NULL  }
,  {  "if#3"       ,  I_IF_3          ,  J_IF_3          ,  expandIf3         ,  NULL  }
,  {  "inspect#4"  ,  I_INSPECT_4     ,  J_INSPECT_4     ,  expandInspect4    ,  NULL  }
,  {  "length#1"   ,  I_LENGTH_1      ,  J_LENGTH_1      ,  expandLength1     ,  NULL  }
,  {  "let#1"      ,  I_LET_1         ,  J_LET_1         ,  expandLet1        ,  NULL  }
,  {  "nargs#1"    ,  I_NARGS_1       ,  J_NARGS_1       ,  expandNargs1      ,  NULL  }
,  {  "pop#1"      ,  I_POP_1         ,  J_POP_1         ,  expandPop1        ,  NULL  }
,  {  "protect#1"  ,  I_PROTECT_1     ,  J_PROTECT_1     ,  expandProtect1    ,  NULL  }
,  {  "push#1"     ,  I_PUSH_1        ,  J_PUSH_1        ,  expandPush1       ,  NULL  }
,  {  "register#2" ,  I_REGISTER_2    ,  J_REGISTER_2    ,  expandRegister2   ,  NULL  }
,  {  "seqalong#3" ,  I_SEQALONG_3    ,  J_SEQALONG_3    ,  expandSeqalong3   ,  NULL  }
,  {  "set#2"      ,  I_SET_2         ,  J_SET_2         ,  expandSet2        ,  NULL  }
,  {  "set#3"      ,  I_SET_3         ,  J_SET_3         ,  expandSet3        ,  NULL  }
,  {  "setx#2"     ,  I_SETX_2        ,  J_SETX_2        ,  expandSetx2       ,  NULL  }
,  {  "special#1"  ,  I_SPECIAL_1     ,  J_SPECIAL_1     ,  expandSpecial1    ,  NULL  }
,  {  "switch#2"   ,  I_SWITCH_2      ,  J_SWITCH_2      ,  expandSwitch2     ,  NULL  }
,  {  "system#3"   ,  I_SYSTEM_3      ,  J_SYSTEM_3      ,  expandSystem3     ,  NULL  }
,  {  "table#5"    ,  I_TABLE_5       ,  J_TABLE_5       ,  expandTable5      ,  NULL  }
,  {  "textmap#2"  ,  I_TEXTMAP_2     ,  J_TEXTMAP_2     ,  expandTextmap2    ,  NULL  }
,  {  "throw#2"    ,  I_THROW_2       ,  J_THROW_2       ,  expandThrow2      ,  NULL  }
,  {  "tr#2"       ,  I_TR_2          ,  J_TR_2          ,  expandTr2         ,  NULL  }
,  {  "trace#1"    ,  I_TRACE_1       ,  J_TRACE_1       ,  expandTrace1      ,  NULL  }
,  {  "try#1"      ,  I_TRY_1         ,  J_TRY_1         ,  expandTry1        ,  NULL  }
,  {  "undef#1"    ,  I_UNDEF_1       ,  J_UNDEF_1       ,  expandUndef1      ,  NULL  }
,  {  "vanish#1"   ,  I_VANISH_1      ,  J_VANISH_1      ,  expandVanish1     ,  NULL  }
,  {  "while#2"    ,  I_WHILE_2       ,  J_WHILE_2       ,  expandWhile2      ,  NULL  }
,  {  "whilst#2"   ,  I_WHILST_2      ,  J_WHILST_2      ,  expandWhilst2     ,  NULL  }
,  {  "write#3"    ,  I_WRITE_3       ,  J_WRITE_3       ,  expandWrite3      ,  NULL  }
,  {  "writeto#1"  ,  I_REDIRECT_1    ,  J_REDIRECT_1    ,  expandWriteto1    ,  NULL  }
,  {  "zinsert#1"  ,  I_ZINSERT_1     ,  J_ZINSERT_1     ,  expandZinsert1    ,  NULL  }
,  {  "___jump_lc___#1", NULL         ,  NULL            ,  expandJumpLc1     ,  NULL  }
,  {  "__line__"   ,  NULL            ,  NULL            ,  expandLine        ,  NULL  }
,  {  "__env__#1"  ,  NULL            ,  NULL            ,  expandSHenv       ,  NULL  }
,  {  "__test__#3" ,  NULL            ,  NULL            ,  expandTest3       ,  NULL  }
,  {  "testfile#2" ,  NULL            ,  NULL            ,  expandTestFile2   ,  NULL  }

,  {  "done"       ,  NULL            ,  NULL            ,  NULL              ,  "\\'throw{done}" }
,  {  "ifdef#3"    ,  NULL            ,  NULL            ,  NULL              ,  "\\'if{\\defined{\\1}{\\2}}{\\3}{}" }
,  {  "ifdef#4"    ,  NULL            ,  NULL            ,  NULL              ,  "\\'if{\\defined{\\1}{\\2}}{\\3}{\\4}" }
,  {  "input#1"    ,  NULL            ,  NULL            ,  NULL              ,  "\\'dofile{\\1}{!+}" }
,  {  "import#1"   ,  NULL            ,  NULL            ,  NULL              ,  "\\'dofile{\\1}{!-}" }
,  {  "read#1"     ,  NULL            ,  NULL            ,  NULL              ,  "\\'dofile{\\1}{?+}" }
,  {  "load#1"     ,  NULL            ,  NULL            ,  NULL              ,  "\\'dofile{\\1}{?-}" }
,  {  "begin#1"    ,  NULL            ,  NULL            ,  NULL              ,  "\\'begin{\\1}{}"  }
,  {  "env#3"      ,  NULL            ,  NULL            ,  NULL              ,  "\\'env{\\1}{}{\\2}{\\3}"  }
,  {  "system#2"   ,  NULL            ,  NULL            ,  NULL              ,  "\\'system{\\1}{\\2}{}"  }
,  {  "system#1"   ,  NULL            ,  NULL            ,  NULL              ,  "\\'system{\\1}{}{}"  }
,  {  "throw#1"    ,  NULL            ,  NULL            ,  NULL              ,  "\\'throw{\\1}{}" }
,  {  "inform#1"   ,  NULL            ,  NULL            ,  NULL              ,  "\\'write{stderr}{device}{\\1\\@{\\N}}" }
,  {  "append#2"   ,  NULL            ,  NULL            ,  NULL              ,  "\\'set{{modes}{a}}{\\1}{\\2}" }
,  {  "appendx#2"  ,  NULL            ,  NULL            ,  NULL              ,  "\\'set{{modes}{ax}}{\\1}{\\2}" }
,  {  "seq#4"      ,  NULL            ,  NULL            ,  NULL              ,  "\\'set{\\1}{\\2}\\'while{\\'let{\\'get{''}{\\1}<\\3}}{\\4\\'setx{\\1}{\\'let{\\'get{''}{\\1}+1}}}" }
,  {  "update#2"   ,  NULL            ,  NULL            ,  NULL              ,  "\\'set{{modes}{e}}{\\1}{\\2}" }
,  {  "updatex#2"  ,  NULL            ,  NULL            ,  NULL              ,  "\\'set{{modes}{ex}}{\\1}{\\2}" }
,  {  "\"\""       ,  NULL            ,  NULL            ,  NULL              ,  "" }
,  {  "\"\"#1"     ,  NULL            ,  NULL            ,  NULL              ,  "" }
,  {  "group#1"    ,  NULL            ,  NULL            ,  NULL              ,  "\\1" }
,  {  "PI"         ,  NULL            ,  NULL            ,  NULL              ,  "3.1415926536"  }
,  {  "E"          ,  NULL            ,  NULL            ,  NULL              ,  "2.71828182846"  }
,  {  "$#1"        ,  NULL            ,  NULL            ,  NULL              ,  "\\'switch{\\__device__}{\\1}"  }
,  {  "$#3"        ,  NULL            ,  NULL            ,  NULL              ,  "\\'switch{\\__device__}{{\\1}{\\2}{\\3}}" }

,  {  NULL         ,  NULL            ,  NULL            ,  NULL              ,  NULL  }
}  ;


mcxstatus ask_user
(  const mcxTing* ask
,  const char* me
)
   {  mcxIO* prompt = mcxIOnew("-", "r")
   ;  mcxTing* ln = mcxTingEmpty(NULL, 10)
   ;  mcxstatus status = STATUS_FAIL

   ;  do
      {  if (!isatty(fileno(stdin)))
         {  yamErr(me, "unsafe mode; cannot prompt terminal!")
         ;  break
      ;  }

         if (mcxIOopen(prompt, RETURN_ON_FAIL))
         break

      ;  fputs(ask->str, stdout)
      ;  fflush(stdout)    /* mq fflush/fputs mix ? */

      ;  mcxIOreadLine(prompt, ln, MCX_READLINE_CHOMP)

      ;  if (ln && ln->len == 1)
         {  if (ln->str[0] == 'y' || ln->str[0] == 'Y')
           /* PARTY */
         ;  else if (ln->str[0] == 'n'  || ln->str[0] == 'N')
            break
         ;  else
            continue
      ;  }
         else
         fprintf(stderr, "[%s]\n", ln->str)
      ;  status = STATUS_OK
   ;  }
      while (0)

   ;  mcxTingFree(&ln)
   ;  mcxIOfree(&prompt)
   ;  return status
;  }


mcxstatus yamOpsDataAccess
(  const mcxTing*    access
)  ;

mcxbool  yamOpList
(  const char* mode
)
   {  cmdHook*    cmdhook     =  cmdHookDir
   ;  mcxbool     listAll     =  strstr(mode, "all") != NULL
   ;  mcxbool     match       =  listAll || 0

   ;  if (listAll || strstr(mode, "zoem"))
      {  while (cmdhook && cmdhook->name)
         {  if (cmdhook->descr)
            fprintf
            (  stdout
            ,  "%-11s %-20s %s\n"
            ,  cmdhook->name
            ,  !cmdhook->tag || !*(cmdhook->tag) ? "..." : cmdhook->tag
            ,  cmdhook->descr
            )
         ;  cmdhook++
      ;  }
         if (!strstr(mode, "legend"))
         fprintf(stdout, "Additionally supplying \"-l legend\" prints legend\n")
      ;  match = 1
   ;  }

      if (listAll || strstr(mode, "legend"))
      {  int m
      ;  for (m=0;strLegend[m];m++)
         fprintf(stdout, "%s\n", strLegend[m])
      ;  match = 1
   ;  }

      if (listAll || strstr(mode, "builtin"))
      {  fputs("\nBuilt-in aliases\n", stdout)
      ;  cmdhook = cmdHookDir
      ;  while (cmdhook && cmdhook->name)
         {  if (cmdhook->composite)
            fprintf
            (  stdout
            ,  "%-15s maps to   %s\n"
            ,  cmdhook->name
            ,  strlen(cmdhook->composite) ? cmdhook->composite : "   (nothing)"
            )
         ;  cmdhook++
      ;  }
         match = 1
   ;  }

      return match ? TRUE : FALSE
;  }


yamSeg* expandNargs1
(  yamSeg*  seg
)
   {  return yamSegPush(seg, mcxTingInteger(NULL, yamCountScopes(arg1_g, 0)))
;  }


yamSeg* expandSHenv
(  yamSeg*  seg
)
   {  const char* envp = getenv(arg1_g->str)
   ;  return yamSegPush(seg, mcxTingNew(envp ? envp : ""))
;  }


yamSeg* expandLine
(  yamSeg*  seg
)
   {  return yamSegPush(seg, mcxTingInteger(NULL, sourceGetLc()))
;  }


yamSeg* expandJumpLc1
(  yamSeg*  seg
)
   {  int ct = strtol(arg1_g->str, NULL, 10)
   ;  sourceIncrLc(NULL, ct)
   ;  return seg
;  }


yamSeg* expandThrow2
(  yamSeg*  seg
)
   {  int mode =     !strcmp(arg1_g->str, "towel")
                  ?  SEGMENT_THROW
                  :     !strcmp(arg1_g->str, "done")
                     ?  SEGMENT_DONE
                     :     !strcmp(arg1_g->str, "error")
                        ?  SEGMENT_ERROR
                        :  0

   ;  const char* type
               =     mode == SEGMENT_THROW
                  ?  "exception"
                  :     mode ==  SEGMENT_DONE
                     ?  "bailout"
                     :  "error"

   ;  mcxTing* msg = arg2_g->len ? arg(arg2_g) : NULL

   ;  if (!mode)
         yamErr
         (  "throw#2"
         ,  "<%s> is unthrowable, throwing error instead"
         ,  arg1_g->str
         )
      ,  mode = SEGMENT_ERROR

   ;  if (msg)
         yamDigest(msg, msg, NULL)
      ,  yamErr(NULL, "[%s :: %s]", type, msg->str)
      ,  mcxTingFree(&msg)

   ;  seg->flags |= mode
   ;  return yamSegPushEmpty(seg)
;  }


yamSeg* expandTestFile2
(  yamSeg*  seg
)
   {  struct stat mystat
   ;  return   yamSegPush
               (  seg
               ,  mcxTingNew(stat(arg2_g->str, &mystat) ? "0" : "1")
               )
;  }


yamSeg* expandTest3
(  yamSeg*  seg
)
   {  mcxTing* ops   =  arg(arg1_g)
   ;  yamSeg* segops =  yamStackPushTmp(ops)
   ;  while (yamParseScopes(segops, 1, 0))
      fprintf(stderr, "block: %s\n", arg1_g->str)
   ;  yamStackFreeTmp(&segops)
   ;  mcxTingFree(&ops)
   ;  return yamSegPushEmpty(seg)
;  }


yamSeg* expandCatch2
(  yamSeg*  seg
)
   {  mcxbits accept  =    !strcmp(arg1_g->str, "done")
                        ?  SEGMENT_DONE
                        :     !strcmp(arg1_g->str, "towel")
                           ?  SEGMENT_DONE | SEGMENT_THROW
                           :     !strcmp(arg1_g->str, "error")
                              ?  SEGMENT_DONE | SEGMENT_THROW | SEGMENT_ERROR
                              :  0
   ;  mcxTing* stuff

   ;  if (!accept)
      {  mcxErr("\\catch#2", "cannot catch <%s>", arg1_g->str)
      ;  seg_check_ok(FALSE, seg)
      ;  return yamSegPushEmpty(seg)
   ;  }

      stuff = arg(arg2_g)
   ;  yamDigest(stuff, stuff, seg)
   ;  yamKeySet
      (  "__zoemstat__"
      ,     seg->flags & SEGMENT_ERROR
         ?  "error"
         :     seg->flags & SEGMENT_THROW
            ?  "towel"
            :  "done"
      )

   ;  BIT_OFF(seg->flags, accept)
                     /* fixordoc: why is pushEmpty not needed? */ 
   ;  return yamSegPush(seg, stuff)
;  }


yamSeg* expandTry1
(  yamSeg*  seg
)
   {  mcxTing* stuff    =  arg(arg1_g)
   ;  mcxbits flag_zoem =  yamDigest(stuff, stuff, seg)
   ;  mcxbool error     =  seg->flags & SEGMENT_ERROR
   ;  mcxbool bailout   =  seg->flags & SEGMENT_DONE

   ;  BIT_OFF(seg->flags, SEGMENT_INTERRUPT)

   ;  yamKeySet("__zoemput__", stuff->str)
   ;  yamKeySet
      ("__zoemstat__", !flag_zoem || bailout ? "done" : error ? "error" : "towel")

   ;  mcxTingFree(&stuff)
   ;  return seg
;  }


/*
 *  fixme improve control
 *  this one does not exit on failure
*/

yamSeg* expandZinsert1
(  yamSeg*  seg
)
   {  mcxTing*  fname      =  arg(arg1_g)
   ;  mcxTing*  filetxt    =  mcxTingEmpty(NULL, 100)
   ;  mcxIO*  xf           =  NULL
   ;  yamSeg* newseg       =  NULL

   ;  if (yamDigest(fname, fname, seg))
      {  mcxTingFree(&filetxt)
      ;  mcxTingFree(&fname)
      ;  return yamSegPushEmpty(seg)
   ;  }
      else if (!strcmp(fname->str, "stdia"))
      {  sourceStdia(filetxt)
      ;  newseg = seg
   ;  }
      else
      {  mcxTing* newtxt
      ;  mcxTing* fnorig = mcxTingNew(fname->str)
      ;  const mcxTing* infile = NULL
      ;  if (!(xf = yamTryOpen(fname, &infile, 1)) && !infile)
         {  mcxTell
            (  "zinsert#1"
            ,  "failure opening file <%s> (rerun?)"
            ,  fnorig->str
            )
         ;  newseg = seg
      ;  }
         else if (infile)
         {  newtxt = mcxTingPrint(NULL, "\\!{%s}", infile->str)
         ;  newseg =  yamSegPush(seg, newtxt)
      ;  }
         else
         {  if (mcxIOreadFile(xf, filetxt))
            yamErr("zinsert#1", "error reading file <%s>", xf->fn->str)
         ;  else
               newtxt = mcxTingPrint(NULL, "\\!{%s}", filetxt->str)
            ,  newseg =  yamSegPush(seg, newtxt)
      ;  }
         mcxTingFree(&fnorig)
   ;  }

      mcxIOfree(&xf)
   ;  mcxTingFree(&filetxt)
   ;  mcxTingFree(&fname)
   ;  return newseg
;  }


void text_map_vigenere
(  mcxTing* data
,  mcxTing* key
)
   {  char* p
   ;  if (!key->len || mcxStrChrAint(key->str, islower, key->len))
      {  mcxErr("vigenere", "need lowercase key")
      ;  return
   ;  }
      for (p=data->str;p<data->str+data->len;p++)
      {  unsigned char c = *p
      ;  int shift = (unsigned char) key->str[(p-data->str)%key->len] - 'a' + 1
      ;  if (islower(c))
         *p = "abcdefghijklmnopqrstuvwxyz"[(c-'a'+shift)%26]
      ;  else if (isupper(c))
         *p = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[(c-'A'+shift)%26]
   ;  }
   }


void text_map_vigenerex
(  mcxTing* data
,  mcxTing* key
)
   {  char* p
   ;  if (!key->len)
      {  mcxErr("vigenere", "need key")
      ;  return
   ;  }
      for (p=data->str;p<data->str+data->len;p++)
      {  unsigned char c = *p
      ;  int shift = (unsigned char) key->str[(p-data->str)%key->len] - 'a' + 1
      ;  if (shift < 0)
         shift *= -1
      ;  if (islower(c))
         *p = "abcdefghijklmnopqrstuvwxyz_"[(c-'a'+shift)%27]
      ;  else if (isupper(c))
         *p = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_"[(c-'A'+shift)%27]
      ;  else if (c == ' ')
         *p = "abcdefghijklmnopqrstuvwxyz_"[(26+shift)%27]
   ;  }
   }


void text_map_caesar
(  mcxTing* data
,  int shift
)
   {  char* p
   ;  for (p=data->str;p<data->str+data->len;p++)
      {  unsigned char c = *p
      ;  if (islower(c))
         *p = "abcdefghijklmnopqrstuvwxyz"[(c-'a'+shift)%26]
      ;  else if (isupper(c))
         *p = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[(c-'A'+shift)%26]
   ;  }
   }


void text_map_alpha
(  mcxTing* data
,  int base
)
   {  int x, r, l, i, d = 0
   ;  x = atoi(data->str)

   ;  if (x <0)
         mcxTingWrite(data, "-")
      ,  x = -x
      ,  d = 1
   ;  else
      mcxTingWrite(data, "")

   ;  do
      {  r = x % base
      ;  x = x / base
      ;  mcxTingNAppend(data, "_abcdefghijklmnopqrstuvwxyz"+r, 1)
   ;  }                                                 /* ^ clangface */
      while (x)

   ;  l = data->len

   ;  for (i=d;i<l/2;i++)
      {  char f = data->str[i], g = data->str[l-i-1]
      ;  data->str[i] = g
      ;  data->str[l-i-1] = f
   ;  }
   }


void text_map_case
(  mcxTing*  data
,  mcxbool   up
)
   {  char* p
   ;  for (p=data->str;p<data->str+data->len;p++)
      {  unsigned char c = *p
      ;  if (isalpha(c))
         *p = up ? toupper(c) : tolower(c)
   ;  }
   }


yamSeg* expandTextmap2
(  yamSeg*  seg
)
   {  mcxTing* ops   =  arg(arg1_g)
   ;  mcxTing* data  =  arg(arg2_g)
   ;  yamSeg* segops =  yamStackPushTmp(ops)
   ;  const char* me =  "\\textmap#2"
   ;  mcxbits flag_zoem = 0
   ;  int x = 0

   ;  if ((flag_zoem = yamDigest(data, data, seg)))

   ;  else
      while (2 == (x = yamParseScopes(segops, 2, 0)))
      {  const char* key = arg1_g->str    /* dangersign */
      ;  const char* val = arg2_g->str    /* dangersign */

      ;  if (!strcmp(key, "word"))
         {  if (!strcmp(val, "ucase"))
            text_map_case(data, TRUE)
         ;  else if (!strcmp(val, "lcase"))
            text_map_case(data, FALSE)
         ;  else
            yamErr(me, "unknown word mode <%s> (ignoring)", val)
      ;  }
         else if (!strcmp(key, "number"))
         {  if (!strcmp(val, "roman"))
            mcxTingRoman(data, atoi(data->str), FALSE)
         ;  else if (!strcmp(val, "alpha") || !strcmp(val, "alpha27"))
            text_map_alpha(data, 27)
         ;  else if (!strcmp(val, "alpha10"))
            text_map_alpha(data, 10)
         ;  else if (!strcmp(val, "alpha2"))
            text_map_alpha(data, 2)
         ;  else
            yamErr(me, "unknown number mode <%s> (ignoring)", val)
      ;  }
         else if (!strcmp(key, "caesar"))
         {  int num = atoi(val) % 26
         ;  text_map_caesar(data, num)
      ;  }
         else if (!strcmp(key, "repeat"))
         {  mcxTing* num = arg(arg2_g), *t
         ;  int n
         ;  if ((flag_zoem = yamDigest(num, num, seg)))
            break                   /* fixme memleak */
         ;  n = atoi(num->str)
         ;  t = mcxTingKAppend(NULL, data->str, n < 0 ? 0 : n)
         ;  mcxTingWrite(data, t->str)
         ;  mcxTingFree(&t)
         ;  mcxTingFree(&num)
      ;  }
         else if (!strcmp(key, "vigenere"))
         text_map_vigenere(data, arg2_g)     /* dangersign */
      ;  else if (!strcmp(key, "vigenerex"))
         text_map_vigenerex(data, arg2_g)     /* dangersign */
      ;  else
         yamErr(me, "unknown mode <%s> (ignoring)", key)
   ;  }

      if (x < 0 || x)      /* error resp trailing arg */
      yamErr(me, "syntax error (ignoring)")

   ;  if (flag_zoem)
      mcxTingFree(&data)
   ;  yamStackFreeTmp(&segops)
   ;  mcxTingFree(&ops)

   ;  return flag_zoem ? yamSegPushEmpty(seg) : yamSegPush(seg, data)
;  }


yamSeg* expandProtect1
(  yamSeg*  seg
)
   {  mcxTing*  protected = yamProtect(arg1_g)
   ;  return yamSegPush(seg, protected)
;  }


yamSeg* expandFinsert1
(  yamSeg*  seg
)
   {  mcxTing*  fname   =  arg(arg1_g)
   ;  mcxTing*  filetxt =  mcxTingEmpty(NULL, 100)
   ;  mcxIO* xf         =  NULL
   ;  mcxbits flag_zoem =  yamDigest(fname, fname, seg)

   ;  if (!flag_zoem && !(xf = yamTryOpen(fname, NULL, 1)))
      {  flag_zoem = SEGMENT_ERROR
      ;  mcxTell
         (  "finsert#1"
         ,  "failure opening file <%s> (rerun?)"
         ,  fname->str
         )
      ;  seg_check_flag(flag_zoem, seg)
   ;  }

      if (!flag_zoem)
      yamReadData(xf, filetxt)
   ;  else
      mcxTingEmpty(filetxt, 0)

   ;  mcxIOfree(&xf)
   ;  mcxTingFree(&fname)

   ;  return yamSegPush(seg, filetxt)
;  }


yamSeg* expandDofile2
(  yamSeg*  seg
)
   {  mcxTing*  fnsearch   =  arg(arg1_g)
   ;  mcxTing*  fname      =  NULL
   ;  mcxTing*  opts       =  arg2_g
   ;  mcxbool   ok         =  FALSE
   ;  const char* me       =  "\\dofile#2"

   /*  DON'T DIGEST OR OUTPUT AS LONG AS OPTS IS IN SCOPE */

   ;  int  mode            =  0

   ;  do
      {  if (opts->len != 2)
         {  yamErr(me, "Second arg <%s> not in {!?}x{+-}", opts->str)
         ;  break
      ;  }

                                          /* do output, use default sink */
         if (!strncmp(opts->str, "!+", 2) || !strncmp(opts->str, "?+", 2))
         mode = SOURCE_DEFAULT_SINK
                                          /* do not do output */
      ;  else if (!strncmp(opts->str, "!-", 2) || !strncmp(opts->str, "?-", 2))
        /* OK */
      ;  else
         {  yamErr(me, "Second arg <%s> not in {!?}x{+-}", opts->str)
         ;  break
      ;  }

         if (!sourceCanPush())
         {  yamErr
            (  me, "maximum file include depth (9) reached"
               "___ when presented with file <%s>"
            ,  fnsearch->str
            )
         ;  break
      ;  }

         if (yamDigest(fnsearch, fnsearch, seg))
         break
      ;  fname = mcxTingNew(fnsearch->str)
      ;

         {  mcxstatus status = sourceAscend(fnsearch, mode, chunk_size)

         ;  if (status == STATUS_FAIL_OPEN)
            {  if (strchr(opts->str, '?'))
               {  ok = TRUE
               ;  break
            ;  }
               else
               {  mcxErr(me, "failed to open file <%s>", fname->str)
               ;  break
            ;  }
            }
            else if (status != STATUS_OK)
            {  mcxErr
               (  me
               ,  "error (%d) occurred while reading file <%s>"
               ,  (int) status
               ,  fname->str
               )
            ;  break
         ;  }
         }
         ok = TRUE
   ;  }
      while (0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&fnsearch)
   ;  mcxTingFree(&fname)
   ;  return ok ? seg : yamSegPushEmpty(seg)
;  }


yamSeg* expandXtag2
(  yamSeg*  seg
)
   {  mcxTing* tag = arg(arg1_g)
   ;  mcxTing* ops = arg(arg2_g)
   ;  return yamXtag(seg, tag, ops)  
;  }


yamSeg* expandXtag1
(  yamSeg*  seg
)
   {  mcxTing* tag = arg(arg1_g)
   ;  return yamXtag(seg, tag, NULL)  
;  }


yamSeg* expandBang1
(  yamSeg*  seg
)
   {  mcxTing* stripped = mcxTingNew("\\")
   ;  if (arg1_g->len > 1)
         mcxTingAppend(stripped, arg1_g->str)
      ,  mcxTingShrink(stripped, -1)
   ;  return yamSegPushx(seg, stripped, SEGMENT_CONSTANT)
;  }


yamSeg* expandBang2
(  yamSeg*  seg
)
   {  if (arg1_g->len == 1)
      return
      yamSegPushx(seg, arg(arg2_g), seg->flags | SEGMENT_CONSTANT)
   ;  else
      {  mcxTing* txt
         =  mcxTingPrint
            (NULL, "\\%.*s{%s}", (int) (arg1_g->len-1), arg1_g->str, arg2_g->str)
      ;  return yamSegPushx(seg, txt, SEGMENT_CONSTANT)
   ;  }
      return NULL    /* unreachcode */
;  }


yamSeg* expand_while_
(  yamSeg*  seg
,  mcxbits  opts
)
   {  mcxbool   dowhile    =  opts & 1
   ;  mcxbool   whilst     =  opts & 2
   ;  mcxTing*  condition  =  mcxTingNew(dowhile ? arg2_g->str : arg1_g->str)
   ;  mcxTing*  data       =  mcxTingNew(dowhile ? arg1_g->str : arg2_g->str)
   ;  mcxTing*  condition_ =  mcxTingEmpty(NULL, 10)
   ;  mcxTing*  data_      =  mcxTingEmpty(NULL, 10)
   ;  mcxTing*  newtxt     =  mcxTingEmpty(NULL, 10)
   ;  mcxbits flag_zoem    =  STATUS_OK
   ;  mcxbool   guard      =  TRUE
#if 0
   ;  sink*     sk         =  sinkGetDefault()
#endif

   ;  if (!dowhile)
      {  mcxTingWrite(condition_, condition->str)
      ;  if
         (  (flag_zoem = yamDigest(condition_, condition_, seg))
         || !atol(condition_->str)
         )
         guard = FALSE
   ;  }

      do
      {  if (!guard)
         break

      ;  mcxTingWrite(data_, data->str)

      ;  if (whilst)
         flag_zoem = yamOutput(data_, NULL, ZOEM_FILTER_DEFAULT)
      ;  else
         {  flag_zoem = yamDigest(data_, data_, seg)
         ;  mcxTingAppend(newtxt, data_->str)
      ;  }
                /* fixme (doc?) a reason we append even ico failure? [yes] */
         if (flag_zoem & SEGMENT_INTERRUPT)
         break

      ;  mcxTingWrite(condition_, condition->str)
      ;  if ((flag_zoem = yamDigest(condition_, condition_, seg)))
         break
      ;  guard =  atol(condition_->str) ? TRUE : FALSE
   ;  }
      while (1)

   ;  mcxTingFree(&data)
   ;  mcxTingFree(&data_)
   ;  mcxTingFree(&condition)
   ;  mcxTingFree(&condition_)

   ;  if (flag_zoem & SEGMENT_DONE)
      seg->flags =0
                     /* flag_zoem might be segless, from yamoutput */
   ;  else
      seg_check_flag(flag_zoem, seg)

                     /* fixordoc: why is pushEmpty not needed? */ 
   ;  return yamSegPush(seg, newtxt)
;  }


yamSeg* expandDowhile2
(  yamSeg*  seg
)
   {  return expand_while_(seg, 1)
;  }


yamSeg* expandWhile2
(  yamSeg*  seg
)
   {  return expand_while_(seg, 0)
;  }


yamSeg* expandWhilst2
(  yamSeg*  seg
)
   {  return expand_while_(seg, 2)
;  }


yamSeg* expandApply2x
(  yamSeg*  seg
,  const char* along
)
   {  mcxTing *data        =  arg(arg2_g)
   ;  mcxTing *newtxt      =  mcxTingEmpty(NULL, 10)
   ;  char* p              =  arg1_g->str
   ;  int delta            =  0
   ;  const char* me       =  "\\apply#2"
   ;  mcxTing *key         =  NULL
   ;  yamSeg *tblseg       =  NULL
   ;  int   x, k, keylen, namelen, i = 0
   ;  mcxbool ok           =  FALSE
   ;  mcxbits keybits      =  0

   ;  while (isspace((unsigned char) *p))
      p++

   ;  key = mcxTingNew(p)

   ;  do
      {  if (yamDigest(data, data, seg))
         break
      ;  if (yamDigest(key, key, seg))
         break

      ;  keylen = checkusrsig(key->str, key->len, &k, &namelen, &keybits)

      ;  if (keylen < 0 || namelen < 0)
         {  yamErr(me, "key part not ok")
         ;  break
      ;  }

         if (k<=0 || k > 9)
         {  yamErr
            (me, "arg number <%d> not in [1,9] for key <%s>", k, key->str)
         ;  break
      ;  }

         if (!(keybits & KEY_CALLBACK))
         {  yamErr(me, "cannot operate on key <%s>", key->str)
         ;  break
      ;  }
         else if (keybits & KEY_ANON)
         {  int cc
         ;  if ((cc = yamClosingCurly(key, keylen, NULL, RETURN_ON_FAIL))<0)
            {  yamErr
               (  me
               ,  "anonymous key <%s> not ok (%d/%d)"
               ,  key->str
               ,  cc+keylen+1
               ,  key->len
               )
            ;  break
         ;  }
            mcxTingNWrite(key_g, key->str, keylen)
         ;  mcxTingNWrite(arg1_g, key->str+keylen+1, cc-1)
         ;  delta = 1
      ;  }
         else if (keylen != key->len)     /* mixed sign comparison */
         {  yamErr
            (  me
            ,  "key <%s> is not of the right \\foo, \\\"foo::foo\", and \\$foo"
            ,  key->str
            )
         ;  break
      ;  }
         else if (keybits & KEY_PRIME)
         mcxTingWrite(key_g, key->str+1)
      ;  else if (keybits & KEY_GLOBAL)
         mcxTingWrite(key_g, key->str+2)
      ;  else
         mcxTingWrite(key_g, key->str)

      ;  tblseg = yamStackPushTmp(data)

               /* perhaps this block should be encapsulated by parse.c
                * pity we have yamExpandKey here.
                * Also, yamStackPushTmp would like to set bit
                * that is checked by yamSegNew, but we cannot do that
                * because of the yamExpandKey below.
                * Seems we do full expansion here.
               */
      ;  while ((x = yamParseScopes(tblseg, k, delta)) == k)
         {  yamSeg* newseg  = yamExpandKey(tblseg, keybits)
         ;  if (!newseg)
            {  yamErr(me, "key <%s> does not expand", key->str)
            ;  goto done         /* fixme */
         ;  }
            else if (newseg != tblseg) /* primitives may return same segment */
            {  if (along)
               mcxTingPrintAfter(newtxt, "\\set{%s}{%d}", along, i)
            ;  mcxTingAppend(newtxt, newseg->txt->str)
            ;  yamSegFree(&newseg)
         ;  }
            i++
      ;  }

         if (!x)

      ;  else if ( x < 0)
         {  mcxErr(me, "parse error!")
         ;  break
      ;  }
         else if (x < k)
         mcxErr(me, "(ignoring) trailing arguments")

      ;  ok = TRUE
   ;  }
      while (0)

   ;  done : seg_check_ok(ok, seg)

   ;  mcxTingFree(&data)
   ;  mcxTingFree(&key)
   ;  yamStackFreeTmp(&tblseg)
   ;  return yamSegPush(seg, newtxt)
;  }


   /* todo make sure along is permissible key */
yamSeg* expandSeqalong3
(  yamSeg*  seg
)
   {  mcxTing* along = arg(arg1_g)
   ;  yamSeg*  newseg = NULL
   ;  mcxTingWrite(arg1_g, arg2_g->str)
   ;  mcxTingWrite(arg2_g, arg3_g->str)
   ;  newseg = expandApply2x(seg, along->str)
   ;  mcxTingFree(&along)
   ;  return newseg
;  }


yamSeg* expandApply2
(  yamSeg*  seg
)
   {  return expandApply2x(seg, NULL)
;  }


yamSeg* expandTable5
(  yamSeg*  seg
)
   {  mcxTing *txtnum   =  arg(arg1_g)
   ;  mcxTing *txtlft   =  arg(arg2_g)
   ;  mcxTing *txtmdl   =  arg(arg3_g)
   ;  mcxTing *txtrgt   =  arg(arg4_g)
   ;  mcxTing *data     =  arg(arg5_g)
   ;  mcxbool ok        =  FALSE
   ;  yamSeg *tmpseg    =  NULL

   ;  mcxTing *txtall   =  mcxTingEmpty(NULL, 100)

   ;  int  x, k

   ;  do
      {  if (yamDigest(data, data, seg) || yamDigest(txtnum, txtnum, seg))
         break

      ;  k = atoi(txtnum->str)

      ;  if (k<=0)
         {  yamErr("\\table#5", "nonpositive loop number <%d>", k)
         ;  break
      ;  }

         tmpseg = yamStackPushTmp(data)

      ;  while ((x = yamParseScopes(tmpseg, k, 0)) == k)
         {  int i
         ;  mcxTingAppend(txtall, txtlft->str)
         ;  for (i=1;i<k;i++)
            {  mcxTingAppend(txtall, (key_and_args_g+i)->str)
            ;  mcxTingAppend(txtall, txtmdl->str)
         ;  }
            mcxTingAppend(txtall, (key_and_args_g+k)->str)
         ;  mcxTingAppend(txtall, txtrgt->str)
      ;  }

         if (x < 0)
         break
      ;  ok =  TRUE
   ;  }
      while (0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&txtnum)
   ;  mcxTingFree(&txtlft)
   ;  mcxTingFree(&txtmdl)
   ;  mcxTingFree(&txtrgt)

   ;  yamStackFreeTmp(&tmpseg)
   ;  mcxTingFree(&data)

   ;  return yamSegPush(seg, txtall)
;  }


yamSeg*  expandFormat2
(  yamSeg* seg
)
   {  return yamFormat2(seg)
;  }


yamSeg* expandFormatted1
(  yamSeg*  seg
)
   {  return yamFormatted1(seg, arg1_g->str)
;  }


yamSeg* expandWrite3
(  yamSeg*  seg
)
   {  mcxTing*    fname    =  arg(arg1_g)
   ;  mcxTing*    yamtxt   =  arg(arg3_g)
   ;  mcxIO       *xfout   =  NULL
   ;  mcxbits   flag_zoem  =  SEGMENT_ERROR
   ;  int         fltidx   =  1

   ;  fflush(NULL)

   ;  fltidx =    !strcmp(arg2_g->str, "device")
               ?  ZOEM_FILTER_DEVICE
               :     !strcmp(arg2_g->str, "txt")
                  ?  ZOEM_FILTER_TXT
                  :     !strcmp(arg2_g->str, "copy")
                     ?  ZOEM_FILTER_COPY
                     :  -1
   ;  do
      {  if (fltidx < 0)
         {  yamErr("\\write#3", "unknown filter <%s>", arg2_g->str)
         ;  break
      ;  }

         if (yamDigest(fname, fname, seg))
         break

      ;  if (!(xfout =  yamOutputNew(fname->str)))
         break

;if(0)fprintf(stderr, "set fnwrite %s\n", fname->str)
      ;  if (strcmp(fname->str, "stderr"))
         yamKeyDef("__fnwrite__", fname->str)

      ;  /* sinkPush(xfout->usr, fltidx) */

      ;  if
         (  (flag_zoem = yamOutput(yamtxt, xfout->usr, fltidx))
         && !stressWrite
         )
         break
      ;  flag_zoem = 0
   ;  }
      while (0)

   ;  if (xfout)
      {  fflush(xfout->fp)
      ;  /* sinkPop(xfout->usr) */
   ;  }

      if (strcmp(fname->str, "stderr"))
      yamKeyDeletex("__fnwrite__")

;if(0)fprintf(stderr, "unset fnwrite %s\n", fname->str)
   ;  seg_check_flag(flag_zoem, seg)

   ;  mcxTingFree(&fname)
   ;  mcxTingFree(&yamtxt)
   ;  return flag_zoem ? yamSegPushEmpty(seg) : seg
;  }


yamSeg* expandDollar2
(  yamSeg*  seg
)
   {  mcxTing*  device     =  yamKeyGet(devtxt_g)
   ;  if (!device)
      {  yamErr
         (  "\\$"
         ,  "key [\\__device__] not defined, rendering use of <%s> useless"
         ,  key_g->str
         )
      ;  seg_check_ok(FALSE, seg)
      ;  return yamSegPushEmpty(seg)
   ;  }
      else if (!strcmp(device->str, arg1_g->str))
      {  mcxTing* txt = arg(arg2_g)
      ;  return yamSegPush(seg, txt)
   ;  }
      else
      return  seg
;  }


yamSeg* expandUndef1
(  yamSeg*  seg
)
   {  int namelen = 0
   ;  mcxbits keybits = 0
   ;  int keylen = checkusrsig(arg1_g->str, arg1_g->len, NULL, &namelen, &keybits)
   ;  const char* me = "\\undef#1"
   ;  mcxTing* val = NULL

   ;  do
      {  const char* key = arg1_g->str      /* dangersign, do not eval henceforth */

      ;  if (keylen <= 0 || keylen != arg1_g->len)       /* mixed sign comparison */
         {  yamErr(me, "not a valid key signature: <%s>", arg1_g->str)
         ;  break
      ;  }

         if (keybits & KEY_GLOBAL)
         key += 2

      ;  if (!(val = yamKeyDelete(key, keybits & KEY_GLOBAL)))
         {  yamErr(me, "key <%s> not defined in target scope", arg1_g->str)
         ;  break
      ;  }
      }
      while (0)

   ;  if (!val)
      {  seg_check_ok(FALSE, seg)
      ;  return yamSegPushEmpty(seg)
   ;  }

      mcxTingFree(&val)
   ;  return seg
;  }


yamSeg* expandDefined2
(  yamSeg*  seg
)
   {  mcxTing *val
   ;  mcxTing *type  =  arg(arg1_g)
   ;  mcxTing *access=  arg(arg2_g)
   ;  char*    yes   = "1"
   ;  char*    no    = "0"
   ;  char*    yn    = "no"
   ;  mcxbool  ok    =  FALSE
   ;  const char* me =  "\\defined#2"
   ;  mcxbits  keybits = 0
   ;  int namelen    =  0

   ;  do
      {  if (yamDigest(access, access, seg))
         {  yamErr(me, "access string does not eval")
         ;  break
      ;  }

         if (!strcmp(type->str, "key") || !strcmp(type->str, "lkey"))
         {  if
            (  checkusrsig(access->str, access->len, NULL, &namelen, &keybits)
            != access->len       /* mixed sign comparison */
            )
            {  yamErr
               (me, "argument <%s> is not a valid key signature", type->str)
            ;  break
         ;  }
         }

         if (!strcmp(type->str, "key"))
         {  val   =  yamKeyGet(access)
         ;  yn    =  val ? yes : no
      ;  }
         else if (!strcmp(type->str, "lkey"))
         {  val   =  yamKeyGetLocal(access)
         ;  yn    =  val ? yes : no
      ;  }
         else if (!strcmp(type->str, "data") || !strcmp(type->str, "%"))
         {  if (yamOpsDataAccess(access))
            break
         ;  yn    =  yamDataGet() ? yes : no
      ;  }
         else if
         (  !strcmp(type->str, "zoem")
         || !strcmp(type->str, "primitive")
         || !strcmp(type->str, "builtin")
         )
         {  const char* cps  =  NULL
         ;  xpnfnc xpf       =  yamOpGet(access, &cps)
         ;  yn  =    !strcmp(type->str, "zoem") && (cps || xpf)
                  ?  yes
                  :     !strcmp(type->str, "primitive") && xpf
                     ?  yes
                     :     !strcmp(type->str, "builtin") && cps
                        ?  yes
                        :  no
      ;  }
         else if (!strcmp(type->str, "ENV"))
         {  yn    =  getenv(access->str) ? yes : no
      ;  }
         else
         {  yamErr(me, "invalid type <%s>", type->str)
         ;  break
      ;  }
         ok  = TRUE
   ;  }
      while (0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&access)
   ;  mcxTingFree(&type)
   ;  return
         ok
      ?  yamSegPush(seg, mcxTingNew(yn))
      :  yamSegPushEmpty(seg)
;  }


yamSeg* expandIf3
(  yamSeg*  seg
)
   {  mcxTing* bool  =  arg(arg1_g)
   ;  mcxTing* case1 =  arg(arg2_g)
   ;  mcxTing* case0 =  arg(arg3_g)
   ;  int b

   ;  if (yamDigest(bool, bool, seg))
      {  yamErr("\\if#3", "condition does not parse")
      ;  mcxTingFree(&case0)
      ;  mcxTingFree(&case1)
      ;  mcxTingFree(&bool)
      ;  return yamSegPushEmpty(seg)
   ;  }

      b =   bool->len ? atoi(bool->str) :  0

   ;  mcxTingFree(&bool)

   ;  if (b)
      {  mcxTingFree(&case0)
      ;  return yamSegPush(seg, case1)
   ;  }
      else
      {  mcxTingFree(&case1)
      ;  return yamSegPush(seg, case0)
   ;  }

      return NULL    /* unreachcode */
;  }


/*
 * Does not change the contents of access, does not claim ownership.
 * *DOES* take ownership of argk_g.
*/

mcxstatus yamOpsDataAccess
(  const mcxTing*    access
)
   {  if (access->len == 0)
      {  n_args_g = 0
   ;  }
      else if (seescope(access->str, access->len) >= 0)
      {  yamSeg* tmpseg = yamStackPushTmp((mcxTing*) access)   /* dirty */
      ;  if (yamParseScopes(tmpseg, 9, 0) < 0)
         {  yamStackFreeTmp(&tmpseg)
         ;  return STATUS_FAIL
      ;  }
         yamStackFreeTmp(&tmpseg)
   ;  }
      else
      {  n_args_g = 1
      ;  mcxTingWrite(arg1_g, access->str)
   ;  }
      return STATUS_OK
;  }


yamSeg* expandGrapeDump
(  yamSeg*  seg
)
   {  mcxTing* access = arg(arg1_g)
   ;  mcxbool ok = TRUE

   ;  do
      {  if (yamDigest(access, access, seg))
         break
      ;  if (yamOpsDataAccess(access))
         break
      ;  if (yamDataPrint())
         {  yamErr("\\dump#1", "no value associated with <%s>", access->str)
         ;  break
      ;  }
         ok =  TRUE
   ;  }
      while (0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&access)
   ;  return seg
;  }


yamSeg* expandGrapeFree
(  yamSeg*  seg
)
   {  mcxTing*  access = arg(arg1_g)
   ;  mcxbool ok = TRUE

  /*  NOTE this routine never fails, we start with ok = TRUE */

   ;  do
      {  if (yamDigest(access, access, seg))
         break
      ;  if (yamOpsDataAccess(access))
         break
      ;  if (yamDataFree())
         {  yamErr("\\free#1", "no value associated with <%s>", access->str)
         ;  break
      ;  }
         ok =  TRUE
   ;  }
      while (0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&access)
   ;  return seg
;  }



/* Never fails, only warns
*/

yamSeg* expandGrapeGet
(  yamSeg*  seg
)
   {  const char* str = NULL
   ;  mcxTing* access = arg(arg1_g)
   ;  mcxbool ok  =  TRUE   /* on purpose; grape key absence -> "" */
   
   ;  do
      {  if (yamDigest(access, access, seg))
         break
      ;  if (yamOpsDataAccess(access))
         break
      ;  if (!(str = yamDataGet()))
         {  yamErr("\\%#1", "no value associated with <%s>", access->str)
         ;  break
      ;  }
         ok =  TRUE
   ;  }
      while(0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&access)
   ;  return yamSegPush(seg, mcxTingNew(str ? str : ""))
;  }


mcxstatus veto_system
(  char* args[]
,  int l
,  const char* me
)
   {  mcxTing* ask = mcxTingEmpty(NULL, 80)
   ;  mcxstatus status = STATUS_FAIL
   ;  int i = 0

   ;  do
      {  mcxTingWrite
         (  ask
         ,  "\n? do you want this command to be exercised? (y/n)\n? ["
         )
      ;  for (i=0; i<l; i++)
         mcxTingPrintAfter
         (  ask
         ,  "%s%s"
         ,  args[i]
         ,  i < l-1 ? " " : "]\n? "
         )
      ;  if (ask_user(ask, me))
         break

      ;  status = STATUS_OK
   ;  }
      while (0)

   ;  mcxTingFree(&ask)
   ;  return status
;  }


yamSeg* expandSystem3
(  yamSeg*  seg
)
   {  mcxTing*    cmd   =  arg(arg1_g) 
   ;  mcxTing*    cmd_  =  mcxTingPrint(NULL, ":%s:", arg1_g->str)
   ;  mcxTing*    arx   =  arg(arg2_g) 
   ;  mcxTing*    data  =  arg(arg3_g) 
   ;  mcxTing*    out   =  NULL
   ;  yamSeg*     argseg=  NULL
   ;  char*       args[YAM_ARG_MAX+1]
   ;  int         k, i
   ;  mcxbool     ok    =  FALSE
   ;  const char* me    =  "\\system#3"
   ;  mcxbool     listed=  system_allow && strstr(system_allow->str, cmd_->str)
                           ?  TRUE
                           :  FALSE

   ;  mcxTingFree(&cmd_)
   ;  do
      {  if (!listed && systemAccess == SYSTEM_SAFE)
         {  yamErr
            (  me
            ,  "system calls are not allowed (use --unsafe or --unsafe-silent)"
            )
         ;  break
      ;  }
         if (yamDigest(arx, arx, seg))
         break
      ;  if (yamDigest(data, data, seg))
         break
      ;  argseg = yamStackPushTmp(arx)

      ;  args[0]  =  cmd->str
      ;  if ((k = yamParseScopes(argseg, YAM_ARG_MAX, 0)) < 0)
         {  yamErr(me, "second argument not a vararg")
         ;  break
      ;  }
         for (i=0; i<k; i++)
         {  yamUnprotect(key_and_args_g+i+1)
         ;  args[i+1] = key_and_args_g[i+1].str
      ;  }
         if
         (  !listed
         && systemAccess == SYSTEM_UNSAFE
         && veto_system(args, k+1, me)
         )
         break

      ;  args[k+1] = NULL
      ;  if (!(out = yamSystem(args[0], args, data)))
         break
      ;  ok = TRUE
   ;  }        /* fixmefixme yamSystem returns != NULL for some failures */
      while (0)

   ;  mcxTingFree(&data)
   ;  mcxTingFree(&cmd)
   ;  yamStackFreeTmp(&argseg)
   ;  mcxTingFree(&arx)   /* mq must this be done after yamSegFree? */

   ;  if (!out)
      out = mcxTingEmpty(NULL, 0)

   ;  if (!ok && !systemHonor && !(seg->flags & SEGMENT_INTERRUPT))
      mcxErr(me, "continuing (cf --system-honor)")
   ;  else if (!ok)
      seg_check_ok(ok, seg)

   ;  return yamSegPush(seg, out)  /* fixme: systemHonor logic still ok ? */
;  }


                                 /* pivot ? switch : branch */
yamSeg* expand_switch_
(  yamSeg*  seg
,  mcxTing* pivot                /* we claim ownership */
,  mcxTing* body                 /* we claim ownership */
)
   {  mcxTing*  clause  =  mcxTingEmpty(NULL, 30)
   ;  mcxTing*  yamtxt  =  mcxTingEmpty(NULL, 30)
   ;  mcxbool  ok       =  TRUE
   ;  int   x           =  -1

   ;  yamSeg*  tmpseg   =  yamStackPushTmp(body)

   ;  if
      (  (tmpseg->flags & SEGMENT_ERROR)
      || (pivot && yamDigest(pivot, pivot, seg))
      )
      ok  = FALSE

   ;  while (ok && (x = yamParseScopes(tmpseg, 2, 0)) == 2)
      {  mcxTingWrite(clause, arg1_g->str)
      ;  mcxTingWrite(yamtxt, arg2_g->str)

      ;  if (yamDigest(clause, clause, seg))
         {  ok = FALSE
         ;  break
      ;  }
         if (pivot)
         {  int n_scopes = yamCountScopes(clause, 0)
         ;  if (n_scopes >= 1)
            {  yamSeg* tmp2seg = yamStackPushTmp(clause)
            ;  int y = yamParseScopes(tmp2seg, 9, 0), yi
            ;  for (yi=1;yi<=y;yi++)
               if (!strcmp(pivot->str, key_and_args_g[yi].str))
               break
            ;  if (yi != y+1)
               break
         ;  }
            else if (!strcmp(clause->str, pivot->str))
            break
      ;  }
         else                    /* branch: empty string fails */
         {  if (clause->len && atol(clause->str))
            break
      ;  }
      }


      if (!ok)                      /* clause parse error or stack size */
      /* NOTHING */
   ;  else if (x < 0)               /* parse error (e.g. no closing scope) */
      ok = FALSE
   ;  else if (x == 1)              /* fall through / else clause */
      mcxTingWrite(yamtxt, arg1_g->str)
   ;  else if (x == 0)              /* nothing matched */
      mcxTingEmpty(yamtxt, 0)

   ;  if (!ok)
      mcxTingEmpty(yamtxt, 0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&pivot)
   ;  mcxTingFree(&clause)

   ;  yamStackFreeTmp(&tmpseg)
   ;  mcxTingFree(&body)

   ;  return yamSegPush(seg, yamtxt)
;  }


yamSeg* expandSwitch2
(  yamSeg*  seg
)
   {  mcxTing*  pivot      =  arg(arg1_g)
   ;  mcxTing*  body       =  arg(arg2_g)
   ;  return expand_switch_(seg, pivot, body)
;  }


yamSeg* expandBranch1
(  yamSeg*  seg
)
   {  mcxTing*  body       =  arg(arg1_g)
   ;  return expand_switch_(seg, NULL, body)
;  }


yamSeg* expandConstant1
(  yamSeg*  seg
)
   {  mcxTing*  yamtxt  =  arg(arg1_g)
   ;  yamSeg*  newseg   =  yamStackPushTmp(yamtxt)
   ;  mcxbool  ok       =  TRUE
   ;  int x             =  -1

   ;  while ((x = yamParseScopes(newseg, 2, 0)) == 2)
      {  mcxTing* key      =  arg(arg1_g)
      ;  if (yamConstantNew(key, arg2_g->str) != key)
         mcxTingFree(&key)
   ;  }

      if (x < 0)
      ok  = FALSE
   ;  else if (x == 1)
      yamErr("\\constant#1", "spurious element")

   ;  yamStackFreeTmp(&newseg)
   ;  mcxTingFree(&yamtxt)

   ;  seg_check_ok(ok, seg)
   ;  return seg
;  }


yamSeg* expandSpecial1
(  yamSeg*  seg
)
   {  mcxTing*  yamtxt     =  arg(arg1_g)
   ;  int      x           =  -1
   ;  int      ct          =  0
   ;  mcxbool  ok          =  TRUE  
   ;  yamSeg*  newseg

   ;  if (yamDigest(yamtxt, yamtxt, seg))
      return yamSegPush(seg, mcxTingEmpty(yamtxt, 0))

  ;   newseg = yamStackPushTmp(yamtxt)

   ;  while ((x = yamParseScopes(newseg,2, 0)) == 2)
      {  int   c           =  atoi(arg1_g->str)
      ;  yamSpecialSet(c, arg2_g->str)
      ;  ct += x
   ;  }

      if (x < 0)
      ok  = FALSE
   ;  else if (!ct)
      yamSpecialClean()
         /* perhaps we should reset the special level of the default output
          * stream to 1.
         */
   ;  else if (x == 1)
      yamErr("\\special#1", "spurious element")

   ;  yamStackFreeTmp(&newseg)
   ;  mcxTingFree(&yamtxt)

   ;  seg_check_ok(ok, seg)
   ;  return seg
;  }


yamSeg*  expandRegister2
(  yamSeg* seg
)
   {  if (!strcmp(arg1_g->str, "END"))
      {  mcxTing* regee = arg(arg2_g)
      ;  mcxLink* lk = reg_end_g
      ;  while (lk->next)
         lk = lk->next
      ;  mcxLinkAfter(lk, regee)
   ;  }
      else
      yamErr("\\register#2", "unsupported tag <%s>", arg1_g->str)
   ;  return seg
;  }


mcxstatus mod_registrees
(  void
)
   {  mcxLink* regee = reg_end_g->next
   ;  while (regee)
      {  mcxTing* stuff = regee->val
      ;  if (yamOutput(stuff, NULL, ZOEM_FILTER_DEFAULT))
         break
      ;  regee = regee->next
   ;  }
      return regee ? STATUS_FAIL : STATUS_OK
;  }


yamSeg*  expand_cmp_
(  yamSeg*  seg
,  int      mode
)
   {  mcxTing*  test    =  arg(arg1_g)
   ;  mcxTing*  op1     =  arg(arg2_g)
   ;  mcxTing*  op2     =  arg(arg3_g)
   ;  char*     ret     =  "0"
   ;  mcxbool   ok      =  FALSE
   ;  const char* me    =  mode == 's' ? "cmp#3" : "eqt#3"
   ;  double    diff, abs

   ;  do
      {  if (!strstr("cp\001lt\001lq\001eq\001gq\001gt\001ne", test->str))
         {  mcxErr(me, "unknown mode <%s>", test->str)
         ;  break
      ;  }

         if (yamDigest(op1, op1, seg) || yamDigest(op2, op2, seg))
         break

      ;  diff  =     (mode == 's')
                  ?  strcmp(op1->str, op2->str)
                  :  atof(op1->str) - atof(op2->str)
      ;  abs = diff*MCX_SIGN(diff)

      ;  if (!strcmp(test->str, "cp"))
         ret = abs < precision_g ? "0" : diff > 0 ? "1" : "-1"
      ;  else if
         (  (!strcmp(test->str, "lt") && diff < -precision_g)
         || (!strcmp(test->str, "lq") && diff < precision_g)
         || (!strcmp(test->str, "eq") && abs  <= precision_g)
         || (!strcmp(test->str, "gq") && diff > -precision_g)
         || (!strcmp(test->str, "gt") && diff > precision_g)
         || (!strcmp(test->str, "ne") && abs  > precision_g)
         )
         ret = "1"
      ;  ok = TRUE
   ;  }
      while (0)

   ;  mcxTingFree(&op1)
   ;  mcxTingFree(&op2)
   ;  mcxTingFree(&test)

   ;  seg_check_ok(ok, seg)

   ;  return
         ok
      ?  yamSegPush(seg, mcxTingNew(ret))
      :  yamSegPushEmpty(seg)
;  }


yamSeg*  expandEqt3
(  yamSeg* seg
)
   {  return expand_cmp_(seg, 'i')
;  }

yamSeg*  expandCmp3
(  yamSeg* seg
)
   {  return expand_cmp_(seg, 's')
;  }


yamSeg*  expandInspect4
(  yamSeg* seg
)
   {  return yamInspect4(seg)
;  }


yamSeg*  expandTr2
(  yamSeg* seg
)
   {  mcxTing* specs =  arg(arg1_g)
   ;  mcxTing* data  =  arg(arg2_g)
   ;  mcxTing* src   =  NULL
   ;  mcxTing* dst   =  NULL
   ;  mcxTing* del   =  NULL
   ;  mcxTing* sqs   =  NULL
   ;  mcxbool  ok    =  FALSE

   ;  yamSeg*  newseg   =  yamStackPushTmp(specs)
   ;  int x             =  -1

   ;  while ((x = yamParseScopes(newseg, 2, 0)) == 2)
      {  const char* key =  arg1_g->str

      ;  if (!strcmp(key, "from"))
         src = arg(arg2_g),  ting_tilde_expand(src, YAM_TILDE_UNIX)

      ;  else if (!strcmp(key, "to"))
         dst = arg(arg2_g),  ting_tilde_expand(dst, YAM_TILDE_UNIX)

      ;  else if (!strcmp(key, "delete"))
         del = arg(arg2_g),  ting_tilde_expand(del, YAM_TILDE_UNIX)

      ;  else if (!strcmp(key, "squash"))
         sqs = arg(arg2_g),  ting_tilde_expand(sqs, YAM_TILDE_UNIX)

      ;  else if (!strcmp(key, "debug"))
         mcx_tr_debug = strchr(arg2_g->str, '1') ? TRUE : FALSE

      ;  else
         {  mcxErr("tr#2", "strange key <%s>", key)
         ;  break
      ;  }
      }

      yamStackFreeTmp(&newseg)
   ;  mcxTingFree(&specs)
   ;

      do
      {  int newlen
      ;  if (x != 0)
         break

      ;  if (yamDigest(data, data, seg))
         break

      ;  yamUnprotect(data)

      ;  newlen
         =  mcxTingTr
            (  data
            ,  src ? src->str : NULL
            ,  dst ? dst->str : NULL
            ,  del ? del->str : NULL
            ,  sqs ? sqs->str : NULL
            ,  0
            )

      ;  if (newlen < 0)
         {  yamErr("\\tr#2", "spec did not parse")
         ;  break
      ;  }

         {  mcxTing* newdata = yamProtect(data)
         ;  mcxTingFree(&data)
         ;  data = newdata
      ;  }
         ok = TRUE
   ;  }
      while (0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&src)
   ;  mcxTingFree(&dst)
   ;  mcxTingFree(&del)
   ;  mcxTingFree(&sqs)

   ;  return yamSegPush(seg, data)
;  }


/* strcmp should be factored out */

enum
{  F_UNKNOWN = 1
,  F_MUL ,  F_ADD ,  F_SUB ,  F_POW ,  F_FRAC ,  F_DIV ,  F_MOD
,  F_AND ,  F_OR  ,  F_NOT ,  F_INV
,  F_MAX ,  F_MIN
,  F_CEIL,  F_FLOOR, F_IRAND, F_DRAND
,  F_ABS ,  F_SIGN,  F_ROUND , F_DEC,  F_INC  
}  ;


int fType
(  mcxTing* mode
)
   {  const char* s = mode->str
   ;  if (mode->len == 1)
      switch(*s)
      {  case '+' :                    return F_ADD     ;  break
      ;  case '-' :                    return F_SUB     ;  break
      ;  case '/' :                    return F_FRAC    ;  break
      ;  case '*' :                    return F_MUL     ;  break
      ;  case '%' :                    return F_MOD     ;  break
   ;  }
      else if (mode->len == 3)   /* very poor addressing scheme, I know */
      {       if (!strcmp(s, "abs"))   return F_ABS

      ;  else if (!strcmp(s, "and"))   return F_AND
      ;  else if (!strcmp(s, "min"))   return F_MIN
      ;  else if (!strcmp(s, "max"))   return F_MAX
      ;  else if (!strcmp(s, "div"))   return F_DIV
      ;  else if (!strcmp(s, "mod"))   return F_MOD
      ;  else if (!strcmp(s, "pow"))   return F_POW
      ;  else if (!strcmp(s, "mul"))   return F_MUL
      ;  else if (!strcmp(s, "sub"))   return F_SUB
      ;  else if (!strcmp(s, "add"))   return F_ADD
      ;  else if (!strcmp(s, "sum"))   return F_ADD

      ;  else if (!strcmp(s, "dec"))   return F_DEC
      ;  else if (!strcmp(s, "inc"))   return F_INC
   ;  }
      else if (mode->len == 4)
      {       if (!strcmp(s, "sign"))  return F_SIGN
      ;  else if (!strcmp(s, "ceil"))  return F_CEIL
   ;  }
      else
      {       if (!strcmp(s, "round")) return F_ROUND
      ;  else if (!strcmp(s, "floor")) return F_FLOOR
      ;  else if (!strcmp(s, "irand")) return F_IRAND
      ;  else if (!strcmp(s, "drand")) return F_DRAND
      ;  else if (!strcmp(s, "or"))    return F_OR
      ;  else if (!strcmp(s, "**"))    return F_POW
      ;  else if (!strcmp(s, "//"))    return F_DIV
      ;  else if (!strcmp(s, "&&"))    return F_AND
      ;  else if (!strcmp(s, "||"))    return F_OR
   ;  }
      return F_UNKNOWN
;  }


yamSeg*  expandF2
(  yamSeg* seg
)
   {  mcxTing*  mode       =  arg(arg1_g)
   ;  mcxTing*  ftxt       =  arg(arg2_g)
   ;  mcxTing*  yamtxt     =  NULL
   ;  double f, g =  0.0
   ;  long   i, j =  0
   ;  int modus = fType(mode)
   ;  mcxbool ok  =  TRUE
   ;  mcxbool integral = FALSE

;if(0)yamErr("f#2", "mode %s", mode)

   ;  if (yamDigest(ftxt, ftxt, seg))
      goto fail

   ;  f  =  atof(ftxt->str)
   ;  i  =  atoi(ftxt->str)

   ;  switch(modus)
      {  case F_FLOOR   : g = floor(f)          ;  break
      ;  case F_CEIL    : g = ceil(f)           ;  break
      ;  case F_ROUND   : g = floor(f+0.5)      ;  break
      ;  case F_ABS     : g = f < 0 ? -f : f    ;  break
      ;  case F_DRAND   : g = (((unsigned long) random()) * 1.0 / (1.0 * RAND_MAX)) * f 
                                                ;  break
      ;  case F_IRAND   : j = ((unsigned long) random()) % (unsigned) i, integral = TRUE
                                                ;  break
      ;  case F_SIGN    : j = f * MCX_SIGN(f) <= precision_g ? 0 : f<0 ? -1.0 : 1, integral = TRUE
                                                ;  break
      ;  case F_INC     : g = f + 1.0           ;  break
      ;  case F_DEC     : g = f - 1.0           ;  break
      ;  case F_NOT     : g = f ? 0.0 : 1.0     ;  break
      ;  default        :
         yamErr("\\f#2", "unknown mode <%s>", mode->str)
      ;  goto fail
   ;  }

      if (integral)
      yamtxt = mcxTingInteger(NULL, j)
   ;  else
      {  double eps = g - floor(g+0.5)
      ;  if (eps * MCX_SIGN(eps) <= precision_g && g*MCX_SIGN(g) <= LONG_MAX)   /* fixme */
         yamtxt = mcxTingInteger(NULL,  roundnear(g))
      ;  else
         yamtxt = mcxTingDouble(NULL, g, 10)
   ;  }

      if (0)
      fail: ok =  FALSE       /* fixme ugly goto, even harmful */

   ;  if (!ok)
      yamtxt = mcxTingEmpty(yamtxt, 0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&ftxt)
   ;  mcxTingFree(&mode)
   ;  return yamSegPush(seg, yamtxt)
;  }


yamSeg*  expandF3
(  yamSeg* seg
)
   {  mcxTing*  mode       =  arg(arg1_g)
   ;  mcxTing*  f1txt      =  arg(arg2_g)
   ;  mcxTing*  f2txt      =  arg(arg3_g)
   ;  mcxTing*  f3txt      =  NULL
   ;  double eps, f1 = 0.0, f2 = 0.0, f3 = 0.0
   ;  long i1 = 0, i2 = 0, done = 0
   ;  int modus = fType(mode)
   ;  mcxbool ok  =  TRUE

;if(0)yamErr("f#3", "mode %s", mode)
   ;  if (yamDigest(f1txt, f1txt, seg))
      goto fail

   ;  f1  =  atof(f1txt->str)
   ;  i1  =  atoi(f1txt->str)

   ;  if ((modus == F_AND && !i1) || (modus == F_OR && i1))
      {  f3 = i1 ? 1.0 : 0.0
      ;  done  =  1              /* short-circuit */
   ;  }
      else
      {  if (yamDigest(f2txt, f2txt, seg))
         goto fail
      ;  f2  =  atof(f2txt->str)
      ;  i2  =  atoi(f2txt->str)
   ;  }

   ;  if (done)
     /*  nothing */
   ;  else
      switch(modus)
      {  
         case F_ADD     :  f3 = f1 + f2                  ; break
      ;  case F_SUB     :  f3 = f1 - f2                  ; break
      ;  case F_FRAC    :  f3 = f2 ? (f1/f2) : 0         ; break
      ;  case F_DIV     :  f3 = f2 ? floor (f1/f2) : 0   ; break
      ;  case F_MUL     :  f3 = f1 * f2                  ; break
      ;  case F_POW     :  f3 = pow(fabs(f1), f2)        ; break

      ;  case F_AND     :  f3 = i1 && i2                 ; break
      ;  case F_OR      :  f3 = i1 || i2                 ; break

      ;  case F_MOD     :  f3 = f2 ? ((f1/f2)-floor(f1/f2))*f2 : 0  ; break
      ;  case F_MAX     :  f3 = MCX_MAX(f1, f2)              ; break
      ;  case F_MIN     :  f3 = MCX_MIN(f1, f2)              ; break

      ;  case F_CEIL    :  f3 = (ceil(f1/f2))*f2         ; break
      ;  case F_FLOOR   :  f3 = (floor(f1/f2))*f2        ; break
      ;  default        :
         yamErr("\\f#3", "unknown mode <%s>", mode->str)
      ;  goto fail
   ;  }

      if
      (  (modus == F_DIV || modus == F_FRAC || modus == F_MOD)
      && !f2
      )
      yamErr("\\f#3", "arithmetic exception for operator <%s>", mode->str)

   ;  eps = f3 - floor(f3+0.5)

   ;  if (eps * MCX_SIGN(eps) <= precision_g && f3*MCX_SIGN(f3) <= LONG_MAX)
      f3txt = mcxTingInteger(NULL,  roundnear(f3))
   ;  else
      f3txt = mcxTingDouble(NULL, f3, 10)

   ;  if(0)
      fail: ok = FALSE       /* fixme ugly goto, even harmful */

   ;  if (!ok)
      f3txt = mcxTingEmpty(f3txt, 0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&f1txt)
   ;  mcxTingFree(&f2txt)
   ;  mcxTingFree(&mode)

   ;  return yamSegPush(seg, f3txt)
;  }



yamSeg*  expandFv2
(  yamSeg* seg
)
   {  mcxTing*    mode        =  arg(arg1_g)
   ;  mcxTing*    data        =  arg(arg2_g)
   ;  long        ival        =  0
   ;  double      fval        =  0.0
   ;  yamSeg*     valseg      =  NULL
   ;  mcxTing*    valtxt      =  NULL
   ;  int         x           =  -1
   ;  int         modus       =  fType(mode)
   ;  int         ct          =  0
   ;  mcxbool     ok          =  TRUE

;if(0)yamErr("fv#2", "mode %s", mode)

   ;  if (modus == F_MIN)
      fval = DBL_MAX
   ;  else if (modus == F_MAX)
      fval = -DBL_MAX
   ;  else if (modus == F_MUL)
      fval = 1.0
   ;  else if (modus == F_AND)
      ival = 1
   ;  else if (modus == F_OR)
      ival = 0

   ;  valseg = yamStackPushTmp(data)

   ;  while (ok && (x = yamParseScopes(valseg, 1, 0)) == 1)
      {  long i
      ;  double f
      ;  mcxTing* op = arg(arg1_g)
      ;  if (yamDigest(op, op, seg))
         {  mcxTingFree(&op)
         ;  goto fail         /* fixme ugly goto */
      ;  }
         i = atoi(op->str)
      ;  f = atof(op->str)
      ;  mcxTingFree(&op)

      ;  switch(modus)
         {  case F_ADD     :  fval +=  f  ;  break
         ;  case F_MUL     :  fval *=  f  ;  break
         ;  case F_AND     :  ival = ival && i     ; break
         ;  case F_OR      :  ival = ival || i     ; break
         ;  case F_MAX     :  fval = MCX_MAX(fval, f)  ; break
         ;  case F_MIN     :  fval = MCX_MIN(fval, f)  ; break
         ;  default :
            yamErr("\\fv#2", "unknown mode <%s>", mode->str)
         ;  goto fail
      ;  }

         ct++
      ;  if ((modus == F_OR && ival) || (modus == F_AND && !ival))
         break                                  /* short-circuit */
   ;  }
      if (x < 0)
      {  goto fail
   ;  }
      else if (ct == 0)
      {  fval = 0.0
      ;  ival = 0
      ;  yamErr("\\fv#2", "I have an empty vararg!")
   ;  }

      if (!fval)
      valtxt = mcxTingInteger(NULL, ival)
   ;  else
      {  double eps = fval - floor(fval+0.5)
      ;  if (eps * MCX_SIGN(eps) <= precision_g && fval*MCX_SIGN(fval) <= LONG_MAX)
         valtxt = mcxTingInteger(NULL, roundnear(fval))
      ;  else
         valtxt = mcxTingDouble(NULL, fval, 10)
   ;  }

      if(0)
      fail: ok = FALSE

   ;  if (!ok)
      valtxt = mcxTingEmpty(valtxt, 0)

   ;  seg_check_ok(ok, seg)

   ;  yamStackFreeTmp(&valseg)
   ;  mcxTingFree(&data)
   ;  mcxTingFree(&mode)

   ;  return yamSegPush(seg, valtxt)
;  }



mcxstatus veto_redirect
(  const char* name
,  const char* me
)
   {  mcxTing* ask = mcxTingEmpty(NULL, 80)
   ;  mcxstatus status = STATUS_FAIL

   ;  do
      {  mcxTingPrint
         (  ask
         ,  "\n? do you allow writing to the file <%s> (y/n)\n? "
         ,  name
         )
      ;  if (ask_user(ask, me))
         break

      ;  status = STATUS_OK
   ;  }
      while (0)

   ;  mcxTingFree(&ask)
   ;  return status
;  }


yamSeg*  expandWriteto1 
(  yamSeg* seg
)
   {  mcxTing* newname     =  arg(arg1_g)
   ;  const char* curname  =  sinkGetDefaultName()  
   ;  const char* me       =  "\\writeto#3"
   ;  mcxIO* xf  
   ;  mcxbool ok = FALSE

   ;  do
      {  if (yamDigest(newname, newname, seg))
         break

      ;  if (strchr(newname->str, '/'))
         {  if (systemAccess == SYSTEM_SAFE)
            {  yamErr
               (  me
               ,  "filename contains path separator (cf --unsafe(-silent))"
               )
            ;  break
         ;  }
            else if
            (  systemAccess == SYSTEM_UNSAFE
            && veto_redirect(newname->str, me)
            )
            break
      ;  }
                           /* fixme, or not? pbb PBD */
         if (!curname)
         yamErrx(1, "\\redirect#1 PANIC", "no current file")

      ;  yamOutputClose(curname)

      ;  if (!(xf = yamOutputNew(newname->str)))
         yamErrx(1, "\\redirect#1", "unable to open file <%s>", newname->str)
                           /* fixme (no exit) */
      ;  mcxTell
         (  "zoem"
         ,  "changing current sink from <%s> to <%s>"
         ,  curname
         ,  newname->str
         )

      ;  sinkPush(xf->usr, -1)

      ;  ok = TRUE
   ;  }
      while (0)

   ;  seg_check_ok(ok, seg)
   ;  mcxTingFree(&newname)
   ;  return ok ? seg : yamSegPushEmpty(seg)
;  }


yamSeg*  expandPush1
(  yamSeg* seg
)
   {  return usrDictPush(arg1_g->str) ? NULL : seg
;  }


yamSeg*  expandPop1
(  yamSeg* seg
)
   {  return usrDictPop(arg1_g->str) ? NULL : seg
;  }


yamSeg*  expandEval1
(  yamSeg* seg
)
   {  mcxTing*  txt = arg(arg1_g)

   ;  yamDigest(txt, txt, seg)
   ;  BIT_OFF(seg->flags, SEGMENT_DONE)
            /* fixordoc: why not push empty in case of error? */
   ;  return yamSegPush(seg, txt)
;  }


yamSeg*  expandExit0
(  yamSeg* seg_unused cpl__unused
)
   {  yamErrx(1,"\\exit", "premature exit -- goodbye")
   ;  return NULL
;  }


yamSeg* expandTrace1
(  yamSeg*   seg
)
   {  mcxTing* t =  arg(arg1_g)
   ;  static int tracing_prev = 0
   ;  int val

   ;  if (yamDigest(t, t, seg))
      {  mcxTingFree(&t)
      ;  return seg
   ;  }

      val = atoi(t->str)

   ;  if (val == -3)
      tracing_prev = yamTracingSet(tracing_prev)
   ;  else if (val == -4)
      showTracebits()
   ;  else
      tracing_prev = yamTracingSet(val)

   ;  mcxTingFree(&t)
   ;  return   val == -4
            ?  seg
            :  yamSegPush(seg, mcxTingInteger(NULL, tracing_prev))
;  }


yamSeg* expandEnv4
(  yamSeg*   seg
)
   {  return yamEnvNew(arg1_g->str, arg2_g->str, arg3_g->str, arg4_g->str, seg) ? NULL : seg
;  }


yamSeg* expandBegin2
(  yamSeg*   seg
)
   {  const char* b = yamEnvOpen(arg1_g->str, arg2_g->str, seg)
   ;  mcxTing* val = b ? mcxTingNew(b) : NULL

   ;  if (!val)
      {  seg_check_ok(FALSE, seg)
      ;  return yamSegPushEmpty(seg)
   ;  }
      return yamSegPush(seg, val)
;  }


/* fixme; make clearer exit(1) logic, e.g. use yamSegPushEmpty. */

yamSeg* expandEnd1
(  yamSeg*   seg
)
   {  mcxTing* end      =  arg(arg1_g) 
   ;  const char* def   =  yamEnvEnd(end->str, seg)
   ;  mcxTing* val      =  def ? mcxTingNew(def) : NULL
   ;  mcxbool  ok       =  FALSE

   ;  do
      {  if (!val)
         {  yamErr("\\end#2", "env <%s> not found", end->str)
         ;  val = mcxTingEmpty(NULL, 0)
         ;  break
      ;  }
         if (yamDigest(val, val, seg))
         break
                        /* must eval now, because we need to pop here */
      ;  if (yamEnvClose(end->str))  /* this pops the env stack */
         break
      ;  ok = TRUE
   ;  }
      while (0)

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&end)
   ;  return yamSegPush(seg, val)
;  }


yamSeg* expandGet2
(  yamSeg*        seg
)
   {  mcxTing* val = yamKeyGetByDictName(arg1_g, arg2_g)
   ;  if (!val)
      {  yamErr("\\get#2", "dict/key <%s/%s> not found", arg1_g->str, arg2_g->str)
      ;  seg_check_ok(FALSE, seg)
      ;  return yamSegPushEmpty(seg)
   ;  }
      return yamSegPush(seg, arg(val))
;  }


yamSeg* expand_set_
(  yamSeg*        seg
,  const char*    me
,  mcxbits        callbits
,  const mcxTing* dict_name
)
   {  mcxbool     data     =  arg1_g->str[0] == '%'
   ;  mcxTing*    valtxt   =  arg(arg2_g)
   ;  mcxbool     ok       =  FALSE
   ;  mcxTing*    key      =  NULL
   ;  mcxTing*    access   =  NULL
   ;  int keylen

   ;  if (data)
      do
      {  mcxbool overwrite =  FALSE
      ;  access     =  mcxTingNew(arg1_g->str+1)
      ;  if (callbits & YAM_SET_EXPAND && yamDigest(valtxt, valtxt, seg))
         break
      ;  if (yamDigest(access, access, seg))
         break
      ;  if (yamOpsDataAccess(access))
         break
      ;  if (yamDataSet(valtxt->str, callbits, &overwrite, callbits & YAM_SET_COND))
         break
      ;  if (callbits & YAM_SET_WARN && overwrite)
         yamErr
         (me, "overwrite in access/data <%s><%s>", access->str, valtxt->str)
      ;  ok = TRUE
   ;  }
      while (0)
   ;  else
      do
      {  int namelen = 0
      ;  mcxbits keybits = 0
      ;  const mcxTing* inserted = NULL
      ;  key = arg(arg1_g)
      ;  keylen = checkusrsig(key->str, key->len, NULL, &namelen, &keybits)

      ;  if (keylen <= 0 || keylen != key->len)    /* mixed sign comparison */
         {  yamErr(me, "not a valid key signature: <%s>", key->str)
         ;  break
      ;  }
         else if (keybits & KEY_ZOEMONLY)
         {  yamErr(me, "attempt to use reserved syntax: <%s>", key->str)
         ;  break
      ;  }
         else if (mcxHashSearch(key, yamTable_g, MCX_DATUM_FIND))
         {  mcxErr(me, "zoem primitive <%s> is overwritten (not recommended)", key->str)
         ;  mcxErr(me, "primitive is still available as <'%s>", key->str)
      ;  }

         if (keybits & KEY_GLOBAL)
            callbits |= YAM_SET_GLOBAL    /* fixme-uglyish           */
         ,  mcxTingDelete(key, 0, 2)      /* get rid of leading ''   */

      ;  if (callbits & YAM_SET_EXPAND && yamDigest(valtxt, valtxt, seg))
         {  yamErr(me, "argument did not parse")
         ;  break
      ;  }

         inserted
         =  yamKeyInsert
            (  key
            ,  valtxt->str
            ,  valtxt->len
            ,  callbits & (YAM_SET_COND | YAM_SET_APPEND | YAM_SET_GLOBAL | YAM_SET_EXISTING)
            ,  dict_name
            )

      ;  if (inserted && inserted != key)
         {  if (callbits & YAM_SET_WARN)
            yamErr(me, "overwriting key <%s>",key->str)
         ;  mcxTingFree(&key)
      ;  }
                     /* !inserted may occur when resetting key (YAM_SET_EXISTING)
                      * but key was not found.
                     */
         ok =  TRUE
   ;  }
      while (0)

   ;  if (!ok)
      mcxTingFree(&key)  /* fixme better key-init-free logic */

   ;  seg_check_ok(ok, seg)

   ;  mcxTingFree(&access)
   ;  mcxTingFree(&valtxt)

   ;  return ok ? seg : yamSegPushEmpty(seg)
;  }


yamSeg* expandDef2
(  yamSeg*   seg
)
   {  return expand_set_(seg, "\\def#2", YAM_SET_WARN, NULL)
;  }


yamSeg* expandDefx2
(  yamSeg*  seg
)
   {  return expand_set_(seg, "\\defx#2", YAM_SET_EXPAND | YAM_SET_WARN, NULL)
;  }


static void my_splice
(  mcxTing* dest
,  mcxTing* val
,  mcxTing* oldval
,  int start
,  int width
)
   {  if (oldval)
      {  mcxTingNWrite(dest, oldval->str, oldval->len)
      ;  mcxTingSplice(dest, val->str, start, width, val->len)
   ;  }
      else
      mcxTingNWrite(dest, val->str, val->len)
;  }


yamSeg* expandSet3
(  yamSeg*   seg
)
   {  mcxTing* spec = arg(arg1_g)
   ;  mcxTing* key  = arg(arg2_g)
   ;  mcxTing* val  = arg(arg3_g)
   ;  mcxTing* condition = NULL
   ;  mcxTing* dict_name = NULL
   ;  mcxbool  unless = FALSE
   ;  yamSeg* tmpseg = yamStackPushTmp(spec)
   ;  mcxbits bits  = 0
   ;  mcxbool  vararg = FALSE
   ;  mcxTing* thestart = NULL, *thewidth = NULL
   ;  int y

   ;  while (2 == (y = yamParseScopes(tmpseg, 2, 0)))
      {  if (!strcmp(arg1_g->str, "modes"))
         {  if (strchr(arg2_g->str, 'u'))
            bits |= YAM_SET_UNARY
         ;  if (strchr(arg2_g->str, 'x'))
            bits |= YAM_SET_EXPAND
         ;  if (strchr(arg2_g->str, 'c'))
            bits |= YAM_SET_COND
         ;  if (strchr(arg2_g->str, 'e'))
            bits |= YAM_SET_EXISTING
         ;  if (strchr(arg2_g->str, 'w'))
            bits |= YAM_SET_WARN
         ;  if (strchr(arg2_g->str, 't'))
            bits |= YAM_SET_TREE
         ;  if (strchr(arg2_g->str, 'g'))
            bits |= YAM_SET_GLOBAL
         ;  if (strchr(arg2_g->str, 'l'))
            bits |= YAM_SET_LOCAL
         ;  if (strchr(arg2_g->str, 'a'))
            bits |= YAM_SET_APPEND
         ;  if (strchr(arg2_g->str, 'v'))
            vararg = TRUE
      ;  }
         else if (!strcmp(arg1_g->str, "start"))
         thestart = mcxTingNew(arg2_g->str)
      ;  else if (!strcmp(arg1_g->str, "width"))
         thewidth = mcxTingNew(arg2_g->str)
      ;  else if (!strcmp(arg1_g->str, "if"))
         condition = mcxTingNew(arg2_g->str)
      ;  else if (!strcmp(arg1_g->str, "unless"))
            condition = mcxTingNew(arg2_g->str)
         ,  unless = TRUE

      ;  else if (!strcmp(arg1_g->str, "dict"))
         dict_name = mcxTingNew(arg2_g->str)

      ;  else
         yamErr("\\set#3", "ignoring spec <%s>", arg1_g->str)
   ;  }

      yamStackFreeTmp(&tmpseg)
   ;  mcxTingFree(&spec) 

   ;  do
      {  if (condition)
         {  mcxbits flag_zoem = yamDigest(condition, condition, seg)
         ;  mcxbool condition_ok = atoi(condition->str)
         ;  if (flag_zoem)
            break
         ;  else if
            (  ( unless &&  condition_ok)
            || (!unless && !condition_ok)
            )
            break
      ;  }

         if (vararg)
         {  yamSeg* tmpseg = yamStackPushTmp(val)
         ;  while (!seg->flags && 2 == (y = yamParseScopes(tmpseg, 2, 0)))
            seg = expand_set_(seg, "\\set#3", bits, dict_name)
         ;  yamStackFreeTmp(&tmpseg)
      ;  }
         else
         {  if (thestart || thewidth)
            {  mcxTing* oldboy = yamKeyGet(key)
            ;  mcxbits flag_zoem = thestart ? yamDigest(thestart, thestart, seg) : 0
            ;  flag_zoem |= thewidth ? yamDigest(thewidth, thewidth, seg) : 0
            ;  if (flag_zoem)
               break
            ;  my_splice
               (  arg2_g
               ,  val
               ,  oldboy
               ,  thestart ? atoi(thestart->str) : 0
               ,  thewidth ? atoi(thewidth->str) : 0
               )
         ;  }
            else
            mcxTingNWrite(arg2_g, val->str, val->len)
         ;  mcxTingNWrite(arg1_g, key->str, key->len)
         ;  seg = expand_set_(seg, "\\set#3", bits, dict_name)
      ;  }
      }
      while (0)

   ;  mcxTingFree(&key) 
   ;  mcxTingFree(&val) 
   ;  mcxTingFree(&condition) 
   ;  mcxTingFree(&thestart) 
   ;  mcxTingFree(&thewidth) 
   ;  mcxTingFree(&dict_name) 

   ;  return seg->flags ? yamSegPushEmpty(seg) : seg
;  }


yamSeg* expandSet2
(  yamSeg*   seg
)
   {  return expand_set_(seg, "\\set#2", 0, NULL)
;  }


yamSeg* expandSetx2
(  yamSeg*  seg
)
   {  return expand_set_(seg, "\\setx#2", YAM_SET_EXPAND,NULL)
;  }


yamSeg* expandVanish1
(  yamSeg*   seg
)
   {  mcxTing* stuff     =  arg(arg1_g)
   ;  mcxbits flag_zoem  =  yamOutput(stuff, NULL, ZOEM_FILTER_NONE)

   ;  mcxTingFree(&stuff)
   ;  seg_check_flag(flag_zoem, seg)
   ;  return flag_zoem ? yamSegPushEmpty(seg) : seg
;  }


mcxenum let_cb
(  const char* token
,  long *ival
,  double *fval
)
   {  mcxTing* txt   =  mcxTingNew(token)
   ;  mcxstatus status_let =  TRM_FAIL
   ;  mcxstatus status_zoem = yamDigest(txt, txt, NULL)
   ;  int delta = (u8) txt->str[0] == '-' ? 1 : 0

   ;  if (status_zoem)
      {  mcxTingWrite(txt, "0")
      ;  status_let = TRM_FAIL
   ;  }
      else if (mcxStrChrAint(txt->str+delta, isdigit, txt->len-delta))
      {  *fval = atof(txt->str)
      ;  status_let = TRM_ISREAL
   ;  }
      else
      {  *ival = atol(txt->str)
      ;  status_let = TRM_ISNUM
      ;  *fval = *ival
   ;  }

      mcxTingFree(&txt)
   ;  return status_let
;  }



/* fixme; try to account for at scope as well [uh, seriously?]
 * (best shot should be possible simply by ignoring escape sequences).
*/

yamSeg* expandLength1
(  yamSeg*   seg
)
   {  mcxTing* stuff = arg(arg1_g)
   ;  char* p
   ;  int len = 0, c, mode = 1

   ;  if (yamDigest(stuff, stuff, seg))
      return yamSegPush(seg, mcxTingEmpty(stuff, 0))

   ;  p = stuff->str

   ;  while((c = (p++)[0]))
      len += (mode = (c == '\\' && mode ? 0 : 1))

   ;  return yamSegPush(seg, mcxTingPrint(stuff, "%d", len))
;  }


yamSeg* expandLet1
(  yamSeg*   seg
)
   {  telRaam* raam  =  trmInit(arg1_g->str)
   ;  long ival      =  0
   ;  double fval    =  0.0
   ;  mcxbool ok     =  FALSE
   ;  int  status    =  TRM_FAIL

   ;  trmRegister(raam, checkusrcall, let_cb, '\\')

   ;  do
      {  if (trmParse(raam))
         {  yamErr("\\let#1", "expression did not parse")
         ;  break
      ;  }

         if (tracing_g & ZOEM_TRACE_LET)
         trmDump(raam, "let")

      ;  status = trmEval(raam, &ival, &fval)

      ;  if (tracing_g & ZOEM_TRACE_LET)
         trmDump(raam, "let")

      ;  if (trmError(status))
         {  yamErr("\\let#1", "arithmetic error occurred")
         ;  break
      ;  }
         ok = TRUE
   ;  }
      while (0)

   ;  trmExit(raam)
   ;  seg_check_ok(ok, seg)

   ;  if (!ok)
      return yamSegPushEmpty(seg)
   ;  else if (trmIsNum(status))
      return yamSegPush(seg, mcxTingInteger(NULL, ival))
   ;  else if (trmIsReal(status))
      {  double fl = floor(fval+0.5)
      ;  long l = fl
      ;  if (fl-fval<0.0000001 && fval-fl<0.0000001)
         return yamSegPush(seg, mcxTingInteger(NULL, l))
      ;  else
         return yamSegPush(seg, mcxTingDouble(NULL, fval, 15))
   ;  }
      else     /* huh ?? what, how, whence ?? */
      { mcxErr("\\let#1", "weirdness happened in [%s]", seg->txt->str)
      ; return yamSegPush(seg, mcxTingInteger(NULL, 0))
   ;  }

      return NULL    /* unreachcode */
;  }


void yamOpsStats
(  void
)
   {  mcxHashStats(stdout, yamTable_g)
;  }


void mod_ops_exit
(  void
)
   {  if (yamTable_g)
      mcxHashFree(&yamTable_g, mcxTingRelease, NULL)
   ;  if (devtxt_g)
      mcxTingFree(&devtxt_g)
   ;  if (reg_end_g)
#if 0
      second argument is not yet functional there.
      mcxListFree(&reg_end_g, mcxTingFree_v)
#endif
      {  mcxLink* lk = reg_end_g->next
      ;  while (lk)
         {  mcxTing* tg = lk->val
         ;  mcxTingFree(&tg)
         ;  lk = lk->next
      ;  }
         mcxListFree(&reg_end_g, NULL)
   ;  }
;  }


void mod_ops_init
(  dim   n
)
   {  cmdHook* cmdhook     =  cmdHookDir
   ;  srandom(mcxSeed(2308947))

   ;  devtxt_g             =  mcxTingNew("__device__")
   ;  yamTable_g           =  yamHashNew(n)

   ;  while (cmdhook && cmdhook->name)
      {  mcxTing*  cmdtxt  =  mcxTingNew(cmdhook->name)
      ;  mcxKV*   kv       =  mcxHashSearch(cmdtxt,yamTable_g,MCX_DATUM_INSERT)
      ;  kv->val           =  cmdhook
      ;  cmdhook++
   ;  }

      reg_end_g            =  mcxListSource(10, 0)
;  }  


void mod_ops_digest
(  void
)
   {  mcxTing* t = mcxTingNew("\\'env{}{}{}{}")
   ;  yamDigest(t, t, NULL)
   ;  mcxTingFree(&t)
;  }


xpnfnc yamOpGet
(  mcxTing* txt
,  const char** compositepp
)
   {  mcxKV* kv = mcxHashSearch(txt, yamTable_g, MCX_DATUM_FIND)
   ;  if (kv)
      {  cmdHook* hk = kv->val
      ;  if (hk->yamfunc)
         return hk->yamfunc
      ;  *compositepp = hk->composite
   ;  }
      return NULL
;  }

