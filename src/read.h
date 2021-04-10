/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_read_h__
#define zoem_read_h__

#include "tingea/io.h"
#include "tingea/hash.h"

#define READ_INLINE 1

mcxTing*  yamReadData
(  mcxIO          *xf
,  mcxTing        *filetxt
)  ;

mcxTing* yamProtect
(  mcxTing* data
)  ;
void  yamUnprotect
(  mcxTing* data
)  ;

mcxTing* yamSystem
(  char* cmd
,  char* argv[]
,  mcxTing* data
)  ;


mcxIO* yamTryOpen
(  mcxTing* fname
,  const mcxTing** inline_txt_ptr
,  mcxbool useSearchPath
)  ;


mcxstatus  yamReadChunk
(  mcxIO          *xf               /* must be open! */
,  mcxTing        *filetxt
,  mcxbits        flags             /* accepts READ_MASKING */
,  int            chunk_size        /* if 0, try to read everything */
,  long           *n_lines_read
,  void           (*lcc_update)(long n_deleted, long n_lines, void* data)
,  void*          data
)  ;


void mod_read_init
(  int n
)  ;

void mod_read_exit
(  void
)  ;

#endif

