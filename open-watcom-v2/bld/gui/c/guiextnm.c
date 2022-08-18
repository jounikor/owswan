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
* Description:  Get/set resource file name.
*
****************************************************************************/


#include <stdlib.h>
#include <string.h>
#include "bool.h"
#include "guiextnm.h"

#include "clibext.h"


static char   GUIResFileName[_MAX_PATH] = "";

char *GUIGetResFileName( void )
{
    if( GUIResFileName[0] == '\0' ) {
#if defined( __UNIX__ ) && defined( GUI_EXT_RES )
        _cmdname( GUIResFileName );
        strcat( GUIResFileName, ".res" );
        return( GUIResFileName );
#else
        return( NULL );
#endif
    }
    /* to keep the 10.6 compiler happy */
    return( GUIResFileName );
}

bool GUISetResFileName( const char *fname )
{
    if( strlen( fname ) < _MAX_PATH ) {
        strcpy( GUIResFileName, fname );
        return( true );
    }
    return( false );
}
