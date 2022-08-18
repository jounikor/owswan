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
* Description:  Fatal runtime error handlers.
*
****************************************************************************/


#ifndef _EXITWMSG_H_INCLUDED
#define _EXITWMSG_H_INCLUDED

#ifdef __cplusplus
    extern "C" {
#endif

// C interface
// - tracks normal calling convention
// - this is the funtion that is called from ASM and from C, C++
// - note there is no #pragma aborts so that debugger can trace out
extern _WCRTLINK _WCNORETURN void __exit_with_msg( char _WCI86FAR *, int );
extern _WCRTLINK _WCNORETURN void __fatal_runtime_error( char _WCI86FAR *, int );
extern _WCRTLINK _WCNORETURN void __exit( int );

// WVIDEO interface

extern _WCRTDATA char volatile __WD_Present;

// this function should be called before __exit_with_msg()
// to allow Watcom Debugger (see WVIDEO) to trap runtime errors.
// this really needs to be far!!!
extern _WCRTLINK int __EnterWVIDEO( char _WCFAR *string );

#ifdef __cplusplus
    };
#endif
#endif
