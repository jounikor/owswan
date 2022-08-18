/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Echo CG API calls and track handle usage (for debugging).
*
****************************************************************************/


#include "_cgstd.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "coderep.h"
#include "types.h"
#include "tree.h"
#include "seldef.h"
#include "echoapi.h"
#include "model.h"
#include "cgmem.h"
#include "useinfo.h"
#include "onexit.h"
#include "dumpio.h"
#include "addrname.h"
#include "inline.h"
#include "envvar.h"
#include "feprotos.h"

#include "clibext.h"


#define hdlSetUsed( handle, value ) ( ((use_info *)handle)->used = value )
#define hdlSetup( hdltype, handle ) ( handleSetup( hdltype, (use_info *)handle) )
#define hdlAddReuse( hdltype, handle ) ( handleAddReuse( hdltype, (use_info *)handle) )


// ERROR MESSAGE FUNCTIONS

static  FILE    *EchoAPIFile = NULL;
static  int     EchoAPIFlush = 0;

static void     EchoAPIRedirect( void )
/*************************************/
{
    char    tmpfile[PATH_MAX];

    if( GetEnvVar("echoapiflush", tmpfile, 11 ) ) {
        EchoAPIFlush = 1;
    }
    if( GetEnvVar("echoapifile", tmpfile, 11 ) ) {
        EchoAPIFile = fopen( tmpfile, "wt" );
    }
}

static  void    EchoAPIUnredirect( void )
/***************************************/
{
    if( EchoAPIFile == NULL )
        return;
    fclose( EchoAPIFile );
    EchoAPIFile = NULL;
}

static  void    EchoAPIChar( char c )
/***********************************/
{
    /* if the codegen is crashing we want flushing to occur
     */
    if( EchoAPIFile != NULL ) {
        fputc( c, EchoAPIFile );
        if( EchoAPIFlush ) {
            fflush( EchoAPIFile );
        }
    } else {
        fputc( c, stdout );
        if( EchoAPIFlush ) {
            fflush( stdout );
        }
    }
}

static  void    EchoAPIString( char const *s )
/********************************************/
{
    while( *s ) {
        EchoAPIChar( *s );
        s++;
    }
}

static void errMsg              // PRINT ERROR MESSAGE
    ( char const *msg )         // - message
{
    DumpLiteral( "\n**** CgDbg Failure: " );
    DumpString( msg );
    DumpChar( '\n' );
}

static void handleFailure( void )
{
    DumpLiteral( "Aborted after CgDbg Failure" );
    EchoAPIUnredirect();
    FatalError( "CGDbg Failure\n" );
}

static void printName( handle_type hdltype )
{
    switch( hdltype ) {
    case NO_HANDLE :
        DumpLiteral("uninitialized");
        break;
    case CG_NAMES :
        DumpLiteral("cg_name");
        break;
    case LABEL_HANDLE :
        DumpLiteral("label_handle");
        break;
    case PATCH_HANDLE :
        DumpLiteral("patch_handle");
        break;
    case SEL_HANDLE :
        DumpLiteral("sel_handle");
        break;
    default :
        DumpLiteral("default");
        break;
    }
}

static void handleMsg             // PRINT ERROR FOR RING
    ( handle_type hdltype,
      char const* msg )         // - message
{
    errMsg( msg );
    DumpLiteral( "\n**** CgDbg Failure: for " );
    printName( hdltype );
    DumpChar( '\n' );
    handleFailure();
}

static void ErrExpectFound( handle_type expect, handle_type found )
{
    DumpLiteral( "\n**** CgDbg Failure: " );
    DumpLiteral( "\n\texpecting type ");
    printName(expect);
    DumpChar( '\n' );

    DumpLiteral( "\n\tfound type ");
    printName(found);
    DumpChar( '\n' );
    handleFailure();
}

// TYPE CHECKING FUNCTIONS

void verifyNotUserType   // VERIFY NOT A USER-DEFINED TYPE
    ( cg_type type )            // - that type
{
    type_def    *alias;

    // Check for aliased types
    alias = TypeAddress( type );
    if( alias ) {
        type = alias->refno;
    }
    if( type >= TY_FIRST_FREE ) {
        errMsg( "cannot use user-defined type" );
        handleFailure();
    }
}


// PRINTING SUPPORT

#define case_str( a ) case a : EchoAPIString( #a ); break;
#define case_prt( a, b ) case a : EchoAPIString( #b ); break;

static void EchoAPIHandle        // EchoAPI A HANDLE
    ( int handle                // - handle
    , char const *lab )         // - label for handle
{
    char buffer[16];
    EchoAPIString( lab );
    sprintf( buffer, "[%x]", handle );
    EchoAPIString( buffer );
}

static void dumpLineHeader( void ) // let us know how deep inline we are
{

    int depth;

    depth = BGInlineDepth();
    while( depth-- != 0 ) {
        EchoAPIChar( '\t' );
    }
}

// debug output
void EchoAPI              // EchoAPI ROUTINE
    ( const char *text          // - text
    , ... )                     // - operands
{
    if( _IsModel( ECHO_API_CALLS ) ) {
        char buffer[256];
        va_list args;
        va_start( args, text );
        dumpLineHeader();
        for( ; *text != 0; ++text ) {
            if( *text == '%' ) {
                ++text;
                switch( *text ) {
                  case 'B' : {
                    EchoAPIHandle( (int)(pointer_uint)va_arg( args, back_handle ), "bh" );
                    break;
                  }
                  case 'c' : {
                    char const *text1 = va_arg( args, char const * );
                    EchoAPIString( text1 );
                    break;
                  }
                  case 'C' : {
                    EchoAPIHandle( (int)(pointer_uint)va_arg( args, call_handle ), "ch" );
                    break;
                  }
                  case 'i' : {
                    int val = va_arg( args, int );
                    sprintf( buffer, "%d", val );
                    EchoAPIString( buffer );
                    break;
                  }
                  case 'L' : {
                    EchoAPIHandle( (int)(pointer_uint)va_arg( args, label_handle ), "lh" );
                    break;
                  }
                  case 'o' : {
                    cg_op op = va_arg( args, cg_op );
                    switch( op ) {
#define STR(x) #x
#define STR1(x,y) x
#define STR2(x,y) STR(y)
#define PICK(e,i,d1,d2,ot,pnum,attr)  case O_##e: EchoAPIString( d2(d1,O_##e) ); break;
#include "cgops.h"
#undef PICK
#undef STR2
#undef STR1
#undef STR
                      default :
                        sprintf( buffer, "O_0%d", op );
                        EchoAPIString( buffer );
                        break;
                    }
                    break;
                  }
                  case 'n' : {
                    EchoAPIHandle( (int)(pointer_uint)va_arg( args, cg_name ), "cg" );
                    break;
                  }
                  case 'P' : {
                    EchoAPIHandle( (int)(pointer_uint)va_arg( args, patch_handle ), "ph" );
                    break;
                  }
                  case 's' : {
                    cg_sym_handle handle = va_arg( args, cg_sym_handle );
                    EchoAPIString( FEName( handle ) );
                    sprintf( buffer, "[%x]", (int)(pointer_uint)handle );
                    EchoAPIString( buffer );
                    break;
                  }
                  case 'S' : {
                    EchoAPIHandle( (int)(pointer_uint)va_arg( args, sel_handle ), "sh" );
                    break;
                  }
                  case 'T' : {
                    EchoAPIHandle( (int)(pointer_uint)va_arg( args, temp_handle ), "th" );
                    break;
                  }
                  case 't' : {
                    cg_type type = (cg_type)va_arg( args, int );
                    switch( type ) {
                      case_str( TY_UINT_1 )
                      case_str( TY_INT_1 )
                      case_str( TY_UINT_2 )
                      case_str( TY_INT_2 )
                      case_str( TY_UINT_4 )
                      case_str( TY_INT_4 )
                      case_str( TY_UINT_8 )
                      case_str( TY_INT_8 )
                      case_str( TY_LONG_POINTER )
                      case_str( TY_HUGE_POINTER )
                      case_str( TY_NEAR_POINTER )
                      case_str( TY_LONG_CODE_PTR )
                      case_str( TY_NEAR_CODE_PTR )
                      case_str( TY_SINGLE )
                      case_str( TY_DOUBLE )
                      case_str( TY_LONG_DOUBLE )
                      case_str( TY_UNKNOWN )
                      case_str( TY_DEFAULT )
                      case_str( TY_INTEGER )
                      case_str( TY_UNSIGNED )
                      case_str( TY_POINTER )
                      case_str( TY_CODE_PTR )
                      case_str( TY_BOOLEAN )
                      case_str( TY_PROC_PARM )
                      default :
                        sprintf( buffer, "TY_0%d", type );
                        EchoAPIString( buffer );
                        break;
                    }
                    break;
                  }
                  case 'x' : {
                    int val = va_arg( args, int );
                    sprintf( buffer, "0x%x", val );
                    EchoAPIString( buffer );
                    break;
                  }
                  default : {
                    EchoAPIString( "*** BAD %" );
                    EchoAPIChar( *text );
                    EchoAPIString( " ***" );
                    break;
                  }
                }
            } else {
                EchoAPIChar( *text );
            }
        }
        va_end( args );
    }
}

cg_name EchoAPICgnameReturn     // EchoAPI cg_name RETURN VALUE
    ( cg_name retn )            // - cg_name value
{
    EchoAPI( " -> %n\n", retn );
    return( retn );
}

call_handle EchoAPICallHandleReturn // EchoAPI call_handle RETURN VALUE
    ( call_handle call )            // - call_handle value
{
    EchoAPI( " -> %C\n", call );
    return( call );
}

cg_type EchoAPICgtypeReturn     // EchoAPI cg_type RETURN VALUE
    ( cg_type retn )            // - cg_type value
{
    EchoAPI( " -> %t\n", retn );
    return( retn );
}


int EchoAPIHexReturn            // EchoAPI HEXADECIMAL RETURN VALUE
    ( int retn )                // - value
{
    EchoAPI( " -> %x\n", retn );
    return( retn );
}


int EchoAPIIntReturn    // EchoAPI DECIMAL RETURN VALUE
    ( int retn )        // - value
{
    EchoAPI( " -> %i\n", retn );
    return( retn );
}


sel_handle EchoAPISelHandleReturn   // EchoAPI sel_handle RETURN VALUE
    ( sel_handle retn )             // - sel_handle value
{
    EchoAPI( " -> %S\n", retn );
    return( retn );
}


temp_handle EchoAPITempHandleReturn // EchoAPI temp_handle RETURN VALUE
    ( temp_handle retn )            // - temp_handle value
{
    EchoAPI( " -> %T\n", retn );
    return( retn );
}


void handleExists               // VERIFY EXISTING HANDLE
    ( handle_type hdltype
    , use_info *useinfo )       // - the handle
{
    if( useinfo == NULL ) {
        handleMsg( hdltype, "handle does not exist" );
    }
    if( useinfo->hdltype != hdltype ) {
        ErrExpectFound( hdltype, useinfo->hdltype );
    }
}

// USE HANDLE EXACTLY ONCE

void handleUseOnce( handle_type hdltype, use_info *useinfo )
{
    handleExists( hdltype, useinfo );
    if( useinfo->used ) {
        handleMsg( hdltype, "handle already used" );
    } else {
        useinfo->used = true;
    }
}

static void handleSetup( handle_type hdltype, use_info *useinfo )
{
    useinfo->hdltype = hdltype;
    useinfo->used = false;
}

static void handleAddReuse( handle_type hdltype, use_info *useinfo )
{
    if( useinfo->hdltype == hdltype ) {
        if( useinfo->used ) {
            handleSetup( hdltype, useinfo );
        } else {
            handleMsg( hdltype, "Handle already exists" );
        }
    } else {
        handleSetup( hdltype, useinfo );
    }
}

void handleAdd( handle_type hdltype, use_info *useinfo )
{
    if( useinfo->hdltype != NO_HANDLE || useinfo->used ) {
        handleMsg( hdltype, "Handle already exists" );
    } else {
        handleSetup( hdltype, useinfo );
    }
}

void hdlAddUnary                // ADD A HANDLE AFTER A UNARY OPERATION
    ( handle_type hdltype       // - the handle type
    , tn handle                 // - the handle
    , tn old )                  // - handle operated upon by unary oper'n
{
    /* unused parameters */ (void)old;

    hdlAddReuse( hdltype, handle );
}

void hdlAddBinary               // ADD A HANDLE AFTER A BINARY OPERATION
    ( handle_type hdltype
    , tn handle                 // - the handle
    , tn old_l                  // - handle operated upon by unary oper'n
    , tn old_r )                // - handle operated upon by unary oper'n
{
    /* unused parameters */ (void)old_l; (void)old_r;

    hdlAddReuse( hdltype, handle );
}

void hdlAddTernary              // ADD A HANDLE AFTER A TERNARY OPERATION
    ( handle_type hdltype
    , tn handle                 // - the handle
    , tn old_t                  // - handle operated upon by unary oper'n
    , tn old_l                  // - handle operated upon by unary oper'n
    , tn old_r )                // - handle operated upon by unary oper'n
{
    /* unused parameters */ (void)old_t; (void)old_l; (void)old_r;

    hdlAddReuse( hdltype, handle );
}

void hdlAllUsed                 // VERIFY ALL HANDLES IN RING HAVE BEEN USED
    ( handle_type hdltype )
{
    /* unused parameters */ (void)hdltype;

    // check to ensure that all nodes of type hdltype are used LMW
    // create linked list of node when creating, and check all are empty
    // after tree is burned
}

void EchoAPIInit( void )
{
    EchoAPIRedirect();
}

void EchoAPIFini( void )
{
    EchoAPIUnredirect();
}

static char *callBackName       // MAKE CALL-BACK NAME FOR PRINTING
    ( cg_callback rtn           // - rtn address
    , char *buffer )            // - buffer
{
    const char  *name;          // - name to be used

    name = FEExtName( (cg_sym_handle)&rtn, EXTN_CALLBACKNAME );
    if( name == NULL ) {
        sprintf( buffer, "%p", rtn );
    } else {
        sprintf( buffer, "%p:%s", rtn, name );
    }
    return( buffer );
}


void EchoAPICallBack(           // CALL-BACK SUPPORT
    tn node
  , cg_callback rtn
  , callback_handle param
  , char *start_end )           // - call-back entry
{
    char buffer[64];

    EchoAPI( "\n*** CALL BACK[%x] to %c(%x) "
          , node
          , callBackName( rtn, buffer )
          , param );
    EchoAPI(start_end);
}
