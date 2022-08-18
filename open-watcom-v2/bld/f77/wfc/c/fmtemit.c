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


//
// FmtEmit      : routines to emit format code information at compile time
//

#include "ftnstd.h"
#include "format.h"
#include "errcod.h"
#include "global.h"
#include "fmtdef.h"
#include "fmtdat.h"
#include "cpopt.h"
#include "emitobj.h"
#include "fmterr.h"
#include "fmtemit.h"


void    GFEmEnd( void )
//=====================
{
    OutByte( END_FORMAT );
    OutInt( ObjOffset( Fmt_revert.cp ) );
    AlignEven();
}


void    GFEmCode( int int_code )
//==============================
{
    byte code = int_code;           // needed to match signature

    if( (code & REV_CODE) == REV_CODE ) {
        code &= ~REV_CODE;
        Fmt_revert.cp = ObjTell();
    }
    if( Options & OPT_EXTEND_FORMAT ) {
        code |= EXTEND_FORMAT;
    }
    OutByte( code );
}


void    GFEmChar( char *ch )
//==========================
{
    OutByte( *ch );
}


void    GFEmNum( int num )
//========================
{
    OutInt( num );
}


void    GFEmByte( int num )
//=========================
{
    if( ( num < 0 ) || ( num > 255 ) ) {
        FmtError( FM_SPEC_256 );
    } else {
        OutByte( num );
    }
}
