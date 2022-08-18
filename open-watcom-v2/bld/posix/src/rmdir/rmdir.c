/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  POSIX rmdir utility
*               Removes directories
*
****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <direct.h>
#include "bool.h"
#include "misc.h"
#include "getopt.h"
#include "argvrx.h"
#include "argvenv.h"


char *OptEnvVar="rmdir";

static const char *usageMsg[] = {
    "Usage: rmdir [-?] [@env] directory [directories...]",
    "\tenv                : environment variable to expand",
    "\tdirectory          : name of directory to be removed",
    "\tOptions: -?        : display this message",
    NULL
};

int main( int argc, char **argv )
{
    int     ret = EXIT_SUCCESS;

    argv = ExpandEnv( &argc, argv );
    GetOpt( &argc, argv, "", usageMsg );
    argv++;
    if( argc == 1 ) {
        Die( "%s\n", usageMsg[0] );
    } else {
        while( *argv != NULL ) {
            if( rmdir( *argv ) ) {
                fprintf( stderr, "rmdir: access denied \"%s\"\n", *argv );
                ret = EXIT_FAILURE;
            }
            argv++;
        }
    }
    return( ret );
}
