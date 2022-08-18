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
* Description:  Utility program used in wmake regression tests.
*
****************************************************************************/


#include <stdio.h>
#if defined( __WATCOMC__ ) || !defined( __UNIX__ )
    #include <direct.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

#include "clibext.h"


int main( int argc, char* argv[] )
{
    char buffer[ PATH_MAX + 1 ];
    int  size = sizeof(buffer);

    if( argc >= 2 ) {
        getcwd( buffer, size );
        printf( "\n%s>%s\n", buffer, argv[1] );
    } else{
        printf( "Error: %s <argument>\n", argv[0] );
    }
    return( 0 );
}
