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


#ifndef _WSAMPLE_H_INCLUDED
#define _WSAMPLE_H_INCLUDED

#ifdef _MARK_ON
    extern void __mark( void __far *, unsigned short, void __near * );
  #ifdef __386__
    #define AUX_INFO_PARM __nomemory [__dx __eax] [__bx] [__ecx]
  #else
    #define AUX_INFO_PARM __nomemory [__dx __ax] [__bx] [__cx]
  #endif
    #pragma aux __mark = \
            "int 3" \
        __parm                      AUX_INFO_PARM \
        __value                     \
        __modify __exact __nomemory []
    #define _MARK_( x )   __mark( x, 0, (void __near *)0 )
#else
    #define _MARK_( x )
#endif

#endif
