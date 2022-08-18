/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Stack management macros.
*
****************************************************************************/


#if defined( __WATCOMC__ ) && defined( _M_IX86 ) && ( !defined(__OS2__) || defined(_M_I86) )

#if defined( _M_I86 )
    #include <malloc.h>
    #define VIHEAPGROW()    _nheapgrow()
    #define VIHEAPSHRINK()  _nheapshrink()
#else
    #define VIHEAPGROW()
    #define VIHEAPSHRINK()
#endif

#if defined( _M_I86 ) && ( defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__) )
    #define VIMALLOC    _nmalloc
    #define VIFREE      _nfree
#else
    #define VIMALLOC    malloc
    #define VIFREE      free
#endif

extern char _NEAR *GetSP( void );
extern void SetSP( char _NEAR * );
#ifdef _M_I86
#pragma aux GetSP = __value [__sp]
#pragma aux SetSP = \
        "mov  sp,ax" \
    __parm      [__ax] \
    __modify    [__sp]
#else
#pragma aux GetSP = __value [__esp]
#pragma aux SetSP = \
        "mov  esp,eax" \
    __parm      [__eax] \
    __modify    [__esp]
#endif

#define InitialStack() \
    { \
        VIHEAPGROW(); \
        sp = GetSP(); \
        stackptr = VIMALLOC( MIN_STACK_K * 1024 ); \
        if( stackptr == NULL ) { \
            exit( 1 ); \
        } \
        SetSP( stackptr + MIN_STACK_K * 1024 - 16 ); \
        _STACKLOW = (unsigned)stackptr; \
        _STACKTOP = (unsigned)( stackptr + MIN_STACK_K * 1024 - 16 ); \
    }

#define FinalStack() \
    { \
        SetSP( sp ); \
        VIFREE( stackptr ); \
        for( ;; ) { \
            stackptr2 = VIMALLOC( EditVars.StackK * 1024 ); \
            if( stackptr2 == NULL ) { \
                EditVars.StackK--; \
                if( EditVars.StackK < MIN_STACK_K ) { \
                    QuitEditor( ERR_NO_MEMORY ); \
                } \
            } else { \
                break; \
            } \
        } \
        SetSP( stackptr2 + EditVars.StackK * 1024 - 16 ); \
        _STACKLOW = (unsigned)stackptr2; \
        _STACKTOP = (unsigned)( stackptr2 + EditVars.StackK * 1024 - 16 ); \
        VIHEAPSHRINK(); \
    }

static char _NEAR  *stackptr, _NEAR *stackptr2;
static char _NEAR  *sp;

extern unsigned _STACKLOW;
extern unsigned _STACKTOP;

#else

#define InitialStack()
#define FinalStack()

#endif
