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
* Description:  Profiler test app, Part IV.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include "pt.h"

extern char flags[SIZE + 1];
extern int count, niter;


int fetch( int argc, char **argv )
{
    int retval;
    int i;

    if( argc == 2 ) {
        for( i = 0; i < 2000; ++i ) {
            retval = atoi( argv[1] );
            _test_486();
        }
    } else {
        printf( "Use: %s  unsigned\n", argv[0] );
        exit( -1 );
    }
    printf ("%d iterations:\n", retval);
    for( i=1; (long)i < 50L * retval; ++i )  ;

    return( retval );
}
