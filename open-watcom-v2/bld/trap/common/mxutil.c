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


#include "trptypes.h"
#include "trpcomm.h"

trap_elen       In_Mx_Num;
trap_elen       Out_Mx_Num;
in_mx_entry_p   In_Mx_Ptr;
mx_entry_p      Out_Mx_Ptr;

void *GetInPtr( trap_elen pos )     /* Absolute position */
{
    trap_elen       entries_left;
    in_mx_entry_p   entry;

    entries_left = In_Mx_Num - 1;
    entry = In_Mx_Ptr;
    for( ;; ) {
        if( entries_left == 0 || pos < entry->len )
            break;
        pos -= entry->len;
        ++entry;
        --entries_left;
    }
    return( (unsigned_8 *)entry->ptr + pos );
}

void *GetOutPtr( trap_elen pos )    /* Absolute position */
{
    trap_elen       entries_left;
    mx_entry_p      entry;

    entries_left = Out_Mx_Num - 1;
    entry = Out_Mx_Ptr;
    for( ;; ) {
        if( entries_left == 0 || pos < entry->len )
            break;
        pos -= entry->len;
        ++entry;
        --entries_left;
    }
    return( (unsigned_8 *)entry->ptr + pos );
}

trap_elen GetTotalSizeIn( void )
{
    trap_elen       i;
    trap_elen       len;

    len = 0;
    for( i = 0; i < In_Mx_Num; i++ ) {
         len += In_Mx_Ptr[i].len;
    }
    return( len );
}

trap_elen GetTotalSizeOut( void )
{
    trap_elen       i;
    trap_elen       len;

    len = 0;
    for( i = 0; i < Out_Mx_Num; i++ ) {
         len += Out_Mx_Ptr[i].len;
    }
    return( len );
}
