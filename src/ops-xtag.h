/*   (C) Copyright 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_xtag_h
#define zoem_xtag_h

#include "segment.h"
#include "tingea/ting.h"

yamSeg* yamXtag
(  yamSeg*  seg
,  mcxTing* dev      /* if dev->len == 0, implicit closing tag */
,  mcxTing* arg
)  ;

void mod_xtag_exit
(  void
)  ;

void mod_xtag_init
(  void
)  ;

#endif


