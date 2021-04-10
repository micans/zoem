/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_util_h__
#define zoem_util_h__

#include <stdio.h>

#include "segment.h"

#include "tingea/hash.h"
#include "tingea/err.h"

void yamStats
(  void
)  ;

void  yamErrx
(  int status
,  const char  *caller
,  const char  *msg
,  ...
)  ;

void yamErr
(  const char  *caller
,  const char  *msg
,  ...
)  ;

void yamErrorFile
(  FILE* fp
)  ;

mcxHash* yamHashNew
(  dim n
)  ;

mcxstatus yamHashFie
(  const char* id
)  ;



typedef struct
{  mcxLink*    sp
;
}  lblStack    ;

lblStack* ting_stack_new
(  int capacity
)  ;

void ting_stack_init
(  lblStack* stack
,  int capacity
)  ;

void ting_stack_push
(  lblStack*   st
,  char*    str
,  int      len
)  ;

const char* ting_stack_pop
(  lblStack*   st
)  ;



void ting_tilde_expand
(  mcxTing* tg
,  mcxbits  modes
)  ;


#define YAM_TILDE_UNIX  1
#define YAM_TILDE_REGEX 2

void ting_stack_free
(  lblStack** stackp
)  ;


#define   YAM_SET_EXPAND   1 << 0         /*    x     */
#define   YAM_SET_LOCAL    1 << 1         /*    l     */
#define   YAM_SET_GLOBAL   1 << 2         /*    g     */
#define   YAM_SET_WARN     1 << 3         /*    w     */
#define   YAM_SET_COND     1 << 4         /*    c     */
#define   YAM_SET_UNARY    1 << 5         /*    u     */
#define   YAM_SET_TREE     1 << 6         /*    t     */
#define   YAM_SET_APPEND   1 << 7         /*    a     */
#define   YAM_SET_EXISTING 1 << 8         /*    e     */


#endif

