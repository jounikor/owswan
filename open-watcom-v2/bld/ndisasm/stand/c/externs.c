/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <stdlib.h>
#include <string.h>
#include "dis.h"
#include "global.h"
#include "externs.h"
#include "buffer.h"
#include "memfuncs.h"
#include "print.h"

#include "clibext.h"


static int ref_compare( const void *entry1, const void *entry2 )
{
    int         ret_val;

    ret_val = stricmp( (*(const ref_entry *)entry1)->label->label.name, (*(const ref_entry *)entry2)->label->label.name );
    if( ret_val == 0 ) {
        if( (*(const ref_entry *)entry1)->offset < (*(const ref_entry *)entry2)->offset ) {
            ret_val = -1;
        } else if( (*(const ref_entry *)entry1)->offset > (*(const ref_entry *)entry2)->offset ) {
            ret_val = 1;
        }
    }
    return( ret_val );
}

externs CreateExterns( ref_list list ) {
    ref_entry           entry;
    int                 index = 0;
    externs             sec_externs;

    sec_externs = (externs) MemAlloc( sizeof( externs_struct ) );
    if( !sec_externs )
        return( NULL );
    memset( sec_externs, 0, sizeof( externs_struct ) );
    sec_externs->number = 0;
    for( entry = list->first; entry != NULL; entry = entry->next ) {
        if( ( entry->label->shnd == ORL_NULL_HANDLE ) && (entry->label->type != LTYP_GROUP) ) {
            sec_externs->number++;
        }
    }
    if( sec_externs->number > 0 ) {
        sec_externs->extern_refs = (ref_entry *) MemAlloc( sizeof( ref_entry ) * sec_externs->number );
        for( entry = list->first; entry != NULL; entry = entry->next ) {
            if( ( entry->label->shnd == ORL_NULL_HANDLE ) && ( entry->label->type != LTYP_GROUP ) ) {
                sec_externs->extern_refs[index] = entry;
                index++;
            }
        }
        qsort( sec_externs->extern_refs, sec_externs->number, sizeof( ref_entry * ), ref_compare );
    }
    return( sec_externs );
}

void FreeExterns( externs sec_externs )
{
    if( sec_externs->number && sec_externs->extern_refs ) {
        MemFree( sec_externs->extern_refs );
    }
    MemFree( sec_externs );
}

void PrintExterns( externs sec_externs )
{
    int                 loop;
    ref_entry           entry;
    label_entry         prev_label = NULL;

    BufferMsg( EXTERNAL_REFS );
    BufferConcatNL();
    BufferConcatNL();
    BufferMsg( SYMBOL );
    BufferConcatNL();
    BufferConcat( "-------" );
    BufferPrint();
    for( loop = 0; loop < sec_externs->number; loop++ ) {
        entry = sec_externs->extern_refs[loop];
        if( prev_label != entry->label ) {
            BufferConcatNL();
            BufferQuoteName( entry->label->label.name );
            BufferAlignToTab( ADDRESSES_POS );
            BufferPrint();
            prev_label = entry->label;
        }
        BufferHexU32( 4, entry->offset );
    }
    BufferConcatNL();
    BufferConcatNL();
    BufferPrint();
}
