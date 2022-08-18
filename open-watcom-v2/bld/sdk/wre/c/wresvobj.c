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
#include "wremsg.h"
#include "wrewait.h"
#include "wrdll.h"
#include "wrselft.h"
#include "wreselft.h"
#include "wregetfn.h"
#include "wregcres.h"
#include "wredde.h"
#include "wreimage.h"
#include "wre.rh"
#include "wresvobj.h"


/****************************************************************************/
/* external function prototypes                                             */
/****************************************************************************/

/****************************************************************************/
/* static function prototypes                                               */
/****************************************************************************/
static bool SaveObjectAs( WRECurrentResInfo *, void * );
static bool SaveObjectInto( WRECurrentResInfo *, void * );

/****************************************************************************/
/* external variables                                                       */
/****************************************************************************/
extern char *WREResSaveIntoTitle;
extern char *WREResSaveAsTitle;
extern char *WREResFilter;

/****************************************************************************/
/* static variables                                                         */
/****************************************************************************/

bool SaveObject( bool save_into )
{
    WRECurrentResInfo   curr;
    void                *rdata;
    bool                ok;

    WRESetWaitCursor( TRUE );

    rdata = NULL;

    ok = WREGetCurrentResource( &curr );

    if( ok ) {
        ok = (curr.lang->Info.Length != 0);
        if( !ok ) {
            WREDisplayErrorMsg( WRE_UPDATEBEFORESAVE1 );
        }
    }

    if( ok ) {
        ok = ((rdata = WREGetCurrentResData( &curr )) != NULL);
    }

    if( ok ) {
        if( save_into ) {
            ok = SaveObjectInto( &curr, rdata );
        } else {
            ok = SaveObjectAs( &curr, rdata );
        }
    }

    if( rdata != NULL ) {
        WRMemFree( rdata );
    }

    WRESetWaitCursor( FALSE );

    return( ok );
}

bool SaveObjectAs( WRECurrentResInfo *curr, void *rdata )
{
    bool                ok;
    char                *fname;
    WRFileType          ftype;
    WREGetFileStruct    gf;
    WRSaveIntoData      idata;
    uint_32             size;

    fname = NULL;

    ok = (curr != NULL && rdata != NULL);

    if( ok ) {
        gf.file_name = NULL;
        gf.title = WREResSaveAsTitle;
        gf.filter = WREResFilter;
        gf.save_ext = TRUE;
        fname = WREGetSaveFileName( &gf );
        ok = (fname != NULL && *fname != '\0');
    }

    if( ok ) {
        ftype = WRESelectFileType( fname, curr->info->is32bit );
        ok = (ftype != WR_DONT_KNOW);
    }

    if( ok ) {
        size = curr->lang->Info.Length;
        idata.info = curr->info->info;
        idata.next = NULL;
        idata.type = &curr->type->Info.TypeName;
        idata.name = &curr->res->Info.ResName;
        idata.data = rdata;
        idata.lang = curr->lang->Info.lang;
        idata.size = size;
        idata.MemFlags = curr->lang->Info.MemoryFlags;
        ok = WRSaveObjectAs( fname, ftype, &idata );
    }

    if( fname != NULL ) {
        WRMemFree( fname );
    }

    return( ok );
}

bool SaveObjectInto( WRECurrentResInfo *curr, void *rdata )
{
    bool                ok;
    char                *fname;
    WREGetFileStruct    gf;
    WRSaveIntoData      idata;
    bool                dup;
    uint_32             size;

    fname = NULL;
    dup = false;

    ok = (curr != NULL && rdata != NULL);

    if( ok ) {
        gf.file_name = NULL;
        gf.title = WREResSaveIntoTitle;
        gf.filter = WREResFilter;
        gf.save_ext = TRUE;
        fname = WREGetOpenFileName( &gf );
        ok = (fname != NULL && *fname != '\0');
    }

    if( ok ) {
        size = curr->lang->Info.Length;
        idata.next = NULL;
        idata.info = curr->info->info;
        idata.type = &curr->type->Info.TypeName;
        idata.name = &curr->res->Info.ResName;
        idata.data = rdata;
        idata.lang = curr->lang->Info.lang;
        idata.size = size;
        idata.MemFlags = curr->lang->Info.MemoryFlags;
        ok = WRSaveObjectInto( fname, &idata, &dup ) && !dup;
    }

    if( dup ) {
        WREDisplayErrorMsg( WRE_DUPRESNAME );
    }

    if( fname != NULL ) {
        WRMemFree( fname );
    }

    return( ok );
}
