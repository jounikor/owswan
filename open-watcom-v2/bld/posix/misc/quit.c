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
* Description:  Quit - print usage messages and exit
*
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "misc.h"

void Quit( const char *usage_msg[], const char *str, ... )
{
    int         i;
    va_list     args;

    if( str != NULL ) {
        va_start( args, str );
        vfprintf( stderr, str, args );
        va_end( args );
    }
    if( usage_msg == NULL ) {
        fprintf( stderr, "The Weenie That Wrote This Didn't Give A Usage String\n" );
        exit( EXIT_FAILURE );
    }
    if( str != NULL ) {
        fprintf( stderr, "%s\n", usage_msg[0] );
        exit( EXIT_FAILURE );
    }
    i = 0;
    while( usage_msg[i] != NULL ) {
        fprintf( stderr, "%s\n", usage_msg[i++] );
    }
    exit( EXIT_FAILURE );
}
