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
* Description:  Debugger 'print' command.
*
****************************************************************************/


#include <ctype.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgstk.h"
#include "dbglit.h"
#include "dbgerr.h"
#include "dbgitem.h"
#include "ldsupp.h"
#include "madinter.h"
#include "dui.h"
#include "i64.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgutil.h"
#include "dbgexpr4.h"
#include "dbgexpr3.h"
#include "dbgexpr2.h"
#include "dbgexpr.h"
#include "dbgmain.h"
#include "dbgovl.h"
#include "dbgparse.h"
#include "dbgprint.h"
#include "remcore.h"
#include "dbgreg.h"
#include "dbgio.h"


#define BUFLEN  UTIL_LEN

#define PREC_LONG_FLOAT 15
#define PREC_FLOAT      7

#define MAX_WIDTH       40
#define NAME_WIDTH      25

#define FMT2RADIX(x)   (((x)<0)?(mad_radix)-(x):(mad_radix)(x))

// Brian!!!! NYI NYI NYI
#define _SetMaxPrec( x )

#define PRINT_OPTS \
    pick( "Program", PRINT_PROGRAM ) \
    pick( "Window",  PRINT_WINDOW  )

enum {
    #define pick(t,e)   e,
    PRINT_OPTS
    #undef pick
};

typedef enum {
    NUM_SIGNED,
    NUM_UNSIGNED,
    NUM_CHECK
} sign_class;

extern void             GraphicDisplay( void );
extern bool             DlgNewWithSym( const char *title, char *, size_t );

static char             *OutPtr;
static char             *OutBuff;
static unsigned         OutLen;
static bool             OutPgm;
static bool             First;

static const char PrintOps[] = {
    #define pick(t,e)   t "\0"
    PRINT_OPTS
    #undef pick
};


void StartPrintBuff( char *buff, unsigned len )
{
    OutPtr = buff;
    OutBuff = buff;
    OutLen = len;
    *buff = 'F'; /* no buffer flush */
}


static void PrtBuff( void )
{
    if( OutPgm ) {
        WriteToPgmScreen( OutBuff, OutPtr - OutBuff );
    } else {
        *OutPtr = NULLCHAR;
        DUIDlgTxt( OutBuff );
    }
    OutPtr = OutBuff;
    *OutPtr = NULLCHAR; /* indicate buffer flush */
}


static void PrtChar( unsigned ch )
{
    unsigned    len;

    len = OutPtr - OutBuff;
    if( len >= OutLen ) {
        PrtBuff();
        len = 0;
    }
    //NYI: cheesy Unicode support
    if( len < BUFLEN ) {
        *OutPtr++ = ch;
    }
}


void EndPrintBuff( void )
{
    PrtChar( NULLCHAR );
}


static void PrtNeed( unsigned len )
{
    if( OutPtr + len > OutBuff + OutLen ) {
        PrtBuff();
    }
}


static void EndBuff( void )
{
    if( OutBuff != OutPtr || *OutPtr != NULLCHAR ) {
        PrtBuff();
    }
}


static void PrtStr( const char *start, unsigned len )
{
    PrtNeed( len );
    for( ; len > 0; --len ) {
        PrtChar( *start++ );
    }
}


/*
 * CnvRadix -- convert an unsigned number of a given radix to a string
 */

static char *CnvRadix( unsigned_64 *value, mad_radix radix, char base, char *buff, int len )
{
    char        internal[65];
    char        *ptr;
    unsigned    dig;
    unsigned_64 big_radix;
    unsigned_64 remainder;

    ptr = &internal[64];
    U32ToU64( radix, &big_radix );
    while( (len > 0) || (U64Test( value ) != 0) ) {
        U64Div( value, &big_radix, value, &remainder );
        dig = U32FetchTrunc( remainder );
        *ptr = (dig <= 9) ? dig + '0' : dig - 10 + base;
        --ptr;
        --len;
    }
    len = &internal[64] - ptr;
    memcpy( buff, ptr + 1, len );
    buff[len] = NULLCHAR;
    return( buff + len );
}


static char *FmtNum( unsigned_64 num, int radixfmt, char base_letter, sign_class sign_type, char *buff, unsigned len )
{
    char        *ptr;
    const char  *prefix;
    size_t      prefix_len;

    if( sign_type == NUM_SIGNED && I64Test( &num ) < 0 ) {
        *buff = '-';
        ++buff;
        U64Neg( &num, &num );
    }
    prefix = NULL;
    prefix_len = 0;
    if( radixfmt != DefRadix ) {
        FindRadixSpec( FMT2RADIX( radixfmt ), &prefix, &prefix_len );
    }
    ptr = buff;
    if( prefix != NULL ) {
        memcpy( ptr, prefix, prefix_len );
        ptr = buff + prefix_len;
    }
    return( CnvRadix( &num, FMT2RADIX( radixfmt ), base_letter, ptr, len ) );
}


/*
 * PrintRadix -- print expression in given radix
 */

static void PrintRadix( int radixfmt, char base_letter, sign_class sign_type )
{
    char                buff[BUFLEN];
    char                *ptr;
    mad_type_info       mti;
    size_t              buff_len;
    item_mach           item;
    mad_type_info       host_mti;
    mad_type_handle     mth;

    if( sign_type == NUM_CHECK ) {
        if( radixfmt != 10 && radixfmt != -10 ) {
            sign_type = NUM_UNSIGNED;
        } else {
            switch( ExprSP->ti.kind ) {
            case TK_BOOL:
            case TK_CHAR:
            case TK_INTEGER:
            case TK_ENUM:
                if( ExprSP->ti.modifier == TM_UNSIGNED ) {
                    sign_type = NUM_UNSIGNED;
                } else {
                    sign_type = NUM_SIGNED;
                }
                break;
            default:
                sign_type = NUM_UNSIGNED;
                break;
            }
        }
    }
    ptr = buff;
    switch( ExprSP->ti.kind ) {
    case TK_BOOL:
    case TK_CHAR:
    case TK_ENUM:
    case TK_INTEGER:
        /* this is to get proper sign extension for 16-bit numbers */
        if( sign_type == NUM_SIGNED ) {
            ExprSP->ti.modifier = TM_SIGNED;
        } else {
            ExprSP->ti.modifier = TM_UNSIGNED;
        }
#if 1
        {
            unsigned len = 1;
            /* If we are printing hex, expand to both nibbles */
            if( ( ExprSP->ti.modifier == TM_UNSIGNED ) && ( radixfmt == 16 || radixfmt == -16 ) && _IsOff( SW_DONT_EXPAND_HEX ) )
                len = ExprSP->ti.size * 2;
            ConvertTo( ExprSP, TK_INTEGER, TM_UNSIGNED, sizeof( ExprSP->v.uint ) );
            ptr = FmtNum( ExprSP->v.uint, radixfmt, base_letter, sign_type, ptr, len );
        }
#endif
        break;
    case TK_ARRAY:
    case TK_FUNCTION:
    case TK_ADDRESS:
    case TK_POINTER:
        AddrFix( &ExprSP->v.addr );
        if( ExprSP->ti.modifier == TM_NEAR ) {
            mth = MADTypeDefault( MTK_ADDRESS, MAF_OFFSET, &DbgRegs->mr, &ExprSP->v.addr );
        } else {
            mth = MADTypeDefault( MTK_ADDRESS, MAF_FULL, &DbgRegs->mr, &ExprSP->v.addr );
        }
        MADTypeInfo( mth, &mti );
        MADTypeInfoForHost( MTK_ADDRESS, sizeof( address ), &host_mti );
        MADTypeConvert( &host_mti, &ExprSP->v.addr, &mti, &item, 0 );
        buff_len = sizeof( buff );
        MADTypeToString( FMT2RADIX( radixfmt ), &mti, &item, ptr, &buff_len );
        ptr += buff_len;
        break;
    case TK_REAL:
        {
            signed_64   tmp;

            I32ToI64( LDToD( &ExprSP->v.real ), &tmp );
            ptr = FmtNum( tmp, FMT2RADIX( radixfmt ), base_letter, NUM_SIGNED, ptr, 1 );
        }
        break;
    default:
        Error( ERR_NONE, LIT_ENG( ERR_ILL_TYPE ) );
    }
    PrtStr( buff, ptr - buff );
}

/*
 * PrintAddress -- print expression as a symbolic address
 */

static void PrintAddress( char fmt )
{
    char        buff[BUFLEN];
    char        *ptr;
    address     addr;

    if( fmt == 'l' ) {
        MakeMemoryAddr( true, EXPR_CODE, &addr );
        ptr = LineAddr( &addr, buff, sizeof( buff ) );
        if( ptr == NULL ) {
            ptr = StrAddr( &addr, buff, sizeof( buff ) );
        }
    } else {
        MakeMemoryAddr( true, EXPR_DATA, &addr );
        ptr = StrAddr( &addr, buff, sizeof( buff ) );
    }
    PrtStr( buff, ptr - buff );
}


/*
 * PrintDouble -- print a real number, or a component of a complex number
 */

static void PrintDouble( char format, xreal *val )
{
    char        *start, *ptr, *back;
    char        buff[MAX_WIDTH + 1];
    bool        found_dot;

    switch( tolower( format ) ) {
    case 'f':
        LDToS( buff, val, 16, 0, MAX_WIDTH, 0, 0, 0, 'F' );
        break;
    case 'e':
        LDToS( buff, val, 16, 0, MAX_WIDTH, 1, 0, format, 'E' );
        break;
    case 'g':
        LDToS( buff, val, 16, 0, MAX_WIDTH, 1, 0, format - 2, 'G' );
        break;
    }
    buff[MAX_WIDTH] = NULLCHAR;
    for( start = buff; *start == ' '; ++start )
        ;
    found_dot = false;
    for( ptr = start; *ptr != NULLCHAR; ++ptr ) {
        if( *ptr == 'e' )
            break;
        if( *ptr == 'E' )
            break;
        if( *ptr == '.' ) {
            found_dot = true;
        }
    }
    back = ptr;
    if( found_dot ) {
        while( *--ptr == '0' )
            ;
        ++ptr;
    }
    while( (*ptr = *back) != NULLCHAR ) {
        ++ptr;
        ++back;
    }
    PrtStr( start, ptr - start );
}


/*
 * PrintReal -- print a real number
 */

static void PrintReal( char format )
{
    if( ExprSP->ti.kind == TK_REAL ) {
        if( ExprSP->ti.size == sizeof( float ) ) {
            _SetMaxPrec( PREC_FLOAT );
        } else {
            _SetMaxPrec( PREC_LONG_FLOAT );
        }
    } else {
        ConvertTo( ExprSP, TK_REAL, TM_NONE, sizeof( ExprSP->v.real ) );
        _SetMaxPrec( PREC_LONG_FLOAT );
    }
    PrintDouble( format, &ExprSP->v.real );
}


/*
 * PrintComplex -- print a complex number
 */

static void PrintComplex( char format )
{
    if( ExprSP->ti.kind == TK_COMPLEX ) {
        if( ExprSP->ti.size == sizeof( float )*2 ) {
            _SetMaxPrec( PREC_FLOAT );
        } else {
            _SetMaxPrec( PREC_LONG_FLOAT );
        }
    } else {
        ConvertTo( ExprSP, TK_COMPLEX, TM_NONE, sizeof( ExprSP->v.cmplx ) );
        _SetMaxPrec( PREC_LONG_FLOAT );
    }
    PrtChar( '(' );
    PrintDouble( format, &ExprSP->v.cmplx.re );
    PrtChar( ',' );
    PrtChar( ' ' );
    PrintDouble( format, &ExprSP->v.cmplx.im );
    PrtChar( ')' );
}


/*
 * PrintChar -- print expression as a character
 */
void PrintChar( void )
{
    PrtChar( '\'' );
    switch( ExprSP->ti.kind ) {
    case TK_BOOL:
    case TK_ENUM:
    case TK_INTEGER:
        PrtChar( (char) U32FetchTrunc( ExprSP->v.uint ) );
        break;
    case TK_CHAR:
        switch( ExprSP->ti.kind ) {
        case 4:
            /* 4 byte Unicode */
            PrtChar( (unsigned_32) U32FetchTrunc( ExprSP->v.uint ) );
            break;
        case 2:
            /* 2 byte Unicode */
            PrtChar( (unsigned_16) U32FetchTrunc( ExprSP->v.uint ) );
            break;
        default:
            PrtChar( (char) U32FetchTrunc( ExprSP->v.uint ) );
            break;
        }
        break;
    case TK_ADDRESS:
    case TK_POINTER:
        PrtChar( (char) ExprSP->v.addr.mach.offset );
        break;
    default:
        Error( ERR_NONE, LIT_ENG( ERR_ILL_TYPE ) );
    }
    PrtChar( '\'' );
}

/*
 * PrintString -- print expression as a string
 */

#define MAX_PRINTSTRING_LEN  100

static void DoPrintString( bool force )
{
    size_t      count;
    address     addr;
    char        buff[MAX_PRINTSTRING_LEN + 10];
    char        *p;

    MakeMemoryAddr( false, EXPR_DATA, &addr );
    if( IS_NIL_ADDR( addr ) && !force ) {
        Error( ERR_NONE, LIT_ENG( ERR_NOT_PRINTABLE ), addr );
    }
    count = ProgPeek( addr, buff, MAX_PRINTSTRING_LEN );
    if( count == 0 && !force ) {
        Error( ERR_NONE, LIT_ENG( ERR_NOT_PRINTABLE ), addr );
    }
    for( p = buff; *p != NULLCHAR; ++p ) {
        if( count == 0 ) {
            if( !force ) {
                Error( ERR_NONE, LIT_ENG( ERR_NOT_PRINTABLE ), addr );
            }
            PrtStr( " ...", 4 );
            break;
        }
        PrtChar( *p );
        --count;
    }
}


void PrintString( void )
{
    DoPrintString( false );
}


void ForcePrintString( void )
{
    DoPrintString( true );
}


static void PrintCharBlock( void )
{
    char        *ascii_start;
    unsigned_16 *unicode_start;
    unsigned    len;
    int         overflow = 0;

    PrtChar( '\'' );
    ascii_start = ExprSP->v.string.loc.e[0].u.p;
    len = ExprSP->ti.size;

    /*
     *  If the memory required to display the string is larger than what we have to display, then
     *  we adjust things so we can display them with a hint that the string is longer than can be displayed
     */

    if( len + 2 > BUFLEN ) {    /* or UTIL_LEN/TXT_LEN? */
        len = BUFLEN - 7;       /* 'string'....<NUL> */
        overflow = 1;
    }

    switch( ExprSP->ti.modifier ) {
    case TM_NONE:
    case TM_ASCII:
        PrtNeed( len );
        for( ; len > 0; --len ) {
            if( *ascii_start == NULLCHAR )
                break;
            PrtChar( *ascii_start++ );
        }
        break;
    case TM_EBCIDIC:
        //NYI:
        break;
    case TM_UNICODE:
        unicode_start = (unsigned_16 *)ascii_start;
        len /= 2;
        PrtNeed( len );
        for( ; len > 0; --len ) {
            if( *unicode_start == NULLCHAR )
                break;
            PrtChar( *unicode_start++ );
        }
        break;
    }
    PrtChar( '\'' );
    if( overflow && len == 0 ) {
        PrtStr( " ...",  4 );
    }
}

static void GetExpr( void )
{
    if( !First && CurrToken == T_COMMA )
        Scan();
    NormalExpr();
    if( CurrToken != T_COMMA && CurrToken != T_LEFT_BRACE )
        ReqEOC();
    First = false;
}

/*
 * DoFormat -- process a format string
 */
static void DoFormat( const char *fmt_ptr, const char *fmt_end )
{
    char        buff[BUFLEN+1];
    char        z_format;

    StartPrintBuff( buff, BUFLEN );
    First = true;
    while( fmt_ptr < fmt_end ) {
        if( (*fmt_ptr != '%') || (fmt_end - fmt_ptr == 1) ) {
            PrtChar( *fmt_ptr );
        } else {
            ++fmt_ptr;
            switch( *fmt_ptr ) {
            case 'z':
            case 'Z':
                if( fmt_end - fmt_ptr == 1 ) {
                    z_format = 'G';
                } else {
                    ++fmt_ptr;
                    switch( *fmt_ptr ) {
                    case 'f':
                    case 'e':
                    case 'E':
                    case 'g':
                    case 'G':
                        z_format = *fmt_ptr;
                        break;
                    default:
                        --fmt_ptr;
                        z_format = 'G';
                        break;
                    }
                }
                GetExpr();
                PrintComplex( z_format );
                PopEntry();
                break;
            case 'd' :
            case 'i' :
                GetExpr();
                PrintRadix( 10, NULLCHAR, NUM_SIGNED );
                PopEntry();
                break;
            case 'u' :
                GetExpr();
                PrintRadix( 10, NULLCHAR, NUM_UNSIGNED );
                PopEntry();
                break;
            case 'o' :
                GetExpr();
                PrintRadix( 8, NULLCHAR, NUM_UNSIGNED );
                PopEntry();
                break;
            case 'x' :
                GetExpr();
                PrintRadix( 16, 'a', NUM_UNSIGNED );
                PopEntry();
                break;
            case 'X' :
                GetExpr();
                PrintRadix( 16, 'A', NUM_UNSIGNED );
                PopEntry();
                break;
            case 'p' :
                GetExpr();
                PrintRadix( -16, 'A', NUM_UNSIGNED );
                PopEntry();
                break;
            case 'c' :
                GetExpr();
                PrintChar();
                PopEntry();
                break;
            case 's' :
                GetExpr();
                PrintString();
                PopEntry();
                break;
            case 'f':
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                GetExpr();
                PrintReal( *fmt_ptr );
                PopEntry();
                break;
            case 'r':
                GetExpr();
                PrintRadix( CurrRadix, 'A', NUM_CHECK );
                PopEntry();
                break;
            case 'a':
            case 'l':
                GetExpr();
                PrintAddress( *fmt_ptr );
                break;
            case '%':
                PrtChar( '%' );
                break;
            default :
                PrtChar( '%' );
                --fmt_ptr;
            }
        }
        ++fmt_ptr;
    }
    EndBuff();
}

typedef struct {
    bool        first_time;
} print_fld;


static walk_result PrintDlgField( sym_walk_info swi, sym_handle *member_hdl, void *_d )
{
    print_fld   *d = _d;
    char        *name;
    unsigned    len;

    if( swi == SWI_SYMBOL ) {
        if( !d->first_time ) {
            /* not the first time through */
            PrtChar( ',' );
            /* print a space if not at start of a line */
            if( OutPtr != OutBuff ) {
                PrtChar( ' ' );
            }
        }
        d->first_time = false;
        DupStack();
        len = DIPSymName( member_hdl, NULL, SNT_SOURCE, NULL, 0 );
        _AllocA( name, len + 1 );
        len = DIPSymName( member_hdl, NULL, SNT_SOURCE, name, len + 1 );
        PrtStr( name, len );
        PrtChar( '=' );
        DoGivenField( member_hdl );
        ExprValue( ExprSP );
        PrintValue();
        PopEntry();
    }
    return( WR_CONTINUE );
}

static void PrintStruct( void )
{
    print_fld   d;

    d.first_time = true;
    PrtChar( '{' );
    DIPWalkSymList( SS_TYPE, ExprSP->th, PrintDlgField, &d );
    PrtChar( '}' );
}

static void PrintArray( void )
{
    array_info          ai;
    char                *ptr;
    unsigned            len;
    char                buff[TXT_LEN];
    bool                first_time;
    signed_64           tmp;

    first_time = true;
    PrtChar( '{' );
    DIPTypeArrayInfo( ExprSP->th, ExprSP->lc, &ai, NULL );
    while( ai.num_elts != 0 ) {
        ChkBreak();
        if( !first_time ) {
            /* not the first time through */
            PrtChar( ',' );
            /* print a space if not at start of a line */
            if( OutPtr != OutBuff ) {
                PrtChar( ' ' );
            }
        }
        DupStack();
        StartSubscript();
        PushNum( ai.low_bound );
        AddSubscript();
        EndSubscript();
        ExprValue( ExprSP );
        buff[0] = '[';
        I32ToI64( ai.low_bound, &tmp );
        ptr = FmtNum( tmp, CurrRadix, 'A', NUM_SIGNED, &buff[1], 1 );
        *ptr++ = ']';
        *ptr++ = '=';
        len = ptr - buff;
        PrtStr( buff, len );
        PrintValue();
        PopEntry();
        ai.num_elts--;
        ai.low_bound++;
        first_time = false;
    }
    PrtChar( '}' );
}

struct val2name {
    unsigned_64         value;
    unsigned_64         best_value;
    sym_handle          *sh;
    bool                found;
};

static walk_result ExactMatch( sym_walk_info swi, sym_handle *sh, void *d )
{
    struct val2name     *vd = d;
    unsigned_64         val;

    if( swi != SWI_SYMBOL )
        return( WR_CONTINUE );
    U64Clear( val );
    if( DIPSymValue( sh, ExprSP->lc, &val ) != DS_OK )
        return( WR_STOP );
    if( U64Cmp( &val, &vd->value ) != 0 )
        return( WR_CONTINUE );
    HDLAssign( sym, vd->sh, sh );
    vd->found = true;
    return( WR_STOP );
}

static walk_result BestMatch( sym_walk_info swi, sym_handle *sh, void *d )
{
    struct val2name     *vd = d;
    unsigned_64         val;
    unsigned_64         tmp;

    if( swi != SWI_SYMBOL )
        return( WR_CONTINUE );
    U64Clear( val );
    if( DIPSymValue( sh, ExprSP->lc, &val ) != DS_OK )
        return( WR_STOP );
    if( U64Test( &val ) == 0 )
        return( WR_CONTINUE );
    U64And( &val, &vd->value, &tmp );
    if( U64Cmp( &tmp, &val ) == 0 ) {
        if( !vd->found || U64Cmp( &val, &vd->best_value ) > 0 ) {
            HDLAssign( sym, vd->sh, sh );
            vd->best_value = val;
            vd->found = true;
        }
    }
    return( WR_CONTINUE );
}


static unsigned ValueToName( char *buff, unsigned len )
{
    char                *p;
    unsigned            name_len;
    struct val2name     d;
    DIPHDL( sym, sh );

    d.sh = sh;
    d.found = false;
    d.value = ExprSP->v.uint;
    DIPWalkSymList( SS_TYPE, ExprSP->th, ExactMatch, &d );
    if( d.found ) {
        return( DIPSymName( sh, NULL, SNT_SOURCE, buff, len ) );
    }
    p = buff;
    while( U64Test( &d.value ) != 0 ) {
        d.found = false;
        DIPWalkSymList( SS_TYPE, ExprSP->th, BestMatch, &d );
        if( !d.found )
            return( 0 );
        U64Not( &d.best_value, &d.best_value );
        U64And( &d.value, &d.best_value, &d.value );
        if( p != buff ) {
            if( len == 0 )
                return( 0 );
            *p++ = '+';
            --len;
        }
        name_len = DIPSymName( sh, NULL, SNT_SOURCE, p, len );
        if( name_len > len )
            return( 0 );
        p += name_len;
        len -= name_len;
    }
    return( p - buff );
}

void PrintValue( void )
{
    char                buff[TXT_LEN];
    char                *p;
    const char          *tstr;
    size_t              tlen;

    switch( ExprSP->ti.kind ) {
    case TK_VOID:
        break;
    case TK_ENUM:
        tlen = ValueToName( buff, TXT_LEN );
        if( tlen != 0 ) {
            /* we have a symbolic name for the value, print that out */
            PrtStr( buff, tlen );
            break; /* from switch */
        }
        PrintRadix( CurrRadix, 'A', NUM_CHECK );
        break;
    case TK_CHAR:
    case TK_BOOL:
    case TK_INTEGER:
        PrintRadix( CurrRadix, 'A', NUM_CHECK );
        break;
    case TK_ADDRESS:
        if( !IS_NIL_ADDR( ExprSP->v.addr ) ) {
            p = StrAddr( &ExprSP->v.addr, buff, sizeof( buff ) );
            PrtStr( buff, p - buff );
            break;
        }
        /* fall through */
    case TK_POINTER:
        if( IS_NIL_ADDR( ExprSP->v.addr ) ) {
            SetTokens( true );
            if( !TokenName( T_SSL_SPEC_NULL, &tstr, &tlen ) ) {
                tlen = 0;
            }
            SetTokens( false );
            if( tlen != 0 ) {
                PrtStr( tstr + 1, tlen - 1 );
            } else {
                PrintRadix( -16, 'A', NUM_UNSIGNED );
            }
        } else {
            PrintRadix( -16, 'A', NUM_UNSIGNED );
        }
        break;
    case TK_ARRAY:
        PrintArray();
        break;
    case TK_STRUCT:
        PrintStruct();
        break;
    case TK_REAL:
        PrintReal( 'G' );
        break;
    case TK_COMPLEX:
        PrintComplex( 'G' );
        break;
    case TK_STRING:
        PrintCharBlock();
        break;
    default:
        Error( ERR_NONE, LIT_ENG( ERR_ILL_TYPE ) );
    }
}


static void DoDefault( void )
{
    char        buff[BUFLEN+1];

    StartPrintBuff( buff, BUFLEN );
    GetExpr();
    PrintValue();
    PopEntry();
    EndBuff();
}

void DoPrintList( bool output )
{
    const char  *fmt_start;
    size_t      fmt_len;

    OutPgm = output;
    First = true;
    while( !ScanEOC() ) {
        if( ScanQuote( &fmt_start, &fmt_len ) ) {
            DoFormat( fmt_start, fmt_start + fmt_len );
        } else {
            DoDefault();
        }
    }
}

static void LogPrintList( void )
{
    if( _IsOn( SW_CMD_INTERACTIVE ) ) {
        DUIShowLogWindow();
    }
    DoPrintList( false );
}

void ChkPrintList( void )
{
    bool        first;
    const char  *start;
    size_t      len;

    first = true;
    while( !ScanEOC() ) {
        if( ScanQuote( &start, &len ) )
            first = true;
        if( !first && CurrToken == T_COMMA )
            Scan();
        ChkExpr();
        if( CurrToken != T_COMMA )
            ReqEOC();
        first = false;
    }
}



/*
 * ProcPrint -- process print command
 */

static char     PrintBuff[TXT_LEN];
void ProcPrint( void )
{
    const char      *old;

    if( CurrToken == T_DIV ) {
        Scan();
        switch( ScanCmd( PrintOps ) ) {
        case PRINT_PROGRAM:
            DoPrintList( true );
            break;
        case PRINT_WINDOW:
            GraphicDisplay();
            break;
        default:
            Error( ERR_LOC, LIT_ENG( ERR_BAD_OPTION ), GetCmdName( CMD_PRINT ) );
            break;
        }
    } else {
        if( ScanEOC() ) {
            DlgNewWithSym( LIT_ENG( New_Expression ), PrintBuff, TXT_LEN );
            if( PrintBuff[0] != NULLCHAR ) {
                old = ReScan( PrintBuff );
                LogPrintList();
                ReScan( old );
            }
        } else {
            LogPrintList();
        }
    }
}
