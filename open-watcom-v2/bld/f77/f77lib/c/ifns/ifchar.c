/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  convert integer to character
*
****************************************************************************/


#include "ftnstd.h"
#include "rterr.h"
#include "ifenv.h"
#include "errcod.h"


void    CHAR( intstar4 arg, string PGM *dest ) {
//==============================================

// Convert character to integer.

// -128 <= arg <= 127 handles the case for:
//      integer*1 i
//      i = 173
//      print *, char(i)
// 0 <= arg <= 255 handles the case for:
//      integer i
//      i = 173
//      print *, char(i)

    if( ( arg < -128 ) || ( arg > 255 ) ) {
        RTErr( LI_CHAR_BOUND );
    }
    *dest->strptr = arg;
}
