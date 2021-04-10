/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_sink_h__
#define zoem_sink_h__

#include "filter.h"

#include "tingea/io.h"
#include "tingea/ting.h"
#include "tingea/types.h"
#include "util.h"
#include "dict.h"


typedef struct
{  filter*     fd
;  lblStack*   xmlStack
;  dictStack*  dlrStack
;  char*       fname
;
}  sink    ;

extern sink* sink_default_g;


/* we have a different struct because the same sink
 * may occur with multiple filters.
*/
typedef struct
{  sink*       sk
;
}  busysink  ;


void sinkSetDefault
(  sink*    sk
,  int      fltidx
)  ;

sink* sinkGet     /* todo: docme */
(  void
)  ;

keyDict* sinkGetDLRtop
(  void
)  ;

/*  The fallback trick does not yet work
 *  for stdia.
*/
keyDict* sinkGetDLRdefault
(  void
)  ;

dictStack* sinkGetDLR
(  void
)  ;

lblStack* sinkGetXML
(  void
)  ;

void sinkPush
(  sink* sk
,  int   fltidx
)  ;

void sinkPop
(  sink* sk
)  ;

sink* sinkNew
(  mcxIO* xf
)  ;

void sinkFree
(  sink* filed
)  ;

mcxIO* yamOutputNew
(  const char*  fname
)  ;

void yamOutputClose
(  const char*  fname
)  ;

void mod_sink_init
(  dim   n
)  ;

void mod_sink_exit
(  void
)  ;

const char* sinkGetDefaultName
(  void
)  ;



mcxstatus sinkDictPush
(  const char* name
)  ;

mcxstatus sinkDictPop
(  const char* name
)  ;

void sinkULimit
(  dim  size
)  ;


#endif

