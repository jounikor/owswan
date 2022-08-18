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
* Description:  Debugger lexical scanner.
*
****************************************************************************/


#include <ctype.h>
#include <stdlib.h>
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbgerr.h"
#include "dbgmem.h"
#include "dbglit.h"
#include "ldsupp.h"
#include "dui.h"
#include "i64.h"
#include "trpld.h"
#include "strutil.h"
#include "dbgscan.h"
#include "dbgmad.h"
#include "numscan.h"
#include "dbgutil.h"
#include "madinter.h"
#include "madcli.h"
#include "dbgshow.h"
#include "dbgparse.h"
#include "dbgupdt.h"
#include "dbglkup.h"
#include "dbgsem.h"
#include "dbgsetfg.h"

#include "clibext.h"


typedef struct radix_str {
    struct radix_str *next;
    mad_radix        value;
    char             string[1];         /* first byte is length */
} radix_str;

typedef union {
    unsigned_64     int_val;
    xreal           real_val;
} token_value;

bool            scan_string = false;
char            *StringStart = NULL;
dig_type_size   StringLength = 0;
bool            ScanCCharNum = true;

static const char CmdLnDelimTab[] = {
    #define pick(chr,tok)   chr
    #include "_dbgtok.h"
    #undef pick
};

static  radix_str       *RadixStrs;
static  const char      *ScanPtr;
static  token_value     TokenVal;
static  const char      *TokenStart;
static  token_table     *ExprTokens;

static void SetRadixSpec( const char *str, size_t len, mad_radix value, bool clear )
{
    radix_str   *radixstr;
    radix_str   **owner;

    owner = &RadixStrs;
    for( radixstr = RadixStrs; radixstr != NULL; radixstr = radixstr->next ) {
        if( SYM_NAME_LEN( radixstr->string ) == len
          && strnicmp( SYM_NAME_NAME( radixstr->string ), str, len ) == 0 )
            break;
        if( SYM_NAME_LEN( radixstr->string ) < len )
            break;
        owner = &radixstr->next;
    }
    if( radixstr == NULL || SYM_NAME_LEN( radixstr->string ) != len ) {
        if( clear )
            return;
        radixstr = DbgMustAlloc( sizeof( radix_str ) + len );
        memcpy( SYM_NAME_NAME( radixstr->string ), str, len );
        SET_SYM_NAME_LEN( radixstr->string, len );
        radixstr->next = *owner;
        *owner = radixstr;
    } else if( clear ) {
        *owner = radixstr->next;
        _Free( radixstr );
        return;
    }
    radixstr->value = value;
}


/*
 * InitScan -- initialize scanner
 */

void InitScan( void )
{
    ScanPtr = LIT_ENG( Empty );
    TokenStart = ScanPtr;
    CurrToken = T_LINE_SEPARATOR;
    RadixStrs = NULL;
    SetRadixSpec( "0x", 2, 16, false );
    SetRadixSpec( "0n", 2, 10, false );
}


void FiniScan( void )
{
    radix_str   *radixstr;

    while( (radixstr = RadixStrs) != NULL ) {
        RadixStrs = RadixStrs->next;
        _Free( radixstr );
    }
}


/*
 * ScanPos -- return the current scan position
 */

const char *ScanPos( void )
{
    return( TokenStart );
}



/*
 * ScanLen -- return the length of current token
 */

size_t ScanLen( void )
{
    return( ScanPtr - TokenStart );
}



/*
 * ScanCmd -- scan a command start of current token, looking up in given table
 */

int ScanCmd( const char *cmd_table )
{
    int         ind;
    const char  *saveptr;

    saveptr = ScanPtr;
    ScanPtr = TokenStart;
    while( isalpha( *ScanPtr ) || *ScanPtr == '_' ) {
        ++ScanPtr;
    }
    ind = Lookup( cmd_table, TokenStart, ScanPtr - TokenStart );
    if( ind < 0 ) {
        ScanPtr = saveptr;
    } else {
        Scan();
    }
    return( ind );
}

struct type_name {
    const char          *start;
    unsigned            len;
    mad_type_handle     mth;
};

static walk_result      FindTypeName( mad_type_handle mth, void *d )
{
    struct type_name    *nd = d;
    const char          *p;
    char                *q;
    unsigned            len;

    GetMADTypeNameForCmd( mth, TxtBuff, TXT_LEN );
    for( p = nd->start, q = TxtBuff; tolower( *p ) == tolower( *q ); ++p, ++q ) {
        if( *q == NULLCHAR ) {
            break;
        }
    }
    if( isalnum( *p ) )
        return( WR_CONTINUE );
    len = q - TxtBuff;
    if( *q == NULLCHAR ) {
        /* an exact match */
        nd->len = len;
        nd->mth = mth;
        return( WR_STOP );
    }
    if( len > nd->len ) {
        nd->len = len;
        nd->mth = mth;
    }
    return( WR_CONTINUE );
}

static mad_type_handle DoScanType( mad_type_kind tk, char *prefix )
{
    struct type_name    data;
    size_t              len;

    len = strlen( prefix );
    if( strnicmp( TokenStart, prefix, len ) != 0 ) {
        return( MAD_NIL_TYPE_HANDLE );
    }
    data.start = TokenStart + len;
    data.len = 0;
    data.mth = MAD_NIL_TYPE_HANDLE;
    MADTypeWalk( tk, FindTypeName, &data );
    if( data.mth != MAD_NIL_TYPE_HANDLE )
        ReScan( data.start + data.len );
    return( data.mth );
}

mad_type_handle ScanType( mad_type_kind tk, mad_type_kind *tkr )
{
    mad_type_handle     mth;

    mth = MAD_NIL_TYPE_HANDLE;
    if( tk & MAS_MEMORY ) {
        mth = DoScanType( tk & ~MAS_IO, LIT_ENG( Empty ) );
        if( mth != MAD_NIL_TYPE_HANDLE ) {
            if( tkr != NULL )
                *tkr = MAS_MEMORY;
            return( mth );
        }
    }
    if( tk & MAS_IO ) {
        mth = DoScanType( tk & ~MAS_MEMORY, LIT_ENG( IO ) );
        if( mth != MAD_NIL_TYPE_HANDLE ) {
            if( tkr != NULL )
                *tkr = MAS_IO;
            return( mth );
        }
    }
    return( mth );
}

mad_string ScanCall( void )
{
    const char          *p;
    char                *q;
    const mad_string    *type;
    char                buff[NAM_LEN];

    for( type = MADCallTypeList(); *type != MAD_MSTR_NIL; ++type ) {
        MADCli( String )( *type, buff, sizeof( buff ) );
        for( q = buff, p = TokenStart; ; ++p, ++q ) {
            if( !isalnum( *p ) ) {
                if( p == TokenStart )
                    return( MAD_MSTR_NIL );
                ReScan( p );
                return( *type );
            }
            if( tolower( *p ) != tolower( *q ) ) {
                break;
            }
        }
    }
    return( MAD_MSTR_NIL );
}

/*
 * ScanEOC -- check if at end of command
 */

bool ScanEOC( void )
{
    return( CurrToken == T_CMD_SEPARATOR || CurrToken == T_LINE_SEPARATOR );
}


static bool FindToken( const char *table, tokens token,
                       const char **start, size_t *len )
{
    while( *table != NULLCHAR ) {
        *start = table;
        for( ; *table != NULLCHAR; ++table )
            ;
        if( (tokens)GETWORD( table + 1 ) == token ) {
            *len = table - *start;
            return( true );
        }
        table += 3;
    }
    return( false );
}


bool TokenName( tokens token, const char **start, size_t *len )
{
    switch( token ) {
    case T_LINE_SEPARATOR:
        *start = LIT_ENG( End_Of_Line );
        *len = strlen( LIT_ENG( End_Of_Line ) ) + 1;
        return( true );
    case T_INT_NUM:
    case T_REAL_NUM:
        *start = LIT_ENG( Num_Name );
        *len = strlen( LIT_ENG( Num_Name ) ) + 1;
        return( true );
    case T_NAME:
        *start = LIT_ENG( Sym_Name_Name );
        *len = strlen( LIT_ENG( Sym_Name_Name ) ) + 1;
        return( true );
    }
    if( token >= FIRST_CMDLN_DELIM && token < ( FIRST_CMDLN_DELIM + SIZE_CMDLN_DELIM ) ) {
        *start = CmdLnDelimTab + token - FIRST_CMDLN_DELIM;
        *len = sizeof( char );
        return( true );
    }
    if( ExprTokens != NULL ) {
        if( FindToken( ExprTokens->delims, token, start, len ) )
            return( true );
        if( FindToken( ExprTokens->keywords, token, start, len ) ) {
            return( true );
        }
    }
    return( false );
}


void Recog( tokens token )
{
    const char  *start;
    size_t      len;

    if( token != CurrToken ) {
        TokenName( token, &start, &len );
        Error( ERR_LOC, LIT_ENG( ERR_WANT_TOKEN ), start, len );
    }
    Scan();
}


/*
 * ScanQuote -- scan a debugger quoted string
 */

bool ScanQuote( const char **start, size_t *len )
{
    int   cnt;

    if( CurrToken != T_LEFT_BRACE ) {
        *start = NULL;
        *len   = 0;
        return( false );
    }
    *start = ScanPtr;
    cnt = 1;
    while( cnt > 0 ) {
        Scan();
        if( CurrToken == T_LEFT_BRACE ) {
            cnt += 1;
        } else if( CurrToken == T_RIGHT_BRACE ) {
            cnt -= 1;
        } else if( CurrToken == T_LINE_SEPARATOR ) {
            Recog( T_RIGHT_BRACE ); /* cause error */
        }
    }
    *len = TokenStart - *start;
    Scan();
    return( true );
}


/*
 * ScanItem - scan to a blank or EOC
 */

bool ScanItem( bool blank_delim, const char **start, size_t *len )
{
    if( ScanEOC() ) {
        *start = NULL;
        *len   = 0;
        return( false );
    }
    if( ScanQuote( start, len ) )
        return( true );
    *start = TokenStart;
    for( ;; ) {
        if( blank_delim && isspace( *ScanPtr ) )
            break;
        if( *ScanPtr == TRAP_PARM_SEPARATOR )
            break;
        if( *ScanPtr == ';' )
            break;
        if( *ScanPtr == NULLCHAR )
            break;
        ++ScanPtr;
    }
    *len = ScanPtr - TokenStart;
    Scan();
    return( true );
}


/*
 * ScanItemDelim - scan to one of delimiter characters or EOC
 */

bool ScanItemDelim( const char *delim, bool blank_delim, const char **start, size_t *len )
{
    if( ScanEOC() ) {
        *start = NULL;
        *len   = 0;
        return( false );
    }
    if( ScanQuote( start, len ) )
        return( true );
    *start = TokenStart;
    for( ;; ) {
        if( blank_delim && isspace( *ScanPtr ) )
            break;
        if( strchr( delim, *ScanPtr ) != NULL )
            break;
        if( *ScanPtr == NULLCHAR )
            break;
        ++ScanPtr;
    }
    *len = ScanPtr - TokenStart;
    Scan();
    return( true );
}


/*
 * ReqEOC -- require end of command
 */

void ReqEOC( void )
{
    if( !ScanEOC() ) {
        Error( ERR_LOC, LIT_ENG( ERR_WANT_EOC ) );
    }
}


/*
 * ReqEOC -- require end of command
 */

void FlushEOC( void )
{
    while( !ScanEOC() ) {
        Scan();
    }
}


static bool ScanExprDelim( const char *table )
{
    const char  *ptr;

    for( ; *table != NULLCHAR; table += 3 ) {
        for( ptr = ScanPtr; ( _IsOn( SW_SSL_CASE_SENSITIVE ) ?
                *table == *ptr : toupper(*table) == toupper(*ptr) )
                && *table != NULLCHAR; ptr++, table++ ) {
            ;
        }
        if( *table == NULLCHAR ) {
            table++;
            CurrToken = (tokens)GETWORD( table );
            ScanPtr = ptr;
            return( true );
        }
        while( *table != NULLCHAR ) {
            table++;
        }
    }
    return( false );
}


static bool ScanCmdLnSepar( void )
{
    if( *ScanPtr == NULLCHAR ) {
        CurrToken = T_LINE_SEPARATOR;
        return( true );
    }
    return( false );
}


static bool ScanCmdLnDelim( void )
{
    const char  *ptr;

    for( ptr = CmdLnDelimTab; *ptr != NULLCHAR; ptr++ ) {
        if( *ptr == *ScanPtr ) {
            CurrToken = ptr - CmdLnDelimTab + FIRST_CMDLN_DELIM;
            ScanPtr++;
            return( true );
        }
    }
    return( false );
}


/*
 * ScanRealNum -- try to scan a real number
 */

static bool ScanRealNum( void )
{
    const char  *curr;

    curr = ScanPtr;
    while( isdigit( *curr ) )
        ++curr;
    if( *curr != '.' )
        return( false );
    SToLD( ScanPtr, &ScanPtr, &TokenVal.real_val );
    if( curr == ScanPtr ) {    /* it isn't a number, it's just a dot */
        ScanPtr++;
        return( false );
    }
    CurrToken = T_REAL_NUM;
    return( true );
}


/*
 * GetDig -- get value of a digit
 */

static int GetDig( unsigned base )
{
    char     chr;

    chr = *ScanPtr;
    chr = toupper( chr );
    if( ( (chr < '0') || (chr > '9') ) && ( (chr < 'A') || (chr > 'Z') ) )
        return( -1 );
    if( chr >= 'A' )
        chr -= 'A' - '0' - 10;
    if( chr - '0' >= base )
        return( -1 );
    return( chr - '0' );
}



/*
 * GetNum -- evaluate a number
 */

static bool GetNum( unsigned base )
{
    bool        ok;
    unsigned_64 num;
    unsigned_64 big_base;
    unsigned_64 big_dig;
    int         dig;

    ok = false;
    U32ToU64( base, &big_base );
    U64Clear( num );
    while( (dig = GetDig( base )) >= 0 ) {
        U32ToU64( dig, &big_dig );
        U64Mul( &num, &big_base, &num );
        U64Add( &num, &big_dig, &num );
        ++ScanPtr;
        ok = true;
    }
    TokenVal.int_val = num;
    return( ok );
}



/*
 * ScanNumber -- scan for a number
 */

static bool ScanNumber( void )
{
    radix_str   *radixstr;
    bool        ret;
    const char  *hold_scan;

    ret = false;
    hold_scan = ScanPtr;
    if( ScanCCharNum && (*ScanPtr == '\'') ) {
        if( ScanPtr[1] != NULLCHAR && ScanPtr[2] == '\'' ) {
            U32ToU64( ScanPtr[1], &TokenVal.int_val );
            ScanPtr += 3;
            CurrToken = T_INT_NUM;
            return( true );
        }
    } else {
        CurrToken = T_BAD_NUM; /* assume we'll find a bad number */
        for( radixstr = RadixStrs; radixstr != NULL; radixstr = radixstr->next ) {
            if( strnicmp( ScanPtr, SYM_NAME_NAME( radixstr->string ), SYM_NAME_LEN( radixstr->string ) ) == 0 ) {
                ret = true;
                ScanPtr += SYM_NAME_LEN( radixstr->string );
                hold_scan = ScanPtr;
                if( GetNum( radixstr->value ) ) {
                    CurrToken = T_INT_NUM;
                    return( true );
                }
                ScanPtr -= SYM_NAME_LEN( radixstr->string );
            }
        }
        if( isdigit( *ScanPtr ) && GetNum( CurrRadix ) ) {
            CurrToken = T_INT_NUM;
            return( true );
        }
    }
    ScanPtr = hold_scan;
    return( ret );
}

#define NAME_ESC        '`'

const char *NamePos( void )
{
    if( *TokenStart == NAME_ESC )
        return( TokenStart + 1 );
    return( TokenStart );
}

size_t NameLen( void )
{
    const char  *end;
    const char  *start;

    end = ScanPtr;
    start = TokenStart;
    if( *start == NAME_ESC ) {
        ++start;
        if( end[-1] == NAME_ESC ) {
            --end;
        }
    }
    if( start >= end ) {
        return( 0 );
    } else {
        return( end - start );
    }
}

/*
 * ScanId -- scan for an identifier
 */

static bool ScanId( void )
{
    char        c;

    c = *ScanPtr;
    if( c == NAME_ESC ) {
        while( (c = *++ScanPtr) != NULLCHAR ) {
            if( c == NAME_ESC ) {
                ++ScanPtr;
                break;
            }
        }
    } else {
        while ( c == '_'  || c == '$' || isalnum( c ) ) {
             c = *++ScanPtr;
        }
    }
    if( NameLen() == 0 )
        return( false );
    CurrToken = T_NAME;
    return( true );
}


static bool ScanKeyword( const char *table )
{
    size_t  namelen;
    size_t  keylen;

    namelen = ScanPtr - TokenStart;
    for( ; *table != NULLCHAR; table += (keylen + 3) ) {
         keylen = strlen( table );
         if( keylen == namelen && ( _IsOn( SW_SSL_CASE_SENSITIVE )  ?
                strncmp( table, TokenStart, namelen ) == 0 :
                strnicmp( table, TokenStart, namelen ) == 0 ) ) {
             table += (namelen + 1);
             CurrToken = (tokens)GETWORD( table );
             return( true );
         }
    }
    return( false );
}


const char *ReScan( const char *point )
{
    const char  *old;

    old = TokenStart;
    ScanPtr = point;
    if( point != NULL )
        Scan();
    return( old );
}


void ScanExpr( token_table *tbl )
{
    ExprTokens = tbl;
    ReScan( TokenStart );
}


void AddActualChar( char data )
{
    char    *hold, *walk1, *walk2;
    size_t  len;

    len = ++StringLength;
    _Alloc( hold, len );
    if( hold == NULL ) {
        _Free( StringStart );
        Error( ERR_NONE, LIT_ENG( ERR_NO_MEMORY_FOR_EXPR ) );
    }
    walk1 = hold;
    walk2 = StringStart;
    for( --len; len > 0; --len )
        *walk1++ = *walk2++;
    *walk1 = data;
    _Free( StringStart );
    StringStart = hold;
}


void AddChar( void )
{
    AddActualChar( *ScanPtr );
}


void AddCEscapeChar( void )
{
    static char escape_seq[] = "\n\t\v\b\r\f\a\\\?\'\"\0";
                                /* the order above must match with SSL file */

    AddActualChar( escape_seq[CurrToken - FIRST_SSL_ESCAPE_CHAR] );
}


static void RawScan( void )
{
    if( ScanPtr[-1] == NULLCHAR ) {
        /* missing end quote; scanned past eol -- error */
        _Free( StringStart );
        StringStart = NULL;   /* this is necessary */
        StringLength = 0;
        scan_string = false;  /* this is essential */
        Error( ERR_NONE, LIT_ENG( ERR_WANT_END_STRING ) );
    }
    if( *ScanPtr != NULLCHAR ) {
        TokenStart = ScanPtr;
        CurrToken = T_UNKNOWN;
        if( ExprTokens != NULL ) {
            ScanExprDelim( ExprTokens->delims );
        }
        ScanPtr = TokenStart;
    } else {
        TokenStart = ScanPtr;   /* this is necessary */
        CurrToken = T_LINE_SEPARATOR;
    }
}


/*
 * Scan -- scan a token
 */

void Scan( void )
{
    if( !scan_string ) {
        while( isspace( *ScanPtr ) ) {
            ++ScanPtr;
        }
        TokenStart = ScanPtr;
        if( ExprTokens != NULL ) {
            if( ScanExprDelim( ExprTokens->delims ) ) {
                return;
            }
        }
        if( ScanCmdLnSepar() )
            return;
        if( ScanCmdLnDelim() )
            return;   /*sf do this if the others fail */
        if( ScanRealNum() )
            return;
        if( ScanNumber() )
            return;
        if( ScanId() ) {
            if( ExprTokens != NULL && CurrToken == T_NAME ) {
                ScanKeyword( ExprTokens->keywords );
            }
            return;
        }
        ++ScanPtr;
        CurrToken = T_UNKNOWN;
    } else {
        RawScan();
    }
}


void RawScanInit( void )
{
    ScanPtr = TokenStart;
    CurrToken = T_UNKNOWN;
}

char RawScanChar( void )
{
    return( *ScanPtr );
}

void RawScanAdvance( void )
{
    if( *ScanPtr != NULLCHAR ) {
        ++ScanPtr;
    }
}

void RawScanFini( void )
{
    TokenStart = ScanPtr;
    Scan();
}

/*
 * IntNumVal -- return a integer number's value
 */

unsigned_64 IntNumVal( void )
{
    return( TokenVal.int_val );
}


/*
 * RealNumVal -- return a real number's value
 */

xreal RealNumVal( void )
{
    return( TokenVal.real_val );
}


/*
 * NewCurrRadix - use when you know there's no scanning in progress
 */

mad_radix NewCurrRadix( mad_radix new_radix )
{
    mad_radix   old_radix;

    old_radix = CurrRadix;
    CurrRadix = new_radix;
    return( old_radix );
}


/*
 * SetCurrRadix - set the current number radix
 */

mad_radix SetCurrRadix( mad_radix new_radix )
{
    mad_radix   old_radix;

    old_radix = NewCurrRadix( new_radix );
    ReScan( TokenStart );
    return( old_radix );
}


void RestoreRadix( void )
{
    SetCurrRadix( DefRadix );
}


void DefaultRadixSet( mad_radix new_radix )
{
    DefRadix = new_radix;
    CurrRadix = new_radix;
    DbgUpdate( UP_RADIX_CHANGE );
}

/*
 * RadixSet - set the default numeric radix
 */

void RadixSet( void )
{
    unsigned   value;
    mad_radix  old_radix;

    old_radix = SetCurrRadix( 10 ); /* radix sets are always base 10 */
    value = ReqExpr();
    ReScan( TokenStart );
    ReqEOC();
    if( value < 2 || value > 36 ) {
        Error( ERR_NONE, LIT_ENG( ERR_BAD_RADIX ), value );
    }
    SetCurrRadix( old_radix );
    DefaultRadixSet( (mad_radix)value );
}


void RadixConf( void )
{
    CnvULongDec( DefRadix, TxtBuff, TXT_LEN );
    ConfigLine( TxtBuff );
}


void FindRadixSpec( mad_radix value, const char **start, size_t *len )
{
    radix_str   *radixstr;

    *start = NULL;
    *len = 0;
    for( radixstr = RadixStrs; radixstr != NULL; radixstr = radixstr->next ) {
        if( radixstr->value == value ) {
            *start = SYM_NAME_NAME( radixstr->string );
            *len   = SYM_NAME_LEN( radixstr->string );
            return;
        }
    }
}


char *AddHexSpec( char *p )
{
    const char  *pref;
    size_t      len;

    if( CurrRadix == 16 )
        return( p );
    FindRadixSpec( 16, &pref, &len );
    memcpy( p, pref, len );
    p += len;
    *p = NULLCHAR;
    return( p );
}


/*
 * ForceSym2Num -- try to force an unknown symbol into a number
 */

bool ForceSym2Num( const char *start, size_t len, unsigned_64 *val_ptr )
{
    const char  *old_scanptr;
    const char  *old_tokenstart;
    token_value old_token_val;
    bool        rtn;

    old_token_val = TokenVal;
    old_scanptr = ScanPtr;
    old_tokenstart = TokenStart;
    ScanPtr = TokenStart = start;
    rtn = ( GetNum( CurrRadix ) && (ScanPtr - TokenStart) == len );
    *val_ptr = TokenVal.int_val;
    ScanPtr = old_scanptr;
    TokenStart = old_tokenstart;
    TokenVal = old_token_val;
    return( rtn );
}
