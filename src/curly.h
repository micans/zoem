/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_curly_h__
#define zoem_curly_h__

#include "segment.h"

#include "tingea/ting.h"
#include "tingea/types.h"

#define  CURLY_NOLEFT       -1
#define  CURLY_NORIGHT      -2

/*
 *    txt->str[offset] must be '{', returns CURLY_NOLEFT if not.
 *    otherwise returns l such that txt->str[offset+l] is matching '}',
 *    or CURLY_NORIGHT if the latter does not exist.
 *    Writes numbers of newlines in linect if not NULL.
*/

int yamClosingCurly
(  const mcxTing* txt
,  int            offset
,  int*           linect
,  mcxOnFail      ON_FAIL
)  ;


#define  CUBE_NOLEQ       -1
#define  CUBE_NOGEQ       -2
#define  CUBE_NEST        -3

/*
 *    txt->str[offset] must be '<', returns CUBE_NOLEQ if not.
 *    otherwise returns l such that txt->str[offset+l] is matching '>',
 *    or CUBE_NOGEQ if the latter does not exist.
 *    Writes numbers of newlines in linect if not NULL.
*/

int yamClosingCube
(  const mcxTing     *txt
,  int         offset
,  int*        linect
,  mcxOnFail   ON_FAIL
)  ;


/* '*' at offset, returns length s.t. txt->str[offset+length] is last
 * char of constant
*/

#define  CONSTANT_NOLEFT    -1
#define  CONSTANT_NORIGHT   -2
#define  CONSTANT_ILLCHAR   -3

int yamEOConstant
(  mcxTing     *txt
,  int         offset
)  ;


mcxTing* yamBlock
(  const mcxTing* txt
,  int            offset
,  int*           length
,  mcxTing*       dst
)  ;


void yamScopeErr
(  yamSeg      *seg
,  const char  *caller
,  int         error
)  ;

#endif

