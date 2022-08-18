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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/*
BAGGAGE:  Baggage file handling.
*/
#include <stdlib.h>
#include <string.h>
#include "baggage.h"
#if defined( __UNIX__ ) && defined( __WATCOMC__ )
  #if ( __WATCOMC__ < 1300 )
    // fix for OW 1.9
    #include <limits.h>
  #endif
#endif
#include "pathgrp2.h"

#include "clibext.h"


//  Baggage::Baggage

Baggage::Baggage( HFSDirectory *d_file, char const filename[] )
    : File( filename )
{
    char        fname[_MAX_PATH];
    pgroup2     pg;

    if( !_badFile ) {
        reset( 0, SEEK_END );
        _size = (uint_32)tell();
        reset();
        _splitpath2( _fullName, pg.buffer, NULL, NULL, &pg.fname, &pg.ext );
        _makepath( fname, NULL, NULL, pg.fname, pg.ext );
        d_file->addFile( this, fname );
        close();
    } else {
        // run in circles, scream and shout.
        HCWarning( FILE_ERR, filename );
        _size = 0;
    }
}


//  Baggage::dump   --Overrides Dumpable::dump

#define BDUMP_SIZE  1024
int Baggage::dump( OutFile * dest )
{
    if( _badFile ) {
        return 1;
    }
    if( !open() ) {
        return 1;
    }
    char    *buf = new char[BDUMP_SIZE];
    size_t  left_to_dump;
    size_t  amount_dumped;

    for( left_to_dump = _size; left_to_dump != 0; left_to_dump -= amount_dumped ) {
        amount_dumped = fread( buf, 1, BDUMP_SIZE, _fp );
        dest->write( buf, amount_dumped );
    }
    delete[] buf;
    close();
    return 1;
}
