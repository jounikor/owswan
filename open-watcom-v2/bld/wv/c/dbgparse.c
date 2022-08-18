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
* Description:  Expression parser (using .prs grammar files).
*
****************************************************************************/


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgstk.h"
#include "dbgerr.h"
#include "dbgio.h"
#include "dbgmem.h"
#include "i64.h"
#include "dbgscan.h"
#include "dbgexpr3.h"
#include "dbgexpr2.h"
#include "dbgexpr.h"
#include "dbgovl.h"
#include "dbgparse.h"
#include "dbgwalk.h"
#include "dbgreg.h"


static char         *ParseTable;
static unsigned     ParseTableSize;
static token_table  ParseTokens;

#define PARSE_STACK_SIZE 128


static void start_expr( void )
{
    const char  *stack[PARSE_STACK_SIZE];

    ScanExpr( &ParseTokens );
    SSLWalk( ParseTable, 0, stack, PARSE_STACK_SIZE );
    ExprResolve( ExprSP );
    ScanExpr( NULL );
}


void SetUpExpr( unsigned addr_depth )
{
    SkipCount = 0;
    ExprAddrDepth = addr_depth;
}


void EvalLValExpr( unsigned addr_depth )
{
    SetUpExpr( addr_depth );
    start_expr();
}


void EvalExpr( unsigned addr_depth )
{
    EvalLValExpr( addr_depth );
    ExprValue( ExprSP );
}

void NormalExpr( void )
{
    EvalExpr( 1 );
}



/*
 * ChkExpr -- check out expression syntax
 */

void ChkExpr( void )
{
    SkipCount = 1;
    ExprAddrDepth = 1;
    start_expr();
}
/*
 * ReqExpr -- get a required expression
 */

unsigned_64 ReqU64Expr( void )
{
    unsigned_64 rtn;

    NormalExpr();
    ConvertTo( ExprSP, TK_INTEGER, TM_UNSIGNED, sizeof( ExprSP->v.uint ) );
    rtn = ExprSP->v.uint;
    PopEntry();
    return( rtn );
}

long ReqLongExpr( void )
{
    unsigned_64 tmp;

    tmp = ReqU64Expr();
    return( U32FetchTrunc( tmp ) );
}

unsigned ReqExpr( void )
{
    unsigned_64 tmp;

    tmp = ReqU64Expr();
    return( U32FetchTrunc( tmp ) );
}


/*
 * ReqXRealExpr -- get a required floating point expression
 */

#ifdef DEADCODE
xreal ReqXRealExpr( void )
{
    xreal v;

    NormalExpr();
    ConvertTo( ExprSP, TK_REAL, TM_NONE, sizeof( ExprSP->v.real ) );
    v = ExprSP->v.real;
    PopEntry();
    return( v );
}
#endif


/*
 * OptExpr -- get an optional expression
 */

unsigned OptExpr( unsigned def_val )
{
    if( CurrToken == T_COMMA || CurrToken == T_LEFT_BRACE || ScanEOC() )
        return( def_val );
    return( ReqExpr() );
}

void MakeMemoryAddr( bool pops, memory_expr def_seg, address *val )
{
    if( ExprSP->flags & SF_LOCATION ) {
        ExprSP->flags &= ~(SF_LOCATION|SF_IMP_ADDR);
        ExprSP->v.addr = ExprSP->v.loc.e[0].u.addr;
        ExprSP->ti.kind = TK_ADDRESS;
        ExprSP->ti.modifier = TM_FAR;
    }
    switch( ExprSP->ti.kind ) {
    case TK_ADDRESS:
    case TK_POINTER:
        if( ExprSP->ti.modifier != TM_NEAR )
            break;
        /* fall through */
    default:
        DefAddr( def_seg, val );
        AddrFix( val );
        //NYI: lost address abstraction
        PushInt( val->mach.segment );
        SwapStack( 1 );
        MakeAddr();
    }
    *val = ExprSP->v.addr;
    AddrFloat( val );
    if( pops ) {
        PopEntry();
    }
}


/*
 * ReqMemAddr -- get a required memory address
 */

void ReqMemAddr( memory_expr def_seg, address *out_val )
{
    mad_radix   old_radix;

    old_radix = SetCurrRadix( 16 );
    _SwitchOff( SW_EXPR_IS_CALL );
    EvalExpr( 0 );   /* memory expression */
    MakeMemoryAddr( true, def_seg, out_val );
    SetCurrRadix( old_radix );
}


/*
 * CallExpr -- get a call expression
 */
void CallExpr( address *out_val )
{
    mad_radix   old_radix;

    old_radix = SetCurrRadix( 16 );
    _SwitchOn( SW_EXPR_IS_CALL );
    EvalExpr( 0 ); /* call expression */
    MakeMemoryAddr( true, EXPR_CODE, out_val );
    SetCurrRadix( old_radix );
}


/*
 * OptMemAddr -- get an optional memory address
 */

void OptMemAddr( memory_expr def_seg, address *def_val )
{
    if( CurrToken == T_COMMA || ScanEOC() )
        return;
    ReqMemAddr( def_seg, def_val );
}


void SetTokens( bool parse_tokens )
{
    if( parse_tokens ) {
        ScanExpr( &ParseTokens );
    } else {
        ScanExpr( NULL );
    }
}



#define PARSE_TABLE_INIT (1024*4)

void LangInit( void )
{
    _Alloc( ParseTable, PARSE_TABLE_INIT );
    ParseTableSize = PARSE_TABLE_INIT;
}

void LangFini( void )
{
    _Free( ParseTable );
    ParseTable = NULL;
    ParseTableSize = 0;
}

static unsigned ReadSection( file_handle fh, unsigned off )
{
    unsigned_16 len;
    unsigned    last;
    void        *new;

    if( ReadStream( fh, &len, sizeof( len ) ) != sizeof( len ) ) {
        return( 0 );
    }
    CONV_LE_16( len );
    last = off + len;
    if( last > ParseTableSize ) {
        new = ParseTable;
        _Realloc( new, last );
        if( new == NULL )
            return( 0 );
        ParseTable = new;
        ParseTableSize = last;
    }
    if( ReadStream( fh, &ParseTable[off], len ) != len ) {
        return( 0 );
    }
    return( off + len );
}

static bool ReadAllSections( file_handle fh )
{
    unsigned    key_off;
    unsigned    delim_off;

    /* read rules */
    key_off = ReadSection( fh, 0 );
    if( key_off == 0 )
        return( false );
    /* read keywords */
    delim_off = ReadSection( fh, key_off );
    if( delim_off == 0 )
        return( false );
    /* read delimiters */
    if( ReadSection( fh, delim_off ) == 0 )
        return( false );
    ParseTokens.keywords = &ParseTable[key_off];
    ParseTokens.delims = &ParseTable[delim_off];
    return( true );
}

bool LangLoad( const char *lang, size_t langlen )
{
    file_handle fh;
    bool        ret;
#ifdef USE_FILENAME_VERSION
    char        lang_fname[13];
    size_t      len;

  #define STRXX(x)  #x
  #define STRX(x)   STRXX(x)

    len = langlen;
    if( len > 6 ) {
        len = 6;
    }
    memcpy( lang_fname, lang, len );
    strcpy( lang_fname + len, STRX( USE_FILENAME_VERSION ) );
    fh = LocalPathOpen( lang_fname, len + 2, "prs" );
#else
    fh = LocalPathOpen( lang, langlen, "prs" );
#endif
    if( fh == NIL_HANDLE )
        return( false );
    ret = ReadAllSections( fh );
    FileClose( fh );
    return( ret );
}
