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
* Description:  C runtime library internal functions that are needed
*               in other libraries.
*
****************************************************************************/


#ifndef _CLIBSUPP_H_INCLUDED
#define _CLIBSUPP_H_INCLUDED

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* used by math run-time library */
_WCRTLINK extern void   __set_EDOM( void );
_WCRTLINK extern void   __set_ERANGE( void );

/* used by math and C++ run-time library */
_WCRTLINK extern FILE   *__get_std_stream( unsigned handle );

/* used by C++ run-time library */
_WCRTLINK extern int    __flush( FILE * );
_WCRTLINK extern int    __plusplus_fstat( int handle, int *pios_mode );
_WCRTLINK extern int    __plusplus_open( const char *name, int *pios_mode, int prot );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
