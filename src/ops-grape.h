/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_grape_h
#define zoem_grape_h


#include "tingea/ting.h"
#include "tingea/types.h"

/*
 * assumes a vararg is stored in key_and_args_g.
 * str contains what must be stored. If val contains a vararg,
 * it is interpreted as a set of key/value pairs. If not,
 * it is interpreted as a single value.
*/

mcxstatus yamDataSet
(  const char* str
,  mcxbits  bits        /* YAM_SET_xxx */
,  mcxbool  *overwrite
,  mcxbool  cond
)  ;


/*
 * assumes a vararg is stored in key_and_args_g.
*/

const char* yamDataGet
(  void
)  ;


/*
 * assumes a vararg is stored in key_and_args_g.
*/

mcxstatus yamDataFree
(  void
)  ;


/*
 * assumes a vararg is stored in key_and_args_g.
*/

mcxstatus yamDataPrint
(  void
)  ;


void mod_grape_exit
(  void
)  ;

void mod_grape_init
(  void
)  ;


#endif


