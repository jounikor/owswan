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
#include "wrelist.h"
#include "wrenew.h"
#include "wredel.h"
#include "wrestat.h"
#include "wredde.h"
#include "wre.rh"
#include "wreimage.h"
#include "wreimg.h"
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
typedef struct WREImageInfo {
    char            *file_name;
    WResID          *res_name;
    WResLangType    lang;
    uint_16         MemFlags;
    bool            is32bit;
    size_t          data_size;
    void            *data;
} WREImageInfo;

typedef struct WREImageSession {
    HCONV           server;
    HCONV           client;
    uint_16         type;
    bool            new;
    WREImageInfo    info;
    WResTypeNode    *tnode;
    WResResNode     *rnode;
    WResLangNode    *lnode;
    WREResInfo      *rinfo;
} WREImageSession;

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static WREImageSession  *WREStartImageSession( WRESPT service, WRECurrentResInfo *curr, bool new );
static WREImageSession  *WREAllocImageSession( void );
static WREImageSession  *WREFindImageSession( HCONV conv );
static WREImageSession  *WREFindResImageSession( WREResInfo *rinfo );
static WREImageSession  *WREFindLangImageSession( WResLangNode *lnode );
static void             WRERemoveImageEditSession( WREImageSession *session );
static void             WREFreeEditSession( WREImageSession *session );
static void             WREDisconnectSession( WREImageSession *session );
static bool             WREAddImageToDir( WRECurrentResInfo *curr, uint_16 type );
static void             WREBringSessionToFront( WREImageSession *session );
static void             WREShowSession( WREImageSession *session, bool show );
static void             WREPokeImageCmd( WREImageSession *session, char *cmd, bool );

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/
static WREImageSession  *PendingSession = NULL;
static LIST             *WREImageSessions = NULL;
static uint_32          WRENumBitmapTitles = 0;
static uint_32          WRENumCursorTitles = 0;
static uint_32          WRENumIconTitles = 0;

static void DumpEmptyResource( WREImageSession *session )
{
    WRECurrentResInfo   curr;

    if( session->lnode->Info.Length == 0 ) {
        curr.info = session->rinfo;
        curr.type = session->tnode;
        curr.res = session->rnode;
        curr.lang = session->lnode;
        WRERemoveEmptyResource( &curr );
        WRESetStatusByID( 0, WRE_EMPTYREMOVED );
    }
}

WResID *WRECreateImageTitle( uint_16 type )
{
    char        *text;
    char        *title;
    uint_32     num;
    WResID      *name;

    if( type == RESOURCE2INT( RT_BITMAP ) ) {
        WRENumBitmapTitles++;
        num = WRENumBitmapTitles;
        text = AllocRCString( WRE_DEFBITMAPNAME );
    } else if( type == RESOURCE2INT( RT_GROUP_CURSOR ) ) {
        WRENumCursorTitles++;
        num = WRENumCursorTitles;
        text = AllocRCString( WRE_DEFCURSORNAME );
    } else if( type == RESOURCE2INT( RT_GROUP_ICON ) ) {
        WRENumIconTitles++;
        num = WRENumIconTitles;
        text = AllocRCString( WRE_DEFICONNAME );
    } else {
        return( NULL );
    }

    if( text != NULL ) {
        title = (char *)WRMemAlloc( strlen( text ) + 20 + 1 );
        if( title != NULL ) {
            title[0] = '\0';
            sprintf( title, text, num );
            name = WResIDFromStr( title );
            WRMemFree( title );
        }
        FreeRCString( text );
    }

    return( name );
}

bool WREAddImageToDir( WRECurrentResInfo *curr, uint_16 type )
{
    WResLangType        lang;
    bool                dup;
    int                 num_retries;
    WResID              *rname, *tname;
    bool                ok, tname_alloc;

    ok = true;
    tname_alloc = false;

    WREGetCurrentResource( curr );

    if( curr->info == NULL ) {
        curr->info = WRECreateNewResource( NULL );
        ok = (curr->info != NULL);
    }

    if( ok ) {
        if( curr->info->current_type == type ) {
            tname = &curr->type->Info.TypeName;
        } else {
            tname = WResIDFromNum( type );
            tname_alloc = (tname != NULL);
            ok = tname_alloc;
        }
    }

    if( ok ) {
        lang.lang = DEF_LANG;
        lang.sublang = DEF_SUBLANG;
        dup = true;
        num_retries = 0;
        rname = NULL;
        while( ok && dup && num_retries <= MAX_RETRIES ) {
            rname = WRECreateImageTitle( type );
            ok = (rname != NULL);
            if( ok ) {
                ok = WRENewResource( curr, tname, rname, DEF_MEMFLAGS, 0, 0,
                                     &lang, &dup, type, tname_alloc );
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

bool WRENewImageResource( WRESPT service, uint_16 type )
{
    WRECurrentResInfo  curr;
    bool               ok;

    ok = WREAddImageToDir( &curr, type );

    if( ok ) {
        ok = (WREStartImageSession( service, &curr, TRUE ) != NULL);
    }

    return( ok );
}

bool WREDumpPendingImageSession( void )
{
    bool                ret;

    ret = TRUE;

    if( PendingSession != NULL ) {
        DumpEmptyResource( PendingSession );
        WREFreeEditSession( PendingSession );
        PendingSession = NULL;
        WRESetPendingService( NoServicePending );
        WREDisplayErrorMsg( WRE_IMGSESSIONKILLED );
    }

    return( ret );
}

bool WREEndEditImageResource( HCONV conv )
{
    WREImageSession     *session;
    bool                ret;

    ret = FALSE;

    session = WREFindImageSession( conv );

    if( session != NULL ) {
        ret = TRUE;
        DumpEmptyResource( session );
        WRERemoveImageEditSession( session );
    }

    return( ret );
}

bool WRECommitImageSession( HCONV server, HCONV client )
{
    bool        ok;

    ok = (client != (HCONV)NULL && server != (HCONV)NULL && PendingSession != NULL);

    if( ok ) {
        WREInsertObject( &WREImageSessions, PendingSession );
        PendingSession->server = server;
        PendingSession->client = client;
    } else {
        WREFreeEditSession( PendingSession );
    }

    WRESetPendingService( NoServicePending );
    PendingSession = NULL;

    return( ok );
}

bool WREGetImageSessionFileName( HCONV server, void **data, size_t *size )
{
    WREImageSession *session;

    if( data == NULL || size == NULL ) {
        return( FALSE );
    }

    session = WREFindImageSession( server );
    if( session == NULL ) {
        return( FALSE );
    }

    *data = WREStrDup( session->info.file_name );
    if( *data != NULL ) {
        *size = strlen( *data ) + 1;
    }

    return( TRUE );
}

bool WREGetImageSessionResName( HCONV server, void **data, size_t *size )
{
    WREImageSession *session;

    if( data == NULL || size == NULL ) {
        return( FALSE );
    }

    session = WREFindImageSession( server );
    if( session == NULL ) {
        return( FALSE );
    }

    if( !WRWResID2Mem( session->info.res_name, data, size, session->info.is32bit ) ) {
        return( FALSE );
    }

    return( TRUE );
}

bool WREGetImageSessionData( HCONV server, void **data, size_t *size )
{
    WREImageSession     *session;
    size_t              tsize;

    if( data == NULL || size == NULL ) {
        return( FALSE );
    }

    session = WREFindImageSession( server );
    if( session == NULL ) {
        return( FALSE );
    }

    if( session->info.data == NULL || session->info.data_size == 0 ) {
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

    if( session->type == RESOURCE2INT( RT_BITMAP ) ) {
        if( !WREAddBitmapFileHeader( (BYTE **)data, size ) ) {
            if( *data != NULL ) {
                WRMemFree( *data );
            }
            return( FALSE );
        }
    }

    return( TRUE );
}

bool WRESetImageSessionResName( HCONV server, HDDEDATA hdata )
{
    WREImageSession     *session;
    WResID              *name;
    void                *data;
    uint_32             size;
    bool                ok;

    ok = (server != (HCONV)NULL && hdata != NULL);

    if( ok ) {
        session = WREFindImageSession( server );
        ok = (session != NULL);
    }

    if( ok ) {
        ok = WREHData2Mem( hdata, &data, &size );
    }

    if( ok ) {
        name = WRMem2WResID( data, FALSE );
        ok = (name != NULL);
    }

    if( ok ) {
        ok = WRERenameWResResNode( session->tnode, &session->rnode, name );
    }

    if( ok ) {
        WRESetResNamesFromType( session->rinfo, session->type, true, name, 0 );
    }

    if( data != NULL ) {
        WRMemFree( data );
    }

    if( name != NULL ) {
        WRMemFree( name );
    }

    return( ok );
}

static bool WRESetBitmapSessionResData( WREImageSession *session, void *data, size_t size )
{
    bool                ok;

    ok = (session != NULL);

    if( ok ) {
        WREStripBitmapFileHeader( (BYTE **)&data, &size );
        if( session->lnode->data != NULL ) {
            WRMemFree( session->lnode->data );
        }
        session->lnode->data = data;
        session->lnode->Info.Length = size;
        session->rinfo->modified = true;
    }

    return( ok );
}

static bool WRESetCursorSessionResData( WREImageSession *session, void *data, uint_32 size )
{
    WRECurrentResInfo   curr;
    bool                ok;

    ok = (session != NULL);

    if( ok ) {
        curr.info = session->rinfo;
        curr.type = session->tnode;
        curr.res = session->rnode;
        curr.lang = session->lnode;
        if( !session->new ) {
            ok = WREDeleteGroupImages( &curr, RESOURCE2INT( RT_GROUP_CURSOR ) );
        }
    }

    if( ok ) {
        session->rinfo->modified = true;
        ok = WRECreateCursorEntries( &curr, data, size );
    }

    return( ok );
}

static bool WRESetIconSessionResData( WREImageSession *session, void *data, uint_32 size )
{
    WRECurrentResInfo   curr;
    bool                ok;

    ok = (session != NULL);

    if( ok ) {
        curr.info = session->rinfo;
        curr.type = session->tnode;
        curr.res = session->rnode;
        curr.lang = session->lnode;
        if( !session->new ) {
            ok = WREDeleteGroupImages( &curr, RESOURCE2INT( RT_GROUP_ICON ) );
        }
    }

    if( ok ) {
        session->rinfo->modified = true;
        ok = WRECreateIconEntries( &curr, data, size );
    }

    return( ok );
}

bool WRESetImageSessionResData( HCONV server, HDDEDATA hdata )
{
    WREImageSession     *session;
    void                *data;
    uint_32             size;
    bool                ok;

    ok = (server != (HCONV)NULL && hdata != NULL);

    if( ok ) {
        session = WREFindImageSession( server );
        ok = (session != NULL);
    }

    if( ok ) {
        ok = WREHData2Mem( hdata, &data, &size );
    }

    if( ok ) {
        if( session->type == RESOURCE2INT( RT_BITMAP ) ) {
            ok = WRESetBitmapSessionResData( session, data, size );
        } else if( session->type == RESOURCE2INT( RT_GROUP_CURSOR ) ) {
            ok = WRESetCursorSessionResData( session, data, size );
        } else if( session->type == RESOURCE2INT( RT_GROUP_ICON ) ) {
            ok = WRESetIconSessionResData( session, data, size );
        }
    }

    return( ok );
}

WREImageSession *WREStartImageSession( WRESPT service, WRECurrentResInfo *curr, bool new )
{
    WREImageSession     *session;
    BYTE                *data;
    size_t              size;
    bool                ok;

    if( curr == NULL ) {
        return( NULL );
    }

    session = WREAllocImageSession();
    if( session == NULL ) {
        return( NULL );
    }

    session->new = new;
    session->info.data_size = 0;
    session->info.data = NULL;
    data = NULL;
    size = 0;
    ok = true;

    switch( service ) {
    case CursorService:
        if( !new && !WRECreateCursorDataFromGroup( curr, &data, &size ) ) {
            ok = false;
            break;
        }
        session->info.data_size = size;
        session->info.data = data;
        break;
    case IconService:
        if( !new && !WRECreateIconDataFromGroup( curr, &data, &size ) ) {
            ok = false;
            break;
        }
        session->info.data_size = size;
        session->info.data = data;
        break;
    case BitmapService:
        if( !new ) {
            session->info.data_size = curr->lang->Info.Length;
            session->info.data = curr->lang->data;
        }
        break;
    case NoServicePending:
    default:
        ok = false;
        break;
    }
    if( !ok ) {
        if( data != NULL ) {
            WRMemFree( data );
        }
        return( NULL );
    }

    session->info.file_name = WREStrDup( WREGetQueryName( curr->info ) );
    session->info.res_name = WRECopyWResID( &curr->res->Info.ResName );
    session->info.lang = curr->lang->Info.lang;
    session->info.MemFlags = curr->lang->Info.MemoryFlags;
    session->info.is32bit = curr->info->is32bit;

    session->type = curr->info->current_type;
    session->tnode = curr->type;
    session->rnode = curr->res;
    session->lnode = curr->lang;
    session->rinfo = curr->info;

    WRESetPendingService( service );

    PendingSession = session;

    if( WinExec( "wimgedit.exe -dde", SW_SHOW ) < 32 ) {
        WREDisplayErrorMsg( WRE_IMGEDITNOTSPAWNED );
        WREFreeEditSession( session );
        PendingSession = NULL;
        WRESetPendingService( NoServicePending );
    }

    return( session );
}

bool WREEditImageResource( WRECurrentResInfo *curr )
{
    WREImageSession     *session;
    WRESPT              service;
    bool                ok;

    ok = (curr != NULL && curr->lang != NULL);

    if( ok ) {
        session = WREFindLangImageSession( curr->lang );
        if( session != NULL ) {
            WREBringSessionToFront( session );
            return( TRUE );
        }
    }

    if( ok ) {
        if( curr->info->current_type == RESOURCE2INT( RT_BITMAP ) ) {
            service = BitmapService;
        } else if( curr->info->current_type == RESOURCE2INT( RT_GROUP_CURSOR ) ) {
            service = CursorService;
        } else if( curr->info->current_type == RESOURCE2INT( RT_GROUP_ICON ) ) {
            service = IconService;
        } else {
            ok = false;
        }
    }

    if( ok ) {
        if( curr->lang->data == NULL && curr->lang->Info.Length != 0 ) {
            curr->lang->data = WREGetCurrentResData( curr );
            ok = (curr->lang->data != NULL);
        }
    }

    if( ok ) {
        ok = (WREStartImageSession( service, curr, FALSE ) != NULL);
    }

    return( ok );
}

bool WREEndAllImageSessions( bool fatal_exit )
{
    WREImageSession     *session;
    LIST                *slist;
    bool                ok;

    _wre_touch( fatal_exit );

    ok = true;

    if( WREImageSessions != NULL ) {
        for( slist = WREImageSessions; ok && slist != NULL; slist = ListNext( slist ) ) {
            session = (WREImageSession *)ListElement( slist );
            if( session != NULL ) {
                WREDisconnectSession( session );
                WREFreeEditSession( session );
            }
        }
        if( ok ) {
            ListFree( WREImageSessions );
            WREImageSessions = NULL;
        }
    }

    return( ok );
}

void WREEndLangImageSession( WResLangNode *lnode )
{
    WREImageSession     *session;

    while( (session = WREFindLangImageSession( lnode )) != NULL ) {
        WREDisconnectSession( session );
        WRERemoveImageEditSession( session );
    }
}

void WREEndResImageSessions( WREResInfo *rinfo )
{
    WREImageSession     *session;

    while( (session = WREFindResImageSession( rinfo )) != NULL ) {
        WREDisconnectSession( session );
        WRERemoveImageEditSession( session );
    }
}

WREImageSession *WREAllocImageSession( void )
{
    WREImageSession *session;

    session = (WREImageSession *)WRMemAlloc( sizeof( WREImageSession ) );

    if( session != NULL ) {
        memset( session, 0, sizeof( WREImageSession ) );
    }

    return( session );
}

WREImageSession *WREFindImageSession( HCONV conv )
{
    WREImageSession *session;
    LIST            *slist;

    for( slist = WREImageSessions; slist != NULL; slist = ListNext( slist ) ) {
        session = (WREImageSession *)ListElement( slist );
        if( session->server == conv || session->client == conv ) {
            return( session );
        }
    }

    return( NULL );
}

WREImageSession *WREFindResImageSession( WREResInfo *rinfo )
{
    WREImageSession     *session;
    LIST                *slist;

    for( slist = WREImageSessions; slist != NULL; slist = ListNext( slist ) ) {
        session = (WREImageSession *)ListElement( slist );
        if( session->rinfo == rinfo ) {
            return( session );
        }
    }

    return( NULL );
}

WREImageSession *WREFindLangImageSession( WResLangNode *lnode )
{
    WREImageSession     *session;
    LIST                *slist;

    for( slist = WREImageSessions; slist != NULL; slist = ListNext( slist ) ) {
        session = (WREImageSession *)ListElement( slist );
        if( session->lnode == lnode ) {
            return( session );
        }
    }

    return( NULL );
}

void WRERemoveImageEditSession( WREImageSession *session )
{
    if( session != NULL ) {
        ListRemoveElt( &WREImageSessions, session );
        WREFreeEditSession( session );
    }
}

void WREFreeEditSession( WREImageSession *session )
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

void WREDisconnectSession( WREImageSession *session )
{
    if( session != NULL ) {
        WREPokeImageCmd( session, "endsession", TRUE );
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

void WREBringSessionToFront( WREImageSession *session )
{
    WREPokeImageCmd( session, "bringtofront", FALSE );
}

void WREShowAllImageSessions( bool show )
{
    WREImageSession     *session;
    LIST                *slist;

    if( WREImageSessions != NULL ) {
        for( slist = WREImageSessions; slist != NULL; slist = ListNext( slist ) ) {
            session = (WREImageSession *)ListElement( slist );
            if( session != NULL ) {
                WREShowSession( session, show );
            }
        }
    }
}

void WREShowSession( WREImageSession *session, bool show )
{
    if( show ) {
        WREPokeImageCmd( session, "show", FALSE );
    } else {
        WREPokeImageCmd( session, "hide", FALSE );
    }
}

void WREPokeImageCmd( WREImageSession *session, char *cmd, bool retry )
{
    if( session != NULL && cmd != NULL ) {
        WREPokeData( session->client, cmd, strlen( cmd ) + 1, retry );
    }
}
