/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  SCM interface library implementation.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined( __WINDOWS__ ) || defined( __NT__ )
    #include <windows.h>
#elif defined( __OS2__ )
    #define INCL_WINSHELLDATA
#endif
#include "wio.h"
#include "bool.h"
#include "rcsdll.hpp"
#include "inifile.hpp"
#include "pathgrp2.h"

#include "clibext.h"


#if defined( __WINDOWS__ ) || defined( __NT__ )
    static      HINSTANCE   hInstance = 0;
#endif

extern "C" {

/* common functions */
RCSDLLENTRY extern RCSGetVersionFn  RCSGetVersion;
RCSDLLENTRY extern RCSSetSystemFn   RCSSetSystem;
RCSDLLENTRY extern RCSQuerySystemFn RCSQuerySystem;
RCSDLLENTRY extern RCSRegBatchCbFn  RCSRegisterBatchCallback;
RCSDLLENTRY extern RCSRegMsgBoxCbFn RCSRegisterMessageBoxCallback;
/* system specific functions -- mapped to function for appropriate system */
RCSDLLENTRY extern RCSInitFn        RCSInit;
RCSDLLENTRY extern RCSCheckoutFn    RCSCheckout;
RCSDLLENTRY extern RCSCheckinFn     RCSCheckin;
RCSDLLENTRY extern RCSHasShellFn    RCSHasShell;
RCSDLLENTRY extern RCSRunShellFn    RCSRunShell;
RCSDLLENTRY extern RCSFiniFn        RCSFini;
RCSDLLENTRY extern RCSSetPauseFn    RCSSetPause;

};  // extern "C"

mksRcsSystem    MksRcs;
pvcsSystem      Pvcs;
genericRcs      Generic;
p4System        Perforce;
gitSystem       Git;
wprojRcs        Wproj;

extern "C" {

static const char *rcs_type_strings[] = {
    #define pick1(a,b,c,d)      b,
    #define pick2(a,b,c,d)      b,
    #include "rcssyst.h"
    #undef pick1
    #undef pick2
};

static const char *pause_strings[] = {
    "no_pause",
    "pause"
};


static rcsSystem *rcs_systems[] = {
    #define pick1(a,b,c,d)      c,
  #if defined( __WINDOWS__ ) || defined( __NT__ )
    #define pick2(a,b,c,d)      c,
  #else
    #define pick2(a,b,c,d)      NULL,
  #endif
    #include "rcssyst.h"
    #undef pick1
    #undef pick2
};

rcsdata RCSAPI RCSInit( rcshwnd window, char *cfg_dir )
{
    userData *data;

    data = new userData( window, cfg_dir );
    data->setSystem( RCSQuerySystem( data ) ); /* sets var. & calls init */
    return( (rcsdata)data );
}

void RCSAPI RCSFini( rcsdata data )
{
    userData *d = (userData*)data;
    if( d == NULL )
        return;
    if( d->getSystem() != NULL ) {
        d->getSystem()->fini();
    }
    delete( d );
}

int RCSAPI RCSCheckout( rcsdata data, rcsstring name, rcsstring pj, rcsstring tgt )
{
    userData *d = (userData*)data;
    if( d == NULL || d->getSystem() == NULL )
        return( 0 );
    return( d->getSystem()->checkout( d, name, pj, tgt ) );
}

int RCSAPI RCSCheckin( rcsdata data, rcsstring name, rcsstring pj, rcsstring tgt )
{
    userData *d = (userData*)data;
    if( d == NULL || d->getSystem() == NULL )
        return( 0 );
    return( d->getSystem()->checkin( d, name, pj, tgt ) );
}
int RCSAPI RCSHasShell( rcsdata data )
{
    userData *d = (userData*)data;
    if( d == NULL || d->getSystem() == NULL )
        return( 0 );
    return( d->getSystem()->hasShell() );
}
int RCSAPI RCSRunShell( rcsdata data )
{
    userData *d = (userData*)data;
    if( d == NULL || d->getSystem() == NULL )
        return( 0 );
    return( d->getSystem()->runShell() );
}
void RCSAPI RCSSetPause( rcsdata data, int p )
{
    userData *d = (userData*)data;
    if( d == NULL || d->getSystem() == NULL )
        return;
    d->setPause( p );
}

/* common functions */

int RCSAPI RCSGetVersion() { return( RCS_DLL_VER ); }

int RCSAPI RCSSetSystem( rcsdata data, rcstype rcs_type )
{
    userData *d = (userData*)data;
    if( d == NULL )
        return( false );
    if( !d->setSystem( rcs_type ) )
        return( false );
    if( rcs_type > RCS_TYPE_LAST )
        return( false );
    MyWriteProfileString( d->getCfgDir(), rcs_type_strings[rcs_type] );
    return( true );

}

rcstype RCSAPI RCSQuerySystem( rcsdata data )
{
    char buffer[MAX_RCS_STRING_LEN];
    rcstype i;

    userData *d = (userData*)data;
    if( d == NULL )
        return( NO_RCS );
    MyGetProfileString( d->getCfgDir(), buffer, MAX_RCS_STRING_LEN );
    for( i = RCS_TYPE_FIRST; i <= RCS_TYPE_LAST; i = (rcstype)( i + 1 ) ) {
        if( strnicmp( buffer, rcs_type_strings[i], strlen( rcs_type_strings[i] ) ) == 0 ) {
            return( i );
        }
    }
    return( NO_RCS );
}

int RCSAPI RCSRegisterBatchCallback( rcsdata data, BatchCallback *fp, void *cookie )
{
    userData *d = (userData*)data;
    if( d == NULL )
        return( 0 );
    return( d->regBatcher( fp, cookie ) );
}

int RCSAPI RCSRegisterMessageBoxCallback( rcsdata data, MessageBoxCallback *fp, void *cookie )
{
    userData *d = (userData*)data;
    if( d == NULL )
        return( 0 );
    return( d->regMessager( fp, cookie ) );
}

#ifdef __NT__

BOOL WINAPI DllMain( HINSTANCE hDll, DWORD reason, LPVOID res )
{
    res = res;
    reason = reason;

    hInstance = hDll;
    return( 1 );
}

#elif defined( __WINDOWS__ )

int WINAPI LibMain( HINSTANCE hInst, WORD wDataSeg, WORD wHeapSize, LPSTR lpszCmdLine )
{
    wDataSeg = wDataSeg;
    wHeapSize = wHeapSize;
    lpszCmdLine = lpszCmdLine;

    hInstance = hInst;

    return( 1 );
}

int CALLBACK WEP( int q )
{
    q = q;
    return( 1);
}

#elif defined( __OS2__ )

int     __dll_initialize( void )
{
    return( 1 );
}

int     __dll_terminate( void )
{
    return( 1 );
}

#endif

}  // extern "C"

int userData::setSystem( rcstype rcs_type )
{
    if( currentSystem != NULL ) {
        currentSystem->fini();
    }
    currentSystem = rcs_systems[rcs_type];
    if( currentSystem != NULL ) {
        if( !currentSystem->init( this ) ) {
            return( false );
        }
    }
    return( true );
}

int rcsSystem::checkout( userData *d, rcsstring name, rcsstring pj, rcsstring tgt )
{
    char Buffer[BUFLEN];
    if( d == NULL )
        return( false );
    if( d->batcher ) {
        sprintf( Buffer, "%s %s %s %s %s",
            checkout_name, pause_strings[d->getPause()], name,
                pj != NULL ? pj : "", tgt != NULL ? tgt : "" );
        d->batcher( Buffer, d->batch_cookie );
    }
    return( true );
}

int rcsSystem::checkin( userData *d, rcsstring name, rcsstring pj, rcsstring tgt )
{
    char        MsgBuf[BUFLEN];
    char        Buffer[BUFLEN];
    char        path[_MAX_PATH];
    pgroup2     pg;
    int         i;
    FILE        *fp;

    *MsgBuf = '\0';
    if( d == NULL )
        return( false );
    if( d->msgBox ) {
        sprintf( Buffer, "Checkin %s", name );
        if( !d->msgBox( "Enter Message", Buffer, MsgBuf, BUFLEN, d->msg_cookie ) ) {
            return( true );
        }
    }
    _splitpath2( name, pg.buffer, &pg.drive, &pg.dir, NULL, NULL );
    _makepath( path, pg.drive, pg.dir, "temp", "___" );
    for( i = 0; i < 10; i++ ) {
        path[strlen(path)-1] = (char)(i + '0');
        if( access( path, W_OK ) ) {
            break;
        }
    }
    if( i >= 10 )
        return( false );

    fp = fopen( path, "w" );
    fputs( MsgBuf, fp );
//  fputs( ".", fp ); // for MKS
    fclose( fp );

    if( d->batcher ) {
        sprintf( Buffer, "%s %s %s %s %s ",
            checkin_name, name, path, pj, tgt );
        d->batcher( Buffer, d->batch_cookie );
    }
    remove( path );
    return( true );
}
