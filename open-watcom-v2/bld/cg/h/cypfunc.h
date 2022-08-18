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


#if defined(__WATCOMC__) && defined( _M_IX86 )

#if defined(__FLAT__) || defined(__SMALL__) || defined(__MEDIUM__)
    #define _SAVES  "push    es"
    #define _RESES  "pop     es"
    #define _SETES  "push    ds" \
                    "pop     es"
    #define _SREG_ES
    #define _SREG_DS
#else
    #define _SAVES
    #define _RESES
    #define _SETES
    #define _SREG_ES    __es
    #define _SREG_DS    __ds
#endif

extern void     *CypCopy( const void *, void *, size_t );
#pragma aux CypCopy = \
        _SAVES \
        _SETES \
        "rep movsb" \
        _RESES \
    __parm __routine    [_SREG_DS __esi] [_SREG_ES __edi] [__ecx] \
    __value             [_SREG_ES __edi]

extern void     *CypFill( void *, size_t, unsigned char );
#pragma aux CypFill = \
        _SAVES \
        _SETES \
        "rep stosb" \
        _RESES \
    __parm __routine    [_SREG_ES __edi] [__ecx] [__al] \
    __value             [_SREG_ES __edi]

extern size_t   CypLength( const char *);
#pragma aux CypLength = \
        _SAVES \
        _SETES \
        "xor    eax,eax" \
        "xor    ecx,ecx" \
        "dec    ecx" \
        "repne scasb" \
        "not    ecx" \
        "dec    ecx" \
        _RESES \
    __parm __routine    [_SREG_ES __edi] \
    __value             [__ecx] \
    __modify            [__eax]

extern bool     CypEqual( const void *, const void *, size_t );
#pragma aux CypEqual = \
        _SAVES \
        _SETES \
        "xor    eax,eax" \
        "repe cmpsb" \
        "jne short L1" \
        "inc    eax" \
    "L1:" \
        _RESES \
    __parm __routine    [_SREG_DS __esi] [_SREG_ES __edi] [__ecx] \
    __value             [__al]

#endif
