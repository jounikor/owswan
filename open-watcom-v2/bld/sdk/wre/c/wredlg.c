/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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


#include "wreglbl.h"
#include <ddeml.h>
#include "wresall.h"
#include "wre_wres.h"
#include "wremsg.h"
#include "ldstr.h"
#include "wreres.h"
#include "wrestrdp.h"
#include "wregcres.h"
#include "wrenames.h"
#include "wrerenam.h"
#include "wrenew.h"
#include "wredel.h"
#include "wrestat.h"
#include "wrelist.h"
#include "wredde.h"
#include "wre.rh"
#include "wredlg.h"
#include "wresdefn.h"


/****************************************************************************/
/* macro definitions                                                        */
/****************************************************************************/
#define MAX_RETRIES 99

/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/****************************************************************************/
/* type definitions                                                         */
/****************************************************************************/
typedef struct WREDialogInfo {
    char            *file_name;
    WResID          *res_name;
    WResLangType    lang;
    uint_16         MemFlags;
    bool            is32bit;
    size_t          data_size;
    void            *data;
} WREDialogInfo;

typedef struct WREDialogSession {
    HCONV               server;
    HCONV               client;
    WREDialogInfo       info;
    WResTypeNode        *tnode;
    WResResNode         *rnode;
    WResLangNode        *lnode;
    WREResInfo          *rinfo;
} WREDialogSession;

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static WResID           *WRECreateDialogTitle( void );
static bool             WREAddDialogToDir( WRECurrentResInfo *curr );
static WREDialogSession *WREStartDialogSession( WRECurrentResInfo *curr );
static WREDialogSession *WREAllocDialogSession( void );
static WREDialogSession *WREFindDialogSession( HCONV conv );
static WREDialogSession *WREFindResDialogSession( WREResInfo *rinfo );
static WREDialogSession *WREFindLangDialogSession( WResLangNode *lnode );
static void             WRERemoveDialogEditSession( WREDialogSession *session );
static void             WREFreeEditSession( WREDialogSession *session );
static void             WREDisconnectSession( WREDialogSession *session );
static void             WREBringSessionToFront( WREDialogSession *session );
static void             WREShowSession( WREDialogSession *session, bool show );
static void             WREPokeDialogCmd( WREDialogSession *session, char *cmd, bool );

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static WREDialogSession *PendingSession = NULL;
static LIST             *WREDlgSessions = NULL;
static uint_32          WRENumDialogTitles = 0;

static void DumpEmptyResource( WREDialogSession *session )
{
    WRECurrentResInfo   curr;

    if( !session->lnode->Info.Length ) {
        curr.info = session->rinfo;
        curr.type = session->tnode;
        curr.res = session->rnode;
        curr.lang = session->lnode;
        WRERemoveEmptyResource( &curr );
        WRESetStatusByID( 0, WRE_EMPTYREMOVED );
    }
}

WResID *WRECreateDialogTitle( void )
{
    char        *text;
    char        *title;
    WResID      *name;

    WRENumDialogTitles++;

    name = NULL;
    text = AllocRCString( WRE_DEFDIALOGNAME );
    if( text != NULL ) {
        title = (char *)WRMemAlloc( strlen( text ) + 10 + 1 );
        if( title != NULL ) {
            title[0] = '\0';
            sprintf( title, text, WRENumDialogTitles );
            name = WResIDFromStr( title );
            WRMemFree( title );
        }
        FreeRCString( text );
    }

    return( name );
}

bool WREAddDialogToDir( WRECurrentResInfo *curr )
{
    WResLangType    lang;
    bool            dup;
    int             num_retries;
    WResID          *rname, *tname;
    bool            ok, tname_alloc;

    ok = true;
    tname_alloc = false;

    WREGetCurrentResource( curr );

    if( curr->info == NULL ) {
        curr->info = WRECreateNewResource( NULL );
        ok = (curr->info != NULL);
    }

    if( ok ) {
        if( curr->info->current_type == RESOURCE2INT( RT_DIALOG ) ) {
            tname = &curr->type->Info.TypeName;
        } else {
            tname = WResIDFromNum( RESOURCE2INT( RT_DIALOG ) );
            tname_alloc = true;
        }
        lang.lang = DEF_LANG;
        lang.sublang = DEF_SUBLANG;
    }

    if( ok ) {
        dup = true;
        num_retries = 0;
        rname = NULL;
        while( ok && dup && num_retries <= MAX_RETRIES ) {
            rname = WRECreateDialogTitle();
            ok = (rname != NULL);
            if( ok ) {
                ok = WRENewResource( curr, tname, rname, DEF_MEMFLAGS, 0, 0,
                                     &lang, &dup, RESOURCE2INT( RT_DIALOG ), tname_alloc );
                if( !ok && dup ) {
                    ok = true;
                }
                num_retries++;
            }
            if( rname != NULL ) {
                WRMemFree( rname );
            }
        }
        if( dup ) {
            WREDisplayErrorMsg( WRE_CANTFINDUNUSEDNAME );
        }
    }

    if( ok ) {
        curr->info->modified = true;
    }

    if( tname_alloc ) {
        WRMemFree( tname );
    }

    return( ok );
}

bool WRENewDialogResource( void )
{
    WRECurrentResInfo  curr;
    bool               ok;

    ok = WREAddDialogToDir( &curr );

    if( ok ) {
        ok = (WREStartDialogSession( &curr ) != NULL);
    }

    return( ok );
}

bool WREDumpPendingDialogSession( void )
{
    bool                ret;

    ret = TRUE;

    if( PendingSession != NULL ) {
        DumpEmptyResource( PendingSession );
        WREFreeEditSession( PendingSession );
        PendingSession = NULL;
        WRESetPendingService( NoServicePending );
        WREDisplayErrorMsg( WRE_DLGSESSIONKILLED );
    }

    return( ret );
}

bool WREEndEditDialogResource( HCONV conv )
{
    WREDialogSession    *session;
    bool                ret;

    ret = FALSE;

    session = WREFindDialogSession( conv );

    if( session ) {
        ret = TRUE;
        DumpEmptyResource( session );
        WRERemoveDialogEditSession( session );
    }

    return( ret );
}

bool WRECommitDialogSession( HCONV server, HCONV client )
{
    bool        ok;

    ok = (client != (HCONV)NULL && server != (HCONV)NULL && PendingSession != NULL);

    if( ok ) {
        WREInsertObject( &WREDlgSessions, PendingSession );
        PendingSession->server = server;
        PendingSession->client = client;
    } else {
        WREFreeEditSession( PendingSession );
    }

    WRESetPendingService( NoServicePending );
    PendingSession = NULL;

    return( ok );
}

bool WREGetDlgSessionFileName( HCONV server, void **data, size_t *size )
{
    WREDialogSession *session;

    if( data == NULL || size == NULL ) {
        return( FALSE );
    }

    session = WREFindDialogSession( server );
    if( session == NULL ) {
        return( FALSE );
    }

    *data = WREStrDup( session->info.file_name );
    if( *data != NULL ) {
        *size = strlen( *data ) + 1;
    }

    return( TRUE );
}

bool WREGetDlgSessionResName( HCONV server, void **data, size_t *size )
{
    WREDialogSession *session;

    if( data == NULL || size == NULL ) {
        return( FALSE );
    }

    session = WREFindDialogSession( server );
    if( session == NULL ) {
        return( FALSE );
    }

    if( !WRWResID2Mem( session->info.res_name, data, size, session->info.is32bit ) ) {
        return( FALSE );
    }

    return( TRUE );
}

bool WREGetDlgSessionIs32Bit( HCONV server, void **data, size_t *size )
{
    WREDialogSession *session;

    if( data == NULL || size == NULL ) {
        return( FALSE );
    }

    session = WREFindDialogSession( server );
    if( session == NULL ) {
        return( FALSE );
    }

    if( !session->info.is32bit ) {
        return( FALSE );
    }

    *size = sizeof( bool );
    *data = WRMemAlloc( *size );
    if( *data == NULL ) {
        return( FALSE );
    }
    memcpy( *data, &session->info.is32bit, *size );

    return( TRUE );
}

bool WREGetDlgSessionData( HCONV server, void **data, size_t *size )
{
    size_t  tsize;

    WREDialogSession *session;

    if( data == NULL || size == NULL ) {
        return( FALSE );
    }

    session = WREFindDialogSession( server );
    if( session == NULL ) {
        return( FALSE );
    }

    if( session->info.data == NULL  ) {
        *data = NULL;
        *size = 0;
        return( TRUE );
    }

    tsize = session->info.data_size;
    *size = tsize;
    *data = WRMemAlloc( tsize );
    if( *data == NULL ) {
        return( FALSE );
    }
    memcpy( *data, session->info.data, tsize );

    return( TRUE );
}

bool WRESetDlgSessionResName( HCONV server, HDDEDATA hdata )
{
    WREDialogSession    *session;
    WResID              *name;
    void                *data;
    uint_32             size;
    bool                ok;

    ok = (server != (HCONV)NULL && hdata != NULL);

    if( ok ) {
        session = WREFindDialogSession( server );
        ok = (session != NULL);
    }

    if( ok ) {
        ok = WREHData2Mem( hdata, &data, &size );
    }

    if( ok ) {
        name = WRMem2WResID( data, session->info.is32bit );
        ok = (name != NULL);
    }

    if( ok ) {
        ok = WRERenameWResResNode( session->tnode, &session->rnode, name );
    }

    if( ok ) {
        session->rinfo->modified = true;
        WRESetResNamesFromType( session->rinfo, RESOURCE2INT( RT_DIALOG ), true, name, 0 );
    }

    if( data != NULL ) {
        WRMemFree( data );
    }

    if( name != NULL ) {
        WRMemFree( name );
    }

    return( ok );
}

bool WRESetDlgSessionResData( HCONV server, HDDEDATA hdata )
{
    WREDialogSession    *session;
    void                *data;
    uint_32             size;
    bool                ok;

    ok = (server != (HCONV)NULL && hdata != NULL);

    if( ok ) {
        session = WREFindDialogSession( server );
        ok = (session != NULL);
    }

    if( ok ) {
        ok = WREHData2Mem( hdata, &data, &size );
    }

    if( ok ) {
        if( session->lnode->data != NULL ) {
            WRMemFree( session->lnode->data );
        }
        session->lnode->data = data;
        session->lnode->Info.Length = size;
        session->rinfo->modified = true;
    }

    return( ok );
}

WREDialogSession *WREStartDialogSession( WRECurrentResInfo *curr )
{
    WREDialogSession    *session;
    bool                is32bit;

    if( curr == NULL ) {
        return( NULL );
    }

    is32bit = curr->info->is32bit;

    session = WREAllocDialogSession();
    if( session == NULL ) {
        return( NULL );
    }

    session->info.file_name = WREStrDup( WREGetQueryName( curr->info ) );
    session->info.res_name = WRECopyWResID( &curr->res->Info.ResName );
    session->info.lang = curr->lang->Info.lang;
    session->info.MemFlags = curr->lang->Info.MemoryFlags;
    session->info.data_size = curr->lang->Info.Length;
    session->info.data = curr->lang->data;
    session->info.is32bit = is32bit;

    session->tnode = curr->type;
    session->rnode = curr->res;
    session->lnode = curr->lang;
    session->rinfo = curr->info;

    WRESetPendingService( DialogService );

    PendingSession = session;

    if( WinExec( "wde.exe -dde", SW_SHOW ) < 32 ) {
        WREDisplayErrorMsg( WRE_DLGEDITNOTSPAWNED );
        WREFreeEditSession( session );
        PendingSession = NULL;
        WRESetPendingService( NoServicePending );
    }

    return( session );
}

bool WREEditDialogResource( WRECurrentResInfo *curr )
{
    bool                ok;
    WREDialogSession    *session;

    ok = (curr != NULL && curr->lang != NULL);

    if( ok ) {
        session = WREFindLangDialogSession( curr->lang );
        if( session != NULL ) {
            WREBringSessionToFront( session );
            return( TRUE );
        }
    }

    if( ok ) {
        if( curr->lang->data == NULL && curr->lang->Info.Length != 0 ) {
            curr->lang->data = WREGetCurrentResData( curr );
            ok = (curr->lang->data != NULL);
        }
    }

    if( ok ) {
        ok = (WREStartDialogSession( curr ) != NULL);
    }

    return( ok );
}

bool WREEndAllDialogSessions( bool fatal_exit )
{
    WREDialogSession    *session;
    LIST                *slist;
    bool                ok;

    _wre_touch( fatal_exit );

    ok = true;

    if( WREDlgSessions != NULL ) {
        for( slist = WREDlgSessions; ok && slist != NULL; slist = ListNext( slist ) ) {
            session = (WREDialogSession *)ListElement( slist );
            if( session != NULL ) {
                WREDisconnectSession( session );
                WREFreeEditSession( session );
            }
        }
        if( ok ) {
            ListFree( WREDlgSessions );
            WREDlgSessions = NULL;
        }
    }

    return( ok );
}

void WREEndLangDialogSession( WResLangNode *lnode )
{
    WREDialogSession    *session;

    while( (session = WREFindLangDialogSession( lnode )) != NULL ) {
        WREDisconnectSession( session );
        WRERemoveDialogEditSession( session );
    }
}

void WREEndResDialogSessions( WREResInfo *rinfo )
{
    WREDialogSession    *session;

    while( (session = WREFindResDialogSession( rinfo )) != NULL ) {
        WREDisconnectSession( session );
        WRERemoveDialogEditSession( session );
    }
}

WREDialogSession *WREAllocDialogSession( void )
{
    WREDialogSession *session;

    session = (WREDialogSession *)WRMemAlloc( sizeof( WREDialogSession ) );

    if( session != NULL ) {
        memset( session, 0, sizeof( WREDialogSession ) );
    }

    return( session );
}

WREDialogSession *WREFindDialogSession( HCONV conv )
{
    WREDialogSession *session;
    LIST             *slist;

    for( slist = WREDlgSessions; slist != NULL; slist = ListNext( slist ) ) {
        session = (WREDialogSession *)ListElement( slist );
        if( session->server == conv || session->client == conv ) {
            return( session );
        }
    }

    return( NULL );
}

WREDialogSession *WREFindResDialogSession( WREResInfo *rinfo )
{
    WREDialogSession *session;
    LIST             *slist;

    for( slist = WREDlgSessions; slist != NULL; slist = ListNext( slist ) ) {
        session = (WREDialogSession *)ListElement( slist );
        if( session->rinfo == rinfo ) {
            return( session );
        }
    }

    return( NULL );
}

WREDialogSession *WREFindLangDialogSession( WResLangNode *lnode )
{
    WREDialogSession *session;
    LIST             *slist;

    for( slist = WREDlgSessions; slist != NULL; slist = ListNext( slist ) ) {
        session = (WREDialogSession *)ListElement( slist );
        if( session->lnode == lnode ) {
            return( session );
        }
    }

    return( NULL );
}

void WRERemoveDialogEditSession( WREDialogSession *session )
{
    if( session != NULL ) {
        ListRemoveElt( &WREDlgSessions, session );
        WREFreeEditSession( session );
    }
}

void WREFreeEditSession( WREDialogSession *session )
{
    if( session != NULL ) {
        if( session->info.file_name != NULL ) {
            WRMemFree( session->info.file_name );
        }
        if( session->info.res_name != NULL ) {
            WRMemFree( session->info.res_name );
        }
        WRMemFree( session );
    }
}

void WREDisconnectSession( WREDialogSession *session )
{
    if( session != NULL ) {
        WREPokeDialogCmd( session, "endsession", TRUE );
        DumpEmptyResource( session );
        if( session->server != (HCONV)NULL ) {
            DdeDisconnect( session->server );
            session->server = (HCONV)NULL;
        }
        if( session->client != (HCONV)NULL ) {
            DdeDisconnect( session->client );
            session->client = (HCONV)NULL;
        }
    }
}

void WREBringSessionToFront( WREDialogSession *session )
{
    WREPokeDialogCmd( session, "bringtofront", FALSE );
}

void WREShowAllDialogSessions( bool show )
{
    WREDialogSession    *session;
    LIST                *slist;

    if( WREDlgSessions ) {
        for( slist = WREDlgSessions; slist != NULL; slist = ListNext( slist ) ) {
            session = (WREDialogSession *)ListElement( slist );
            if( session != NULL ) {
                WREShowSession( session, show );
            }
        }
    }
}

void WREShowSession( WREDialogSession *session, bool show )
{
    if( show ) {
        WREPokeDialogCmd( session, "show", FALSE );
    } else {
        WREPokeDialogCmd( session, "hide", FALSE );
    }
}

void WREPokeDialogCmd( WREDialogSession *session, char *cmd, bool retry )
{
    if( session != NULL && cmd != NULL ) {
        WREPokeData( session->client, cmd, strlen( cmd ) + 1, retry );
    }
}
