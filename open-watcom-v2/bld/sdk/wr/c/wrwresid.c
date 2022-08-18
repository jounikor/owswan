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


#include "wrglbl.h"
#include "wresall.h"

/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/

static WResID *WR16Mem2WResID( void *data )
{
    WResID      *new;
    WResID      *tmpName;
    int         stringlen;
    int         len;

    if( data == NULL ) {
        return( NULL );
    }

    tmpName = (WResID *)data;

    len = sizeof( WResID );
    stringlen = 0;
    if( tmpName->IsName ) {
        stringlen = tmpName->ID.Name.NumChars;
        len += stringlen;
        if( stringlen > 1 ) {
            len--;
        }
    }

    new = (WResID *)MemAlloc( len );
    if( new == NULL ) {
        return( NULL );
    }

    if( tmpName->IsName ) {
        new->IsName = true;
        new->ID.Name.NumChars = stringlen;
        new->ID.Name.Name[0] = '\0';
        memcpy( &new->ID.Name.Name[0], &tmpName->ID.Name.Name[0], stringlen );
    } else {
        memcpy( new, data, sizeof( WResID ) );
    }

    return( new );
}

static WResID *WR32Mem2WResID( void *data )
{
    WResID      *new;
    WResID      *tmpName;
    char        *uni_str;
    char        *str;
    size_t      str_len;
    size_t      stringlen;
    size_t      len;

    if( data == NULL ) {
        return( NULL );
    }

    uni_str = NULL;
    str = NULL;
    str_len = 0;
    stringlen = 0;

    tmpName = (WResID *)data;

    len = sizeof( WResID );
    if( tmpName->IsName ) {
        stringlen = tmpName->ID.Name.NumChars;
        uni_str = (char *)MemAlloc( stringlen * 2 + 2 );
        if( uni_str == NULL ) {
            return( NULL );
        }
        memcpy( uni_str, &tmpName->ID.Name.Name[0], stringlen * 2 );
        uni_str[stringlen * 2] = '\0';
        uni_str[stringlen * 2 + 1] = '\0';
        WRunicode2mbcs( uni_str, &str, &str_len );
        MemFree( uni_str );
        if( str == NULL ) {
            return( NULL );
        }
        len += str_len;
        if( str_len > 1 ) {
            len--;
        }
    }

    new = (WResID *)MemAlloc( len );
    if( new == NULL ) {
        return( NULL );
    }

    if( tmpName->IsName ) {
        new->IsName = true;
        new->ID.Name.NumChars = str_len - 1;
        new->ID.Name.Name[0] = '\0';
        memcpy( &new->ID.Name.Name[0], str, str_len - 1 );
    } else {
        memcpy( new, data, sizeof( WResID ) );
    }

    if( str != NULL ) {
        MemFree( str );
    }

    return( new );
}

static bool WRWResID2Mem16( WResID *name, void **data, size_t *size )
{
    size_t      len;

    if( name == NULL || data == NULL || size == NULL ) {
        return( false );
    }

    len = sizeof( WResID );
    if( name->IsName ) {
        len += name->ID.Name.NumChars;
        if( name->ID.Name.NumChars > 1 ) {
            len--;
        }
    }

    *data = MemAlloc( len );
    if( *data == NULL ) {
        return( false );
    }

    memcpy( *data, name, len );
    *size = len;

    return( true );
}

static bool WRWResID2Mem32( WResID *name, void **data, size_t *size )
{
    WResID      *tmpName;
    char        *str;
    char        *uni_str;
    size_t      uni_len;
    size_t      len;

    if( name == NULL || data == NULL || size == NULL ) {
        return( false );
    }

    uni_str = NULL;
    uni_len = 0;
    len = sizeof( WResID );
    if( name->IsName ) {
        str = WResIDToStr( name );
        if( str != NULL ) {
            WRmbcs2unicode( str, &uni_str, &uni_len );
            MemFree( str );
        }
        if( uni_str == NULL ) {
            return( false );
        }
        uni_len -= 2;  // get rid of the null terminator
        len = len + uni_len;
        if( uni_len > 0 ) {
            len--;
        }
    }

    *data = MemAlloc( len );
    if( *data == NULL ) {
        if( uni_str != NULL ) {
            MemFree( uni_str );
        }
        return( false );
    }

    *size = len;

    if( name->IsName ) {
        tmpName = (WResID *)*data;
        tmpName->IsName = true;
        tmpName->ID.Name.NumChars = name->ID.Name.NumChars;
        tmpName->ID.Name.Name[0] = '\0';
        memcpy( &tmpName->ID.Name.Name[0], uni_str, uni_len );
        MemFree( uni_str );
    } else {
        memcpy( *data, name, len );
    }

    return( true );
}

bool WRAPI WRWResID2Mem( WResID *name, void **data, size_t *size, bool is32bit )
{
    if( is32bit ) {
        return( WRWResID2Mem32( name, data, size ) );
    } else {
        return( WRWResID2Mem16( name, data, size ) );
    }
}

WResID * WRAPI WRMem2WResID( void *data, bool is32bit )
{
    WResID      *r_id;

    if( is32bit ) {
        r_id = WR32Mem2WResID( data );
    } else {
        r_id = WR16Mem2WResID( data );
    }

    return( r_id );
}
