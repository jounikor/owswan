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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %     Copyright (C) 1992, by WATCOM International Inc.  All rights    %
// %     reserved.  No part of this software may be reproduced or        %
// %     used in any form or by any means - graphic, electronic or       %
// %     mechanical, including photocopying, recording, taping or        %
// %     information storage and retrieval systems - except with the     %
// %     written permission of WATCOM International Inc.                 %
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//
//  Modified    By              Reason
//  ========    ==              ======
//  92/01/09    Steve McDowell  Initial simple implementation using C library
//                              string functions wherever possible.
//  92/01/20    ...             Added checking for invalid Strings, including
//                              adding the ! operator, the == and !=
//                              operators with integer arguments, and the
//                              "valid" member and friend functions.
//  92/03/11    ...             Re-implemented using StringReps (reference
//                              counts, offset and length).
//  92/10/08    Greg Bentz      Cleanup.
//  93/08/31    Greg Bentz      - make extractor skip whitespace if
//                                ios::skipws is set
//                              - correct offset computation in substring
//  93/10/07    Greg Bentz      - alloc_mult_size() not setting new value right
//  93/10/19    Raymond Tang    split into separate files
//  94/04/06    Greg Bentz      combine header files

#include "strng.h"

int __CompareChars( const char *l, long p_llen,
                    const char *r, long p_rlen ) {
/*****************************************************/
// Compare the strings specified by l/p_llen and r/p_rlen.
// Return -1, 0 or 1 depending on whether the relation is <, == or >.
// If either length < 0, then use std::strlen to find the length.
    int         ret;
    std::size_t llen;
    std::size_t rlen;

    if( l == NULL ) {
        llen = 0;
    } else if( p_llen < 0 ) {
        llen = std::strlen( l );
    } else {
        llen = (std::size_t)p_llen;
    }
    if( r == NULL ) {
        rlen = 0;
    } else if( p_rlen < 0 ) {
        rlen = std::strlen( r );
    } else {
        rlen = (std::size_t)p_rlen;
    }
    if( llen == rlen ) {
        return( std::memcmp( l, r, llen ) );
    }
    if( llen < rlen ) {
        ret = std::memcmp( l, r, llen );
        return( ret == 0 ? -1 : ret );
    }
    ret = std::memcmp( l, r, rlen );
    return( ret == 0 ? 1 : ret );
}

