/****************************************************************************
*
*                            Open Watcom Project
*
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


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbglit.h"
#include "namelist.h"
#include "dui.h"
#include "symcomp.h"

#include "clibext.h"

typedef struct sorted_names {
    struct sorted_names *next;
    mod_handle          mod;
    name_list           list;
    bool                code;
    bool                data;
    bool                d2_only;
    bool                dup_ok;
} sorted_names;

static sorted_names *SortedNames = NULL;

name_list *SymCompInit( bool code, bool data, bool d2_only, bool dup_ok, mod_handle mod )
{
    sorted_names        *curr;
    walk_find           wf;

    for( curr = SortedNames; curr != NULL; curr = curr->next ) {
        if( code != curr->code )
            continue;
        if( data != curr->data )
            continue;
        if( d2_only != curr->d2_only )
            continue;
        if( dup_ok != curr->dup_ok )
            continue;
        if( mod != curr->mod )
            continue;
        return( &curr->list );
    }
    _Alloc( curr, sizeof( *curr ) );
    wf = 0;
    if( code )
        wf |= WF_CODE;
    if( data )
        wf |= WF_DATA;
    NameListInit( &curr->list, wf );
    NameListAddModules( &curr->list, mod, d2_only, dup_ok );
    curr->next = SortedNames;
    SortedNames = curr;
    curr->code = code;
    curr->data = data;
    curr->d2_only = d2_only;
    curr->dup_ok = dup_ok;
    curr->mod = mod;
    return( &curr->list );
}


void SymCompFini( void )
{
    sorted_names        *curr, *next;
    for( curr = SortedNames; curr != NULL; curr = next ) {
        next = curr->next;
        NameListFree( &curr->list );
        _Free( curr );
    }
    SortedNames = NULL;
}

void SymCompMatches( name_list *list, char *match, unsigned *pfirst, unsigned *plast )
{
    unsigned            first,last;
    size_t              len;

    len = strlen( match );
    for( first = 0; first < NameListNumRows( list ); ++first ) {
        NameListName( list, first, TxtBuff, SNT_SOURCE );
        if( strnicmp( match, TxtBuff, len ) == 0 ) {
            break;
        }
    }
    for( last = first; last < NameListNumRows( list ); ++last ) {
        NameListName( list, last, TxtBuff, SNT_SOURCE );
        if( strnicmp( match, TxtBuff, len ) != 0 ) {
            break;
        }
    }
    *pfirst = first;
    *plast = last;
}
