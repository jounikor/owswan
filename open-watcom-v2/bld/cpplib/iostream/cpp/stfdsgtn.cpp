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
* Description:
*
****************************************************************************/

#ifdef __SW_FH
#include "iost.h"
#else
#include "variety.h"
#include <cstring>
#include <iostream>
#include <streambu>
#endif
#include "ioutil.h"
#include "lock.h"


namespace std {

  // Return the "len" characters starting at get_ptr. If there aren't
  // enough characters, return as many as possible. Advance the get_ptr.

  streamsize streambuf::do_sgetn( char *buf, streamsize len ) {

    streamsize available;
    streamsize returned;

    returned = 0;
    __lock_it( __b_lock );
    while( len > 0 ) {
        // # characters left in buffer
        available = (__huge_ptr_int)(egptr() - gptr());
        if( available <= 0 ) {                  // nothing left?
            if( underflow() == EOF ) break;
            available = (__huge_ptr_int)(egptr() - gptr());
        }
        if( available > len ) {                 // too many?
            available = len;
        }
        memcpy( buf, gptr(), available );
        gbump( available );
        returned  += available;
        buf       += available;
        len       -= available;
    }
    return( returned );
  }

}
