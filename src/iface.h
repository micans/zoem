/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#ifndef zoem_iface_h__
#define zoem_iface_h__


void showTracebits
(  void
)  ;

extern int ENTRY_EXXIT   ;
extern int ENTRY_SPLIT   ;
extern int ENTRY_DEBUG   ;
extern int ENTRY_STDIA   ;
extern int ENTRY_STDIN   ;
extern int ENTRY_STDOUT  ;


extern int SYSTEM_SAFE        ;
extern int SYSTEM_UNSAFE      ;
extern int SYSTEM_UNSAFE_SILENT;

extern mcxTing* system_allow;

extern int ZOEM_TRACE_PREVIOUS;
extern int ZOEM_TRACE_DEFAULT ;
extern int ZOEM_TRACE_ALL     ;
extern int ZOEM_TRACE_ALL_LONG;

extern int ZOEM_TRACE_LONG    ;
extern int ZOEM_TRACE_KEYS    ;
extern int ZOEM_TRACE_ARGS    ;
extern int ZOEM_TRACE_DEFS    ;
extern int ZOEM_TRACE_SCOPES  ;
extern int ZOEM_TRACE_SEGS    ;
extern int ZOEM_TRACE_OUTPUT  ;
extern int ZOEM_TRACE_REGEX   ;
extern int ZOEM_TRACE_HASH    ;
extern int ZOEM_TRACE_SYSTEM  ;
extern int ZOEM_TRACE_LET     ;
extern int ZOEM_TRACE_IO      ;
extern int ZOEM_TRACE_PRIME   ;

extern int stressWrite        ;
extern int systemAccess       ;
extern int systemHonor        ;
extern int ztablength_g       ;
extern int readlineOK         ;
extern size_t chunk_size      ;

void mod_iface_exit(void);

#endif

