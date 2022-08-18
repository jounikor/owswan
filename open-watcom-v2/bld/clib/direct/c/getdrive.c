/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Get active drive.
*
****************************************************************************/


#include "variety.h"
#include <direct.h>
#if defined( __DOS__ ) || defined( __WINDOWS__ )
    #include <dos.h>
#elif defined( __OS2__ )
    #include <wos2.h>
#elif defined( __NT__ )
    #include <windows.h>
#elif defined( __RDOS__ ) || defined( __RDOSDEV__ )
    #include <rdos.h>
#endif

_WCRTLINK int _getdrive( void )
{
#if defined( __DOS__ ) || defined( __WINDOWS__ )
    unsigned    dnum;

    _dos_getdrive( &dnum );
    return( dnum );
#elif defined( __OS2__ )
    OS_UINT     dnum;
    ULONG       ndrv;

    DosQCurDisk( &dnum, &ndrv );
    return( (int)dnum );
#elif defined( __NT__ )
    char        dir[MAX_PATH];  // [4]

    GetCurrentDirectory( sizeof( dir ), dir );
    return( tolower( (unsigned char)dir[0] ) - 'a' + 1 );
#elif defined( __RDOS__ ) || defined( __RDOSDEV__ )
    return( RdosGetCurDrive() + 1 );
#endif
}
