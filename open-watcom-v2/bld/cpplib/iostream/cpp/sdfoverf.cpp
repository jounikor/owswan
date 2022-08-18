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

#ifdef __SW_FH
#include "iost.h"
#else
#include "variety.h"
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <stdiobuf.h>
#endif
#include "ioutil.h"


// Handle allocating a buffer, if required. Handle overflow of the
// output streambuf buffer. Take the contents of the buffer and send
// them to the FILE stream found in file_pointer. Also send the
// character "c", unless it's EOF.

int stdiobuf::overflow( int c ) {

    int waiting;
    int written;

    __lock_it( __b_lock );

    // Flush any input waiting in the buffer:
    if( in_avail() > 0 ) {
        // note that sync() will discard get area
        if( sync() == EOF ) {
            return( EOF );
        }
    } else {
        // discard get area
        setg( NULL, NULL, NULL );
    }

    // Try to allocate a buffer:
    if( base() == NULL ) {
        if( allocate() == EOF ) {
            return( EOF );
        }
        if( base() == NULL ) {
            if( c != EOF ) {
                char charbuf = (char)c;
                written = std::fwrite( &charbuf
                                  , sizeof( char )
                                  , 1
                                  , __file_pointer );
                if( written != sizeof( char ) ) {
                    return( EOF );
                }
            }
            return( __NOT_EOF );
        } else {
            setp( base(), ebuf() );
        }
    } else if( pptr() <= pbase() ) {
        setp( base(), ebuf() );
    }

    // Now write the output:
    if( (c != EOF) && (pptr() < epptr()) ) {
        *pptr() = (char)c;
        pbump( 1 );
        c = EOF;
    }
    waiting = (__huge_ptr_int)(pptr() - pbase());
    if( waiting > 0 ) {
        written = std::fwrite( pbase(), sizeof( char ), waiting, __file_pointer );
        if( written == EOF ) {
            return( EOF );
        }
        waiting -= written;
        if( waiting > 0 ) {
            std::memmove( pbase(), pbase() + written, waiting );
            setp( pbase(), epptr() );
            pbump( waiting );
        } else {
            setp( pbase(), epptr() );
        }
    }
    if( c != EOF ) {
        if( pptr() < epptr() ) {
            *pptr() = (char)c;
            pbump( 1 );
        } else {
            return( EOF );
        }
    }
    if( waiting ) {
        return( EOF );
    } else {
        return( __NOT_EOF );
    }
}
