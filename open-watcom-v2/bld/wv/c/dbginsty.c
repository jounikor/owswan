/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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


#include <ctype.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgstk.h"
#include "dbgitem.h"
#include "dbgio.h"
#include "wspawn.h"
#include "mad.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgexpr2.h"
#include "dbgexpr.h"
#include "dbgovl.h"
#include "dbgbrk.h"
#include "dbgparse.h"
#include "dipimp.h"
#include "dipinter.h"
#include "dbginsty.h"


void BreakOnSelected( const char *item )
{
    const char      *old;

    old = ReScan( item );
    EvalLValExpr( 1 );
    ReScan( old );
    BreakOnExprSP( (void *)item );
}


bool ScanSelectedExpr( const char *expr )
{
    const char  *old;
    bool        rc;

    old = ReScan( expr );
    _SwitchOn( SW_ERR_IN_TXTBUFF );
    _SwitchOn( SW_NO_DISAMBIGUATOR );
    rc = ( Spawn( NormalExpr ) == 0 );
    _SwitchOff( SW_NO_DISAMBIGUATOR );
    _SwitchOff( SW_ERR_IN_TXTBUFF );
    ReScan( old );
    return( rc );
}

inspect_type WndGetExprSPInspectType( address *paddr )
{
    sym_info    info;
    DIPHDL( sym, sh );

    LValue( ExprSP );
    *paddr = NilAddr;
    if( ExprSP->ti.kind == TK_FUNCTION ) {
        ExprValue( ExprSP );
        *paddr = ExprSP->v.addr;
        PopEntry();
        return( INSP_CODE );
    } else if( (ExprSP->flags & SF_LOCATION) && ExprSP->th != NULL && !IsInternalMod( DIPTypeMod( ExprSP->th ) ) ) {
        if( ExprSP->v.loc.num == 1 && ExprSP->v.loc.e[0].type == LT_ADDR ) {
            *paddr = ExprSP->v.loc.e[0].u.addr;
        }
        PopEntry();
        return( INSP_DATA );
    } else {
        if( (ExprSP->flags & SF_LOCATION) && ExprSP->v.loc.e[0].type == LT_ADDR ) {
            *paddr = ExprSP->v.loc.e[0].u.addr;
        } else if( ExprSP->ti.kind == TK_ADDRESS || ExprSP->ti.kind == TK_POINTER ) {
            *paddr = ExprSP->v.addr;
        }
        if( !IS_NIL_ADDR( *paddr ) ) {
            AddrFloat( paddr );
            if( DeAliasAddrSym( NO_MOD, *paddr, sh ) != SR_NONE ) {
                DIPSymInfo( sh, ExprSP->lc, &info );
                PopEntry();
                switch( info.kind ) {
                case SK_CODE:
                case SK_PROCEDURE:
                    return( INSP_CODE );
                    break;
                default:
                    return( INSP_RAW_DATA );
                    break;
                }
            }
        }
    }
    ExprValue( ExprSP );
    MakeMemoryAddr( true, EXPR_DATA, paddr );
    return( INSP_RAW_DATA );
}

static void DoLValExpr( void )
/********************************/
{
    EvalLValExpr( 1 );
    ReqEOC();
}

bool    WndEvalInspectExpr( const char *item, bool pop )
/******************************************************/
{
    const char  *old;
    char        buff[12],*p;
    bool        rc;

    if( ispunct(item[0]) &&
      ( item[1] == NULLCHAR || ( ispunct( item[1] ) && item[2] == NULLCHAR ) ) ) {
        // nyi - pui - use SSL
        p = StrCopy( item, StrCopy( "operator", buff ) );
        if( item[0] == '[' && item[1] == NULLCHAR ) {
            StrCopy( "]", p );
        } else if( item[0] == '(' && item[1] == NULLCHAR ) {
            StrCopy( ")", p );
        }
        old = ReScan( buff );
    } else {
        old = ReScan( item );
    }
    _SwitchOn( SW_CALL_FATAL );
    _SwitchOn( SW_ERR_IN_TXTBUFF );
    _SwitchOn( SW_NO_DISAMBIGUATOR );
    rc = ( Spawn( DoLValExpr ) == 0 );
    _SwitchOff( SW_CALL_FATAL );
    _SwitchOff( SW_NO_DISAMBIGUATOR );
    _SwitchOff( SW_ERR_IN_TXTBUFF );
    ReScan( old );
    if( pop && rc )
        PopEntry();
    return( rc );
}

