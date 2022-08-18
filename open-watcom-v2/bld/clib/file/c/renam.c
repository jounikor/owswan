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


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>
#include "seterrno.h"
#include "doserror.h"
#include "rtdata.h"
#include "tinyio.h"
#include "_doslfn.h"


#ifdef _M_I86
  #ifdef __BIG_DATA__
    #define AUX_INFO \
        __parm __caller     [__dx __ax] [__es __di] \
        __value             [__ax] \
        __modify __exact    [__ax __dx]
  #else
    #define AUX_INFO \
        __parm __caller     [__dx] [__di] \
        __value             [__ax] \
        __modify __exact    [__ax]
  #endif
#else
    #define AUX_INFO \
        __parm __caller     [__edx] [__edi] \
        __value             [__eax] \
        __modify __exact    [__eax]
#endif

extern unsigned __rename_sfn( const char *old, const char *new );
#pragma aux __rename_sfn =  \
        _SET_ES             \
        _SET_DSDX           \
        _MOV_AH DOS_RENAME  \
        _INT_21             \
        _RST_DS             \
        _RST_ES             \
        "call __doserror1_" \
    AUX_INFO

#if defined( __WATCOM_LFN__ ) && !defined( __WIDECHAR__ )

#ifdef _M_I86
extern lfn_ret_t __rename_lfn( const char *old, const char *new );
  #ifdef __BIG_DATA__
    #pragma aux __rename_lfn =  \
            "push   ds"         \
            "xchg   ax,dx"      \
            "mov    ds,ax"      \
            "mov    ax,7156h"   \
            "stc"               \
            "int 21h"           \
            "pop    ds"         \
            "call __lfnerror_0" \
        __parm __caller     [__dx __ax] [__es __di] \
        __value             [__dx __ax] \
        __modify __exact    [__ax __dx]
  #else
    #pragma aux __rename_lfn =  \
            "push   es"         \
            "mov    ax,ds"      \
            "mov    es,ax"      \
            "mov    ax,7156h"   \
            "stc"               \
            "int 21h"           \
            "pop    es"         \
            "call __lfnerror_0" \
        __parm __caller     [__dx] [__di] \
        __value             [__dx __ax] \
        __modify __exact    [__ax __dx]
  #endif
#endif

static lfn_ret_t _rename_lfn( const char *old, const char *new )
/**************************************************************/
{
#ifdef _M_I86
    return( __rename_lfn( old, new ) );
#else
    call_struct     dpmi_rm;

    strcpy( RM_TB_PARM1_LINEAR, old );
    strcpy( RM_TB_PARM2_LINEAR, new );
    memset( &dpmi_rm, 0, sizeof( dpmi_rm ) );
    dpmi_rm.ds  = RM_TB_PARM1_SEGM;
    dpmi_rm.edx = RM_TB_PARM1_OFFS;
    dpmi_rm.es  = RM_TB_PARM2_SEGM;
    dpmi_rm.edi = RM_TB_PARM2_OFFS;
    dpmi_rm.eax = 0x7156;
    return( __dpmi_dos_call_lfn( &dpmi_rm ) );
#endif
}

#endif  /* __WATCOM_LFN__ && !__WIDECHAR__ */

_WCRTLINK int __F_NAME(rename,_wrename)( const CHAR_TYPE *old, const CHAR_TYPE *new )
/***********************************************************************************/
{
#ifdef __WIDECHAR__
    char        mbOld[MB_CUR_MAX * _MAX_PATH];      /* single-byte char */
    char        mbNew[MB_CUR_MAX * _MAX_PATH];      /* single-byte char */

    if( wcstombs( mbOld, old, sizeof( mbOld ) ) == (size_t)-1 ) {
        mbOld[0] = '\0';
    }
    if( wcstombs( mbNew, new, sizeof( mbNew ) ) == (size_t)-1 ) {
        mbNew[0] = '\0';
    }
    return( rename( mbOld, mbNew ) );
#else
  #if defined( __WATCOM_LFN__ )
    if( _RWD_uselfn ) {
        lfn_ret_t   rc;

        rc = _rename_lfn( old, new );
        if( LFN_ERROR( rc ) ) {
            return( __set_errno_dos( LFN_INFO( rc ) ) );
        }
        if( LFN_OK( rc ) ) {
            return( 0 );
        }
    }
  #endif
    return( __rename_sfn( old, new ) );
#endif
}
