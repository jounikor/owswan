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
* Description:  This file contains prototypes and #defines used by the
*               implementation of the iostream library.
*               This file is not for distribution.
*
****************************************************************************/


#ifndef _IOUTIL_H_INCLUDED

// function should be inlined
#pragma disable_message( 656 )

#include <cstdio>
#include "xfloat.h"

typedef int __huge_ptr_int;

union ipvalue {
    void *pword;
    long  iword;
};

struct ios_word_values {
    ios_word_values *next;       // next group of values
    short            count;      // how many values are here?
    ipvalue          values[1];  // "count" iterations
};

extern int __FlagsToBase( long __format_flags );

class __WATCOM_ios {
public:
    static void *find_user_word( std::ios *pios, int index );
    static void free_xalloc_storage( std::ios *pios );
    static std::ios::iostate writeitem( std::ostream &ostrm,
                                        char const *buffer,
                                        int size,
                                        int fill_offset );
};

#define _IOUTIL_H_INCLUDED
#endif
