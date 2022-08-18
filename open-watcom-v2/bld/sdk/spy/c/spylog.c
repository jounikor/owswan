/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2017-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Spy log functions.
*
****************************************************************************/


#include "spy.h"
#include <time.h>


/*
 * LogSpyBoxHeader - write out spy log title
 */
void LogSpyBoxHeader( FILE *f )
{
    size_t      len;
    char        buff[128];
    time_t      tod;

    tod = time( NULL );
    strcpy( buff, ctime( &tod ) );
    buff[strlen( buff ) - 1] = '\0';

    fwrite( SpyName, 1, strlen( SpyName ), f );
    fwrite( "\n", 1, 1, f );
    fwrite( buff, 1, strlen( buff ), f );
    fwrite( "\n", 1, 1, f );
    fwrite( "\n", 1, 1, f );
    fwrite( TitleBar, 1, TitleBarLen, f );
    fwrite( "\n", 1, 1, f );
    len = strlen( TitleBarULine );
    fwrite( TitleBarULine, 1, len, f );
    fwrite( "\n", 1, 1, f );

} /* LogSpyBoxHeader */
