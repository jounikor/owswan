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


#ifndef TYPE_H
#define TYPE_H
/*
 Description:
 ============
    Some stuff for windows development.

*/
#ifdef __OS2__
#include <pm1632.h>
#else
#include <wi163264.h>
#endif

#ifndef WINEXP
    #define WINEXP APIENTRY                     // use with linker export option
#endif
#ifndef WINIEXP
    #define WINIEXP CALLBACK            // use with in-obj export rtns
#endif
#ifndef WINREXP
    #define WINREXP __export CALLBACK
#endif

typedef unsigned char   bool;

#define _max( x, y ) ( ( (x) < (y) ) ? (y) : (x) )
#define _min( x, y ) ( ( (x) > (y) ) ? (y) : (x) )

extern void debugger_int( void );
#pragma aux debugger_int = "int 3h"

#endif

