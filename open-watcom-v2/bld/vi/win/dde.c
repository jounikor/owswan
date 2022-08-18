/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2020 The Open Watcom Contributors. All Rights Reserved.
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


#include "vi.h"
#include <stddef.h>
#include "ddedef.h"
#include "wclbdde.h"


/* Local Windows CALLBACK function prototypes */
WINEXPORT HDDEDATA CALLBACK DDECallback( UINT type, UINT fmt, HCONV hconv,
                             HSZ topicstrh, HSZ itemstrh, HDDEDATA hmem,
                             ULONG_PTR data1, ULONG_PTR data2 );

typedef struct hsz_list {
    struct hsz_list     *next;
    struct hsz_list     *prev;
    HSZ                 hsz;
    char                string[1];
} hsz_list;
static hsz_list *hszHead, *hszTail;

HDDEDATA        DDERet;
DWORD           DDEInstId;
UINT            ClipboardFormat = CF_TEXT;
UINT            ServerCount;
bool            UseDDE;

static PFNCALLBACK  ddeproc = NULL;

/*
 * DDECallback - callback routine for DDE
 */
WINEXPORT HDDEDATA CALLBACK DDECallback( UINT type, UINT fmt, HCONV hconv,
                             HSZ topicstrh, HSZ itemstrh, HDDEDATA hmem,
                             ULONG_PTR data1, ULONG_PTR data2 )
{
    char        tmp[64];
    vi_rc       rc;

    fmt = fmt;
    data1 = data1;
    data2 = data2;

    switch( type ) {
    case XTYP_CONNECT:
    case XTYP_CONNECT_CONFIRM:
    case XTYP_DISCONNECT:
    case XTYP_REQUEST:
    case XTYP_POKE:
        MySprintf( tmp, "%u %U %U %U %U", type, (DWORD)hconv, (DWORD)topicstrh, (DWORD)itemstrh, (DWORD)hmem );
        rc = SourceHookWithData( SRC_HOOK_DDE, tmp );
        if( rc != ERR_NO_ERR ) {
            DDERet = DdeCreateDataHandle( DDEInstId, (LPBYTE)"err", 4, 0, itemstrh, fmt, 0 );
        } else {
            DDERet = DdeCreateDataHandle( DDEInstId, (LPBYTE)"ok", 3, 0, itemstrh, fmt, 0 );
        }
        return( DDERet );
    }
    return( (HDDEDATA)NULL );

} /* DDECallback */

/*
 * CreateStringHandle - build a kept string handle
 */
bool CreateStringHandle( const char *name, HSZ *hdl )
{
    hsz_list    *hlptr;
    int         len;
    HSZ         ohdl;
    char        buff[256];

    if( hdl == NULL ) {
        hdl = &ohdl;
    }
    len = strlen( name );
    if( len > 255 )
        len = 255;
    memcpy( buff, name, len );
    buff[len] = '\0';
    *hdl = DdeCreateStringHandle( DDEInstId, buff, 0 );
    if( *hdl == 0 ) {
        return( false );
    }
    if( !DdeKeepStringHandle( DDEInstId, *hdl ) ) {
        return( false );
    }
    hlptr = MemAlloc( offsetof( hsz_list, string ) + len + 1 );

    hlptr->hsz = *hdl;
    memcpy( hlptr->string, buff, len + 1 );
    AddLLItemAtEnd( (ss **)&hszHead, (ss **)&hszTail, (ss *)hlptr );
    return( true );

} /* CreateStringHandle */

/*
 * deleteStringData - delete a string handle from list
 */
static void deleteStringData( hsz_list *hlptr )
{
    DdeFreeStringHandle( DDEInstId, hlptr->hsz );
    DeleteLLItem( (ss **)&hszHead, (ss **)&hszTail, (ss *)hlptr );
    MemFree( hlptr );

} /* deleteStringData */

/*
 * DeleteStringHandle - delete a specific string handle
 */
void DeleteStringHandle( HSZ hdl )
{
    hsz_list    *hlptr;

    for( hlptr = hszHead; hlptr != NULL; hlptr = hlptr->next ) {
        if( hlptr->hsz == hdl ) {
            deleteStringData( hlptr );
            break;
        }
    }

} /* DeleteStringHandle */

/*
 * freeAllStringHandles - free all alocatted string handles
 */
static void freeAllStringHandles( void )
{
    hsz_list    *hlptr, *next;

    for( hlptr = hszHead; hlptr != NULL; hlptr = next ) {
        next = hlptr->next;
        deleteStringData( hlptr );
    }

} /* freeAllStringHandles */

/*
 * DDEInit - set up to do dde
 */
bool DDEInit( void )
{
    if( UseDDE ) {
        return( true );
    }

    ddeproc = MakeProcInstance_DDE( DDECallback, InstanceHandle );

    if( DdeInitialize( &DDEInstId, ddeproc, CBF_FAIL_EXECUTES |
                       CBF_FAIL_ADVISES | CBF_SKIP_REGISTRATIONS |
                       CBF_SKIP_UNREGISTRATIONS, 0L ) ) {
        FreeProcInstance_DDE( ddeproc );
        return( false );
    }

    UseDDE = true;
    return( true );

} /* DDEInit */

/*
 * DDEFini - clean up dde related things
 */
void DDEFini( void )
{
    if( !UseDDE ) {
        return;
    }
    if( ServerCount > 0 ) {
        DdeNameService( DDEInstId, 0, 0, DNS_UNREGISTER );
    }
    freeAllStringHandles();
    DdeUninitialize( DDEInstId );
    FreeProcInstance_DDE( ddeproc );
    UseDDE = false;

} /* DDEFini */
