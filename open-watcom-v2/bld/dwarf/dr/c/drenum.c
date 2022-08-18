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
* Description:  DWARF enumeration debug info processing.
*
****************************************************************************/


#include "drpriv.h"
#include "drutils.h"
#include "drenum.h"

typedef struct {
    enumCallback    callback;
    void            *data;
} enum_cb_info;

static bool ReadEnumerator( drmem_hdl abbrev, drmem_hdl mod, void *inf )
/**********************************************************************/
{
    unsigned_32         val;
    char                *name;
    enum_cb_info        *info;

    name = DWRGetName( abbrev, mod );
    if( name == NULL ) {
        DWREXCEPT( DREXCEP_DWARF_LIB_FAIL );
    }
    if( !DWRScanForAttrib( &abbrev, &mod, DW_AT_const_value ) ) {
        DWREXCEPT( DREXCEP_DWARF_LIB_FAIL );
    }
    val = DWRReadConstant( abbrev, mod );
    info = (enum_cb_info *)inf;
    return( info->callback( name, val, info->data ) );
}

void DRLoadEnum( drmem_hdl entry, void * data, enumCallback callback )
/********************************************************************/
{
    enum_cb_info    info;
    drmem_hdl       abbrev;
    dw_tagnum       tag;

    if( DWRReadTagEnd( &entry, &abbrev, &tag ) ) {
        DWREXCEPT( DREXCEP_DWARF_LIB_FAIL );
    }
    if( tag != DW_TAG_enumeration_type ) {
        DWREXCEPT( DREXCEP_DWARF_LIB_FAIL );
    }
    abbrev++;   /* skip child flag */
    DWRSkipAttribs( abbrev, &entry );

    info.callback = callback;
    info.data = data;
    DWRAllChildren( entry, ReadEnumerator, &info );
}
