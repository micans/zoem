/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_digest_h__
#define zoem_digest_h__

#include "sink.h"

#include "tingea/ting.h"
#include "tingea/types.h"


/*
 * An output file is fully described by its yamFileData descriptor.
 * This descriptor is stored in the 'usr' member of an mcxIO descriptor.
*/


/*
 * returns same as yamDigest
*/

mcxbits yamOutput
(  mcxTing        *txtin
,  sink*          sd
,  int            fltidx
)  ;


/*
 * yamDigest only expands and does not filter.  txtout can be the same as
 * txtin; in that case, txtin is overwritten with its expanded image.
 *
 * returns 0, SEGMENT_ERROR, SEGMENT_THROW, or SEGMENT_DONE
 *
*/

mcxbits  yamDigest
(  mcxTing   *txtin
,  mcxTing   *txtout
,  yamSeg    *baseseg
)  ;

#endif

