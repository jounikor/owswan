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
* Description:  Process the 'call' command.
*
****************************************************************************/


#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbglit.h"
#include "dbgerr.h"
#include "dbgmem.h"
#include "dbgstk.h"
#include "wspawn.h"
#include "madcli.h"
#include "strutil.h"
#include "dbgmad.h"
#include "dbgscan.h"
#include "dbgexpr.h"
#include "dbgloc.h"
#include "dbgcall.h"
#include "dbgcall2.h"
#include "dbgshow.h"
#include "dbgprint.h"
#include "dbgparse.h"
#include "dbgreg.h"
#include "dbgsetfg.h"


#define MAX_PARMS       10

static char             *DefParms[MAX_PARMS];
static mad_string       DefCallType;
static char             *DefReturn;


/*
 * InitCall - initialize default call attributes
 */

static void FreeParms( void )
{
    int         i;

    for( i = 0; i < MAX_PARMS; ++i ) {
        _Free( DefParms[i] );
        DefParms[i] = NULL;
    }
}

void FiniCall( void )
{
    _Free( DefReturn );
    DefReturn = NULL;
    FreeParms();
}

static void GetLocation( location_list *ll, dig_type_info *ti )
{
    bool        reg_set;

    reg_set = false;
    if( CurrToken == T_LEFT_BRACKET ) {
        reg_set = true;
        Scan();
    }
    ParseRegSet( reg_set, ll, ti );
    if( ti->size == 0 ) {
        Error( ERR_LOC, LIT_ENG( ERR_WANT_REG_NAME ) );
    }
    if( reg_set ) {
        Recog( T_RIGHT_BRACKET );
    }
}

/*
 * CallSet - set default call attributes
 */

static void DoCallSet( void )
{
    mad_string          ctype;
    int                 parm;
    int                 i;
    const char          *start;
    struct {
        union {
            const char  *arg;
            char        *start;
        } u;
        unsigned        len;
    }                   new_parms[MAX_PARMS];
    location_list       ll;
    dig_type_info       ti;

    if( CurrToken == T_DIV ) {
        Scan();
        ctype = ScanCall();
        if( ctype == MAD_MSTR_NIL ) {
            Error( ERR_LOC, LIT_ENG( ERR_BAD_CALL_TYPE ) );
        }
    } else {
        ctype = DefCallType;
    }
    if( CurrToken == T_LEFT_PAREN ) {
        parm = 0;
        Scan();
        if( CurrToken != T_RIGHT_PAREN ) {
            for( ;; ) {
                if( parm >= MAX_PARMS )
                    Error( ERR_LOC, LIT_ENG( ERR_TOO_MANY_PARMS ) );
                new_parms[parm].u.arg = ScanPos();
                GetLocation( &ll, &ti );
                new_parms[parm].len = ScanPos() - new_parms[parm].u.arg;
                ++parm;
                if( CurrToken != T_COMMA )
                    break;
                Scan();
            }
        }
        Recog( T_RIGHT_PAREN );
    } else {
        parm = -1;
    }
    start = ScanPos();
    if( CurrToken == T_DIV ) {
        Scan();
    } else if( ScanEOC() ) {
        start = NULL;
    } else {
        /* syntax check */
        ChkPrintList();
    }
    ReqEOC();
    if( start != NULL ) {
        char *new_return;

        i = ScanPos() - start;
        new_return = DbgMustAlloc( i + 1 );
        memcpy( new_return, start, i );
        new_return[i] = NULLCHAR;
        _Free( DefReturn );
        DefReturn = new_return;
    }
    if( parm >= 0 ) {
        for( i = 0; i < parm; ++i ) {
            char    *new_arg;

            _Alloc( new_arg, new_parms[i].len + 1 );
            if( new_arg == NULL ) {
                parm = i;
                for( i = 0; i < parm; ++i ) {
                    _Free( new_parms[i].u.start );
                }
                parm = 0;
                Error( ERR_NONE, LIT_ENG( ERR_NO_MEMORY_FOR_EXPR ) );
                break;
            } else {
                memcpy( new_arg, new_parms[i].u.arg, new_parms[i].len );
                new_arg[new_parms[i].len] = NULLCHAR;
                new_parms[i].u.start = new_arg;
            }
        }
        FreeParms();
        for( i = 0; i < parm; ++i ) {
            DefParms[i] = new_parms[i].u.start;
        }
    }
    DefCallType = ctype;
}

void CallSet( void )
{
    DoCallSet();
    _SwitchOn( SW_HAVE_SET_CALL );
}

void CallConf( void )
{
    char        *ptr;
    unsigned    i;
    bool        first;

    if( _IsOn( SW_HAVE_SET_CALL ) ) {
        ptr = TxtBuff;
        if( DefCallType != MAD_MSTR_NIL ) {
            *ptr++ = '/';
            ptr += MADCli( String )( DefCallType, ptr, TXT_LEN );
        }
        *ptr++ = '(';
        first = true;
        for( i = 0; i < MAX_PARMS; ++i ) {
            if( DefParms[i] != NULL ) {
                if( !first ) {
                    *ptr++ = ',';
                }
                ptr = StrCopy( DefParms[i], ptr );
                first = false;
            }
        }
        *ptr++ = ')';
        if( DefReturn == NULL ) {
            *ptr++ = '/';
            *ptr = NULLCHAR;
        } else {
            StrCopy( DefReturn, ptr );
        }
        ConfigLine( TxtBuff );
    }
}


/*
 * CallResults -- print out result of user call
 */

static void CallResults( void )
{
    if( CurrToken != T_DIV ) {
        DoPrintList( false );
    }
}


/*
 * ProcCall -- process call immediate command
 */

void ProcCall( void )
{
    mad_string          ctype;
    int                 parm;
    const char          *results;
    address             start;
    const char          *old;
    location_list       ll;
    dig_type_info       ti;
    dig_type_info       *pti;
    char                *p;
    const mad_reg_info  **parm_reg;

    if( CurrToken == T_DIV ) {
        Scan();
        ctype = ScanCall();
        if( ctype == MAD_MSTR_NIL ) {
            Error( ERR_LOC, LIT_ENG( ERR_BAD_CALL_TYPE ) );
        }
    } else {
        ctype = DefCallType;
    }
    CallExpr( &start );
    if( _IsOff( SW_HAVE_SET_CALL ) ) {
        FiniCall();
        p = TxtBuff;
        *p++ = '(';
        parm_reg = MADCallParmRegList( ctype, start );
        for( parm = 0; parm_reg[parm] != NULL; ++parm ) {
            if( parm > 0 ) {
                *p++ = ',';
            }
            p = StrCopy( parm_reg[parm]->name, p );
        }
        *p++ = ')';
        StrCopy( MADCallReturnReg( ctype, start )->name, p );
        old = ReScan( TxtBuff );
        DoCallSet();
        ReScan( old );
    }
    parm = 0;
    results = DefReturn;
    if( CurrToken == T_LEFT_PAREN ) {
        Scan();
        if( CurrToken != T_RIGHT_PAREN ) {
            for( ; ; ) {
                pti = &ti;
                if( CurrToken == T_DIV ) {
                    Scan();
                    if( CurrToken == T_DIV ) {
                        Scan();
                        /* on stack */
                        LocationCreate( &ll, LT_INTERNAL, NULL );
                        pti = NULL;
                    } else {
                        GetLocation( &ll, pti );
                    }
                } else {
                    if( DefParms[parm] == NULL ) {
                        /* on stack */
                        LocationCreate( &ll, LT_INTERNAL, NULL );
                        pti = NULL;
                    } else {
                        old = ReScan( DefParms[parm] );
                        GetLocation( &ll, pti );
                        ReScan( old );
                    }
                }
                PushLocation( &ll, pti );
                NormalExpr();
                SwapStack( 1 );
                ++parm;
                if( CurrToken != T_COMMA )
                    break;
                Scan();
            }
        }
        Recog( T_RIGHT_PAREN );
        results = ScanPos();
        if( CurrToken == T_DIV ) {
            Scan();
        } else if( ScanEOC() ) {
            results = DefReturn;
        } else {
            /* syntax check */
            ChkPrintList();
        }
    }
    ReqEOC();
    FreezeRegs();
    if( PerformExplicitCall( start, ctype, parm ) && results != NULL ) {
        old = ReScan( results );
        if( Spawn( CallResults ) == 0 ) {
            ReScan( old );
        }
    }
    UnFreezeRegs();
    FreePgmStack( false );
}
