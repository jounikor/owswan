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
* Description:  Debugger command line processing.
*
****************************************************************************/


#include <ctype.h>
#define BACKWARDS
#include "dbgdefn.h"
#include "dbgdata.h"
#include "dbglit.h"
#include "dbgio.h"
#include "dbgmem.h"
#include "dui.h"
#include "wdmsgdef.h"
#include "dbgscrn.h"
#include "trpld.h"
#include "strutil.h"
#include "dbgcmdln.h"
#include "dbgprog.h"
#include "dbglkup.h"
#include "dbgerr.h"


#define MIN_MEM_SIZE    (500UL*1024)

enum {
    #define pick(t,e)   e,
    #include "_dbgopts.h"
    #undef pick
};

#ifndef GUI_IS_GUI
enum {
    MSG_USAGE_COUNT = 0
    #define pick(num,eng,jap)   + 1
    #include "usage.gh"
    #undef pick
};
#endif

bool                    DownLoadTask = false;

static char             *(*GetArg)( int );
static int              CurrArgc;
static char             *CurrArgp;
static char             CurrChar;

static const char OptNameTab[] = {
    #define pick(t,e)   t "\0"
    #include "_dbgopts.h"
    #undef pick
};


static void SetupChar( void )
{
   CurrChar = *CurrArgp;
   if( CurrChar == NULLCHAR ) {
        CurrChar = ' ';
        CurrArgp = GetArg( ++CurrArgc );
        if( CurrArgp == NULL ) {
            --CurrArgc;
            CurrChar = ARG_TERMINATE; /* absolute end of arguments */
        } else {
            --CurrArgp; /* so that NextChar increment is negated */
        }
    }
}


static void NextChar( void )
{
    ++CurrArgp;
    SetupChar();
}


void OptError( char *err )
{
    char        *curr;
    char        buff[CMD_LEN];
    char        token[CMD_LEN];

    curr = token;
    while( isalnum( CurrChar ) ) {
        *curr++ = CurrChar;
        NextChar();
    }
    if( curr == token ) {
        if( CurrChar == ARG_TERMINATE )
            CurrChar = ' ';
        *curr++ = CurrChar;
    }
    *curr = NULLCHAR;
    Format( buff, err, token );
    StartupErr( buff );
}


void SkipSpaces( void )
{
    while( CurrChar == ' ' || CurrChar == '\t' ) {
        NextChar();
    }
}


bool HasEquals( void )
{
    SkipSpaces();
    return( CurrChar == '=' || CurrChar == '#' );
}


void WantEquals( void )
{
    SkipSpaces();
    if( CurrChar != '=' && CurrChar != '#' )
        OptError( LIT_ENG( STARTUP_Expect_Eql ) );
    NextChar();
    SkipSpaces();
}


/*
 * GetValueLong -- get a long numeric value from command line
 */
unsigned long GetValueLong( void )
{
    unsigned long val;

    WantEquals();
    if( !isdigit( CurrChar ) )
        OptError( LIT_ENG( STARTUP_Invalid_Num ) );
    val = 0;
    do {
        val = val * 10 + CurrChar - '0';
        NextChar();
    } while( isdigit( CurrChar ) );
    return( val );
}

unsigned GetValue( void )
{
    unsigned long val;

    val = GetValueLong();
    if( val > 0xffff )
        OptError( LIT_ENG( STARTUP_Num_Too_Big ) );
    return( val );
}

unsigned long GetMemory( void )
{
    unsigned long   val;

    val = GetValueLong();
    if( tolower( CurrChar ) == 'k' ) {
        val *= 1024;
        NextChar();
    } else if( tolower( CurrChar ) == 'b' ) {
        NextChar();
    } else if( val < 1000 ) {
        val *= 1024;
    }
    return( val );
}

static void DoGetItem( char *buff, bool stop_on_first )
{
    for( ;; ) {
        if( CurrChar == ' ' )
            break;
        if( CurrChar == '\t' )
            break;
        if( CurrChar == ARG_TERMINATE )
            break;
        if( CurrChar == TRAP_PARM_SEPARATOR )
            break;
        if( CurrChar == '{' )
            break;
        if( OptDelim( CurrChar ) && stop_on_first )
            break;
        *buff++ = CurrChar;
        NextChar();
        stop_on_first = true;
    }
    *buff = NULLCHAR;
}

void GetItem( char *buff )
{
    DoGetItem( buff, true );
}


/*
 * GetFileName -- get filename from command line
 */
char *GetFileName( int pass )
{
    char        buff[CMD_LEN];

    WantEquals();
    GetItem( buff );
    return( pass == 1 ? NULL : DupStr( buff ) );
}

void GetRawItem( char *start )
{
    unsigned num;

    SkipSpaces();
    if( CurrChar == '{' ) {
        NextChar();
        num = 1;
        for( ;; ) {
            if( CurrChar == ARG_TERMINATE ) {
                StartupErr( LIT_ENG( STARTUP_Expect_Brace ) );
            } else if( CurrChar == '{' ) {
                ++num;
            } else if( CurrChar == '}' ) {
                if( --num == 0 ) {
                    NextChar();
                    break;
                }
            }
            *start++ = CurrChar;
            NextChar();
        }
        *start = NULLCHAR;
    } else {
        DoGetItem( start, false );
    }
}

static void GetTrapParm( int pass )
{
    char    *start;
    char    parm[TRP_LEN];

    start = parm;
    SkipSpaces();
    *start++ = TRAP_PARM_SEPARATOR;
    GetRawItem( start );
    if( pass == 2 ) {
        _Alloc( start, strlen( parm ) + strlen( TrapParms ) + 1 );
        StrCopy( parm, StrCopy( TrapParms, start ) );
        _Free( TrapParms );
        TrapParms = start;
    }
}

static void GetInitCmd( int pass )
{
    char    cmd[CMD_LEN];

    WantEquals();
    SkipSpaces();
    GetRawItem( cmd );
    if( pass == 2 ) {
        _Free( InitCmdList );
        _Alloc( InitCmdList, strlen( cmd ) + 1 );
        StrCopy( cmd, InitCmdList );
    }
}

#ifndef GUI_IS_GUI
static void PrintUsage( void )
{
    char        *msg_buff;
    dui_res_id  line_id;

    for( line_id = MSG_USAGE_BASE; line_id < MSG_USAGE_BASE + MSG_USAGE_COUNT; line_id++ ) {
        msg_buff = DUILoadString( line_id );
        puts( msg_buff );
        DUIFreeString( msg_buff );
    }
}
#endif

/*
 * ProcOptList -- process an option list
 */

static void ProcOptList( int pass )
{
    char            buff[80];
    char            err_buff[CMD_LEN];
    char            *curr;
    unsigned long   mem;

    SetupChar(); /* initialize scanner */
    for( ;; ) {
        SkipSpaces();
        if( !OptDelim( CurrChar ) )
            break;
        NextChar();
        curr = buff;
#ifndef GUI_IS_GUI
        if( CurrChar == '?' ) {
            PrintUsage();
            StartupErr( "" );
        }
#endif
        while( isalnum( CurrChar ) ) {
            *curr++ = CurrChar;
            NextChar();
        }
        if( curr == buff ) {
            if( OptDelim( CurrChar ) ) {
                NextChar();
                SkipSpaces();
                break;
            }
            OptError( LIT_ENG( STARTUP_No_Recog_Optn ) );
        }
        switch( Lookup( OptNameTab, buff, curr - buff ) ) {
        case OPT_CONTINUE_UNEXPECTED_BREAK:
            _SwitchOn( SW_CONTINUE_UNEXPECTED_BREAK );
            break;
        case OPT_DEFERSYM:
            _SwitchOn( SW_DEFER_SYM_LOAD );
            break;
        case OPT_DOWNLOAD:
            DownLoadTask = true;
            break;
        case OPT_NOEXPORTS:
            _SwitchOn( SW_NO_EXPORT_SYMS );
            break;
        case OPT_LOCALINFO:
            if( pass == 2 ) {
                char *symfile = GetFileName( pass );
                FindLocalDebugInfo( symfile );
                _Free( symfile );
            }
            break;
        case OPT_INVOKE:
            if( pass == 2 )
                _Free( InvokeFile );
            InvokeFile = GetFileName( pass );
            break;
        case OPT_NOINVOKE:
            if( pass == 2 )
                _Free( InvokeFile );
            InvokeFile = NULL;
            break;
        case OPT_NOSOURCECHECK:
            _SwitchOff( SW_CHECK_SOURCE_EXISTS );
            break;
        case OPT_NOSYMBOLS:
            _SwitchOff( SW_LOAD_SYMS );
            break;
        case OPT_NOMOUSE:
            _SwitchOff( SW_USE_MOUSE );
            break;
        case OPT_DYNAMIC:
            mem = GetMemory();
            if( pass == 1 ) {
                if( mem < MIN_MEM_SIZE )
                    mem = MIN_MEM_SIZE;
                MemSize = mem;
            }
            break;
        case OPT_DIP:
            {
                int i;

                for( i = 0; DipFiles[i] != NULL; ++i )
                    ;
                DipFiles[i] = GetFileName( pass );
            }
            break;
        case OPT_TRAP:
            if( pass == 2 )
                _Free( TrapParms );
            TrapParms = GetFileName( pass );
            SkipSpaces();
            if( CurrChar == TRAP_PARM_SEPARATOR ) {
                NextChar();
                GetTrapParm( pass );
            } else if( CurrChar == '{' ) {
                GetTrapParm( pass );
            }
            break;
#ifdef ENABLE_TRAP_LOGGING
        case OPT_TRAP_DEBUG_FLUSH:
            if( pass == 2 )
                _Free( TrapTraceFileName );
            TrapTraceFileName = GetFileName( pass );
            TrapTraceFileFlush = true;
            break;
        case OPT_TRAP_DEBUG:
            if( pass == 2 )
                _Free( TrapTraceFileName );
            TrapTraceFileName = GetFileName( pass );
            TrapTraceFileFlush = false;
            break;
#endif
        case OPT_REMOTE_FILES:
            _SwitchOn( SW_REMOTE_FILES );
            break;
        case OPT_LINES:
            DUISetNumLines( GetValue() );
            break;
        case OPT_COLUMNS:
            DUISetNumColumns( GetValue() );
            break;
#ifdef BACKWARDS
        case OPT_NO_FPU:
        case OPT_NO_ALTSYM:
            break;
        case OPT_REGISTERS:
            GetValue();
            break;
#endif
        case OPT_INITCMD:
            GetInitCmd( pass );
            break;
        case OPT_POWERBUILDER:
            _SwitchOn( SW_POWERBUILDER );
            break;
        case OPT_HELP:
#ifndef GUI_IS_GUI
            PrintUsage();
            StartupErr( "" );
#endif
            break;
        default:
            if( !ProcSysOption( buff, curr - buff, pass ) && !DUIScreenOption( buff, curr - buff, pass ) ) {
                Format( err_buff, LIT_ENG( STARTUP_Invalid_Option ), buff, curr - buff );
                StartupErr( err_buff );
            }
            break;
        }
    }
}

static char *GetEnvArg( int i )
{
    /* unused parameters */ (void)i;

    return( NULL );
}



/*
 * ProcCmd -- start processing command line options
 */

void ProcCmd( void )
{
    char        buff[TXT_LEN];
    unsigned    screen_mem;
    size_t      have_env;
    int         pass;

    MemSize = MIN_MEM_SIZE;
    TrapParms = NULL;
    _SwitchOn( SW_LOAD_SYMS );
    _SwitchOn( SW_USE_MOUSE );
    ProcSysOptInit();
    DUIScreenOptInit();

    have_env = DUIEnvLkup( EXENAME, buff, sizeof( buff ) );
    for( pass = 1; pass <= 2; ++pass ) {
        if( have_env ) {
            GetArg = &GetEnvArg;
            CurrArgc = 0;
            CurrArgp = buff;
            ProcOptList( pass );
            if( CurrChar != ARG_TERMINATE ) {
                OptError( LIT_ENG( STARTUP_Expect_End_Env_Str ) );
            }
        }
        GetArg = &GetCmdArg;
        CurrArgc = 0;
        CurrArgp = GetCmdArg( 0 );
        if( CurrArgp != NULL ) {
            ProcOptList( pass );
            if( pass == 2 ) {
                SetCmdArgStart( CurrArgc, CurrArgp );
            }
        }
        if( pass == 1 ) {
            screen_mem = DUIConfigScreen();
            if( MemSize + screen_mem >= MemSize ) {
                MemSize += screen_mem;
            } else {
                MemSize = ~0;
            }
            SysSetMemLimit();
            TrapParms = DupStr( "std" );
            InvokeFile = DupStr( "" );
        }
    }
}
