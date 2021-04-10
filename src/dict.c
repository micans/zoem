/*   (C) Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007 Stijn van Dongen
 *
 * This file is part of Zoem. You can redistribute and/or modify Zoem under the
 * terms of the GNU General Public License;  either version 3 of the License or
 * (at your option) any later  version.  You should have received a copy of the
 * GPL along with Zoem, in the file COPYING.
*/

#include <string.h>
#include <stdio.h>               /* fileno too */
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include "dict.h"
#include "util.h"

#include "tingea/ting.h"
#include "tingea/types.h"
#include "tingea/alloc.h"
#include "tingea/hash.h"


void dictStackFree
(  dictStack** stackpp
,  const char* type
,  const char* fname
)
   {  dictStack* stack = *stackpp
   ;  keyDict* dict = stack->top
   ;  keyDict* next
            /* delta is because the bottom user dictionary is put
             * in place by zoem and is beyond control of the user
            */
   ;  while (dict)
      {  next = dict->down
      ;  if (dict->level > 1)
         yamErr
         (  "dictStackFree"
         ,  "%s dictionary <%s> not closed in output file <%s>"
         ,  type
         ,  dict->name->str
         ,  fname
         )
      ;  mcxHashFree(&dict->table, mcxTingRelease, mcxTingRelease)
      ;  mcxTingFree(&dict->name)
      ;  mcxFree(dict)
      ;  dict = next
   ;  }
      free(stack)
   ;  *stackpp = NULL
;  }


dictStack* dictStackNew
(  dim         dict_size
,  dim         n_dict_max
,  const char* name
)
   {  keyDict*    dict     =  mcxAlloc(sizeof(keyDict), EXIT_ON_FAIL)
   ;  dictStack*  stack    =  mcxAlloc(sizeof *stack, EXIT_ON_FAIL)
   ;  dict->down           =  NULL
   ;  dict->name           =  mcxTingNew(name)
   ;  dict->table          =  yamHashNew(dict_size)
   ;  mcxHashSetOpts(dict->table, 0.25, -1)

   ;  stack->top           =  dict
   ;  stack->bottom        =  dict
   ;  stack->n_dict        =  1
   ;  stack->N_dict        =  n_dict_max
   ;  dict->level          =  stack->n_dict
   ;  return stack
;  }


mcxstatus dictStackPush
(  dictStack* stack
,  int dict_size
,  const char* label
)
   {  keyDict* top
   ;  if (stack->n_dict+1 > stack->N_dict)
      {  yamErr
         (  "dictStackPush"
         ,  "no more than <%d> dicts allowed in stack"
         ,  stack->N_dict
         )
      ;  return STATUS_FAIL
   ;  }
      top = mcxAlloc(sizeof(keyDict), EXIT_ON_FAIL)
   ;  top->down   =  stack->top
   ;  top->table  =  yamHashNew(dict_size)
   ;  top->name   =  mcxTingNew(label)
   ;  top->level  =  stack->n_dict + 1
   ;  stack->top  =  top
   ;  stack->n_dict++
   ;  return STATUS_OK
;  }


mcxstatus dictStackPop
(  dictStack* stack
,  const char* type
,  const char* name
)
   {  keyDict* top = stack->top
   ;  if (!top->down)
      {  yamErr
         (  "dictStackPop"
         ,  "cannot pop last <%s> scope!"
         ,  type
         ,  top->name->str
         )
      ;  return STATUS_FAIL
   ;  }
      else if (strcmp(name, top->name->str))
      {  yamErr
         (  "dictStackPop"
         ,  "close request for <%s> while <%s> in scope"
         ,  name
         ,  top->name->str
         )
      ;  return STATUS_FAIL
   ;  }

      mcxHashFree(&(top->table), mcxTingRelease, mcxTingRelease)
   ;  mcxTingFree(&(top->name))
   ;  stack->top = top->down
   ;  mcxFree(top)
   ;  stack->n_dict--
   ;  return STATUS_OK
;  }

