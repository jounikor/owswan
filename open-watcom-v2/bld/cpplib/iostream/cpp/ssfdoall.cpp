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
#include <strstream>
#endif
#include "ioutil.h"


namespace std {

  // Do the allocation required if allocate() thinks it's needed. If the
  // allocation fails, return EOF. Copy the old buffer into the new
  // buffer. Free the old buffer. Return __NOT_EOF if everything
  // succeeds.

  int strstreambuf::doallocate() {

    char      *oldbuf;
    streamsize oldbufsize;
    char      *newbuf;
    streamsize newbufsize;
    size_t     base_offset, ptr_offset, end_offset;

    __lock_it( __b_lock );
    if( !__dynamic || __frozen ) {
        return( EOF );
    }
    oldbuf     = base();
    oldbufsize = blen();
    if( __allocation_size <= oldbufsize ) {
        newbufsize = oldbufsize + DEFAULT_MAINBUF_SIZE;
    } else {
        newbufsize = __allocation_size;
    }
    if( __alloc_fn == NULL ) {
        newbuf = new char [newbufsize];
    } else {
        newbuf = (char *) __alloc_fn( newbufsize );
    }
    if( newbuf == NULL ) {
        return( EOF );
    }
    setb( newbuf, newbuf + newbufsize, false );

    // Copy the get area. This can be done directly by copying bytes and
    // setting new pointers. The size of the get area does not change.
    if( eback() != NULL ) {
        base_offset = (__huge_ptr_int)(eback() - oldbuf);
        ptr_offset  = (__huge_ptr_int)(gptr()  - oldbuf);
        end_offset  = (__huge_ptr_int)(egptr() - oldbuf);
        memcpy( newbuf+base_offset, eback(), end_offset-base_offset );
        setg( newbuf+base_offset, newbuf+ptr_offset, newbuf+end_offset );
    }

    // Copy the put area. This can be done directly by copying bytes and
    // setting new pointers. Add the extra bytes allocated above to the
    // end of the put area.
    if( pbase() == NULL ) {
        setp( newbuf, newbuf + newbufsize );
    } else {
        base_offset = (__huge_ptr_int)(pbase() - oldbuf);
        ptr_offset  = (__huge_ptr_int)(pptr()  - oldbuf);
        end_offset  = (__huge_ptr_int)(epptr() - oldbuf);
        memcpy( newbuf+base_offset, pbase(), end_offset-base_offset );
        end_offset += newbufsize - oldbufsize;    // grow the put area
        setp( newbuf+base_offset, newbuf+end_offset );
        pbump( ptr_offset - base_offset );
    }

    // Free the old buffer.
    if( oldbuf != NULL ) {
        if( __free_fn == NULL ) {
            delete [] oldbuf;
        } else {
            __free_fn( oldbuf );
        }
    }
    return( __NOT_EOF );
  }

} // namespace std
