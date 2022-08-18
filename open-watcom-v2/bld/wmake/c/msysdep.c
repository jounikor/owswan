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
* Description:  Platform dependent internal helper functions for wmake.
*
****************************************************************************/


#include "make.h"
#include <ctype.h>
#include "wio.h"
#include "mcache.h"
#include "mmemory.h"
#include "mrcmsg.h"
#include "msg.h"
#include "mupdate.h"
#if defined(__UNIX__) || defined( __WATCOMC__ )
    #include <utime.h>
#else
    #include <sys/utime.h>
#endif
#if defined( __DOS__ )
    #include <dos.h>
    #include "tinyio.h"
#else
  #if defined( __OS2__ )
    #define INCL_DOSMODULEMGR
    #define INCL_DOSERRORS
    #define INCL_DOSMISC
    #define INCL_ORDINALS
    #include <os2.h>
  #endif
#endif
#include "pcobj.h"


#if defined( __DOS__ )

extern char DOSSwitchChar( void );
#pragma aux DOSSwitchChar = \
        "mov ax,3700h"  \
        "int 21h"       \
    __parm __caller [] \
    __value         [__dl] \
    __modify        [__ax __dx]

#if defined ( _M_I86 )

/* see page 90-91 of "Undocumented DOS" */
extern void __far *_DOS_list_of_lists( void );
#pragma aux _DOS_list_of_lists = \
        "mov ax,5200h"  \
        "int 21h"       \
    __parm __caller [] \
    __value         [__es __bx] \
    __modify        [__ax __es __bx]
#endif

#endif

static ENV_TRACKER  *envList;

#if defined( __DOS__ )

static int __far critical_error_handler( unsigned deverr, unsigned errcode, unsigned __far *devhdr )
{
    deverr = deverr; errcode = errcode; devhdr = devhdr;
    return( _HARDERR_FAIL );
}

void InitHardErr( void )
{
    _harderr( critical_error_handler );
}

#else

void InitHardErr( void )
{
}

#endif

int SwitchChar( void )
/***************************/
{
#if defined( __DOS__ )
    return( DOSSwitchChar() );
#elif   defined( __OS2__ ) || defined( __NT__ ) || defined( __RDOS__ )
    return( '/' );
#elif   defined( __UNIX__ )
    return( '-' );
#endif
}

int OSCorrupted( void )
/*********************/
{
#if defined( __DOS__ ) && defined ( _M_I86 )
    _Packed struct mcb {
        UINT8   id;
        UINT16  owner;
        UINT16  len;
    } __far *chain;
    UINT16 __far  *first_MCB;
    UINT16      chain_seg;
    UINT16      new_chain_seg;

    first_MCB = _DOS_list_of_lists();
    if( _FP_OFF( first_MCB ) == 1 ) {    /* next instr will hang! */
        /* list of lists DOS call may have been interrupted */
        return( 1 );
    }
    chain_seg = first_MCB[-1];
    for( ;; ) {
        chain = _MK_FP( chain_seg, 0 );
        if( chain->id == 'Z' ) {
            break;
        }
        if( chain->id != 'M' ) {
            return( 1 );
        }
        new_chain_seg = chain_seg + ( chain->len + 1 );
        if( new_chain_seg <= chain_seg ) {
            return( 1 );
        }
        chain_seg = new_chain_seg;
    }
#endif
    return( 0 );
}

bool TouchFile( const char *name )
/********************************/
{
#if defined( __DOS__ )
    tiny_date_t     dt;
    tiny_time_t     tm;
    tiny_ftime_t    p_hms;
    tiny_fdate_t    p_ymd;
    tiny_ret_t      ret;

    ret = TinyOpen( name, TIO_WRITE );
    if( TINY_OK( ret ) ) {
        dt = TinyGetDate();
        p_ymd.year  = dt.year + (1900 - 1980);
        p_ymd.month = dt.month;
        p_ymd.day   = dt.day_of_month;

        tm = TinyGetTime();
        p_hms.hours   = tm.hour;
        p_hms.minutes = tm.minutes;
        p_hms.twosecs = tm.seconds / 2;

        TinySetFileStamp( TINY_INFO( ret ), p_hms, p_ymd );
    } else {
        ret = TinyCreate( name, TIO_NORMAL );
        if( TINY_ERROR( ret ) ) {
            return( false );
        }
    }
    TinyClose( TINY_INFO( ret ) );
#else
    int     fh;

    if( utime( name, NULL ) == -1 ) {
        fh = creat( name, PMODE_RW );
        if( fh == -1 ) {
            return( false );
        }
        close( fh );
    }
#endif
    return( true );
}

#define FUZZY_DELTA     60      /* max allowed variance from stored time-stamp */

bool IdenticalAutoDepTimes( time_t in_obj, time_t stamp, auto_dep_type autodep_type )
/***********************************************************************************/
{
    time_t  diff_time;

    if( in_obj == stamp )
        return( true );
#if defined( __WATCOMC__ ) && __WATCOMC__ < 1300
    /*
     * OW1.x bootstrap compiler workaround
     */
    if( in_obj < stamp ) {
        if( (( in_obj + 1 ) & ~1) == (stamp & ~1) ) {
            return( true );
        }
    } else {
        if( (( stamp + 1 ) & ~1) == (in_obj & ~1) ) {
            return( true );
        }
    }
#else
  #if defined( __DOS__ ) || defined( __OS2__ )
    /*
     * host is two-second file time based, the stamp is always two-second based time
     * if in_obj is not a two-second based file time ( ELF, COFF or RES object files )
     * then we need to round in_obj to the nearest two-second based time
     */
    if( autodep_type != AUTO_DEP_OMF && (in_obj & 1) ) {
        if( stamp == (in_obj & ~1) ) {
            return( true );
        }
    }
  #else
    /*
     * host is one-second file time based, the stamp is always one-second based time
     * if in_obj is a two-second based file time ( OMF object files )
     * then we need to round stamp to the nearest two-second based time
     */
    if( autodep_type == AUTO_DEP_OMF && (stamp & 1) ) {
        if( in_obj == (stamp & ~1) ) {
            return( true );
        }
    }
  #endif
#endif
    if( in_obj < stamp ) {
        /* stamp is newer than time in .OBJ file */
        if( Glob.fuzzy ) {
            /* check for a "tiny" difference in times (almost identical) */
            diff_time = stamp - in_obj;
            if( diff_time <= FUZZY_DELTA ) {
                return( true );
            }
        }
    }
    return( false );
}

#ifdef DLLS_IMPLEMENTED

#include "idedrv.h"

static DLL_CMD  *dllCommandList;

void OSLoadDLL( char *cmd_name, char *dll_name, char *ent_name )
/**************************************************************/
{
    DLL_CMD     *n;

    // we want newer !loaddlls to take precedence
    n = MallocSafe( sizeof( *n ) );
    n->cmd_name = StrDupSafe( cmd_name );
    n->next = dllCommandList;
    dllCommandList = n;
    IdeDrvInit( &n->inf, StrDupSafe( dll_name ),
                (ent_name == NULL) ? NULL : StrDupSafe( ent_name ) );
}

DLL_CMD *OSFindDLL( char const *cmd_name )
/****************************************/
{
    DLL_CMD     *n;

    for( n = dllCommandList; n != NULL; n = n->next ) {
        if( 0 == stricmp( cmd_name, n->cmd_name ) ) {
            break;
        }
    }
    return( n );
}

#define DLL_PREFIX  "DLL:"
#define DLL_PSIZE   sizeof( DLL_PREFIX ) - 1

int OSExecDLL( DLL_CMD* dll, char const* cmd_args )
/*************************************************
 * Returns the error code returned by IdeDrvDLL
 */
{
    int     retcode = IdeDrvExecDLL( &dll->inf, cmd_args );

    setmode( STDOUT_FILENO, O_TEXT );
    return( retcode );
}

#else

DLL_CMD *OSFindDLL( char const *cmd_name )
/****************************************/
{
    /* unused parameters */ (void)cmd_name;

    return( NULL );
}

void OSLoadDLL( char *cmd_name, char *dll_name, char *ent_name )
/**************************************************************/
{
    /* unused parameters */ (void)cmd_name; (void)dll_name; (void)ent_name;
}

int OSExecDLL( DLL_CMD* dll, char const* cmd_args )
/*************************************************/
{
    /* unused parameters */ (void)dll; (void)cmd_args;

    return( -1 );
}

#endif

#ifndef NDEBUG
STATIC void cleanDLLCmd( void )
{
#ifdef DLLS_IMPLEMENTED
    DLL_CMD     *n;
    DLL_CMD     *temp;

    n  = dllCommandList;
    while( n != NULL ) {
        FreeSafe( (char *)n->cmd_name );
        if( n->inf.dll_name != NULL ) {
            FreeSafe( (char*) n->inf.dll_name );
        }
        if( n->inf.ent_name != NULL ) {
            FreeSafe( (char *)n->inf.ent_name );
        }
        temp = n;
        n = n->next;
        FreeSafe( temp );
    }
#endif
}
#endif


#ifndef NDEBUG
void DLLFini( void )
{
    cleanDLLCmd();
}
#endif

static sig_atomic_t     sig_count;

void CheckForBreak( void )
{
    if( sig_count > 0 ) {
        sig_count = 0;
        PrtMsg( ERR | USER_BREAK_ENCOUNTERED );
        ExitError();
        // never return
    }
}


static void passOnBreak( void )
{
#ifdef DLLS_IMPLEMENTED
    DLL_CMD     *n;

    for( n = dllCommandList; n != NULL; n = n->next ) {
        IdeDrvStopRunning( &n->inf );
    }
#endif
}


static void breakHandler( int sig_number )
{
    /* unused parameters */ (void)sig_number;

    sig_count = 1;
    passOnBreak();
}

void InitSignals( void )
{
    sig_count = 0;
    DoingUpdate = false;
#ifndef __UNIX__
    signal( SIGBREAK, breakHandler );
#endif
    signal( SIGINT, breakHandler );
}

#if defined( __OS2__ )

/* Older versions of OS/2 did not support BEGIN/ENDLIBPATH.
 * Dynamically query the API entrypoints to prevent load failures.
 */

#define BEGPATHNAME "BEGINLIBPATH"
#define ENDPATHNAME "ENDLIBPATH"

static char os2BegLibPath[1024] = "";
static char os2EndLibPath[1024] = "";

static APIRET   (APIENTRY *fnDosQueryExtLIBPATH)( PSZ, ULONG ) = NULL;
static APIRET   (APIENTRY *fnDosSetExtLIBPATH)( PSZ, ULONG ) = NULL;

#define NODLLENTRY      ((PFN)-1)

static bool ensure_loaded( ULONG ord, PFN *fn )
{
    char    old_lpath[1024];
    HMODULE hmod;

    if( *fn != NULL )
        return( *fn != NODLLENTRY );
    if( DosLoadModule( old_lpath, sizeof( old_lpath ), "DOSCALLS", &hmod ) == NO_ERROR ) {
        if( DosQueryProcAddr( hmod, ord, NULL, fn ) == NO_ERROR ) {
            return( true );
        }
    }
    *fn = NODLLENTRY;
    return( false );
}

#endif

char *GetEnvExt( const char *str )
{
#if defined( __OS2__ )
    char    *rc;

    rc = NULL;
    if( strcmp( str, BEGPATHNAME ) == 0 ) {
        if( ensure_loaded( ORD_DOS32QUERYEXTLIBPATH, (PFN *)&fnDosQueryExtLIBPATH ) ) {
            rc = os2BegLibPath;
            if( os2BegLibPath[0] == '\0' ) {
                if( fnDosQueryExtLIBPATH( os2BegLibPath, BEGIN_LIBPATH ) ) {
                    rc = NULL;
                }
            }
        }
        return( rc );
    }
    if( strcmp( str, ENDPATHNAME ) == 0 ) {
        if( ensure_loaded( ORD_DOS32QUERYEXTLIBPATH, (PFN *)&fnDosQueryExtLIBPATH ) ) {
            rc = os2EndLibPath;
            if( os2EndLibPath[0] == '\0' ) {
                if( fnDosQueryExtLIBPATH( os2EndLibPath, END_LIBPATH ) ) {
                    rc = NULL;
                }
            }
        }
        return( rc );
    }
#endif
    return( getenv( str ) );
}

int SetEnvExt( ENV_TRACKER *env )
{
#if defined( __OS2__ )
    int     rc;

    rc = -1;
    if( strcmp( env->name, BEGPATHNAME ) == 0 ) {
        if( ensure_loaded( ORD_DOS32SETEXTLIBPATH, (PFN *)&fnDosSetExtLIBPATH ) ) {
            if( env->value == NULL )
                env->value = "";
            strcpy( os2BegLibPath, env->value );
            rc = fnDosSetExtLIBPATH( os2BegLibPath, BEGIN_LIBPATH );
        }
        return( rc );
    }
    if( strcmp( env->name, ENDPATHNAME ) == 0 ) {
        if( ensure_loaded( ORD_DOS32SETEXTLIBPATH, (PFN *)&fnDosSetExtLIBPATH ) ) {
            if( env->value == NULL )
                env->value = "";
            strcpy( os2EndLibPath, env->value );
            rc = fnDosSetExtLIBPATH( os2EndLibPath, END_LIBPATH );
        }
        return( rc );
    }
#endif
#if defined( _MSC_VER )
    return( putenv( env->name ) );
#else
  #if !defined( __WATCOMC__ )
    if( env->value == NULL )
        return( unsetenv( env->name ) );
  #endif
    return( setenv( env->name, env->value, true ) );
#endif
}

int SetEnvSafe( const char *name, const char *value )
/****************************************************
 * This function takes over responsibility for freeing env
 */
{
    char        *p;
    ENV_TRACKER **walk;
    ENV_TRACKER *old;
    int         rc;
    size_t      len;
    size_t      len1;
    ENV_TRACKER *env;

    len = strlen( name );
    len1 = ( value == NULL ) ? 0 : strlen( value );
    env = MallocSafe( sizeof( ENV_TRACKER ) + len + len1 + 1 );
    // upper case the name
    p = env->name;
    while( *name != NULLCHAR ) {
        *p++ = ctoupper( *name++ );
    }
#ifdef _MSC_VER
    *p++ = '=';
    len++;  /* include '=' character to search */
#else
    *p++ = NULLCHAR;
#endif
    if( value == NULL ) {
        env->value = NULL;
        *p = NULLCHAR;
    } else {
        env->value = p;
        strcpy( env->value, value );
    }
    rc = SetEnvExt( env );          // put into environment
    if( *p == NULLCHAR ) {
        rc = 0;                     // we are deleting the envvar, ignore errors
    }
    for( walk = &envList; *walk != NULL; walk = &(*walk)->next ) {
#ifdef _MSC_VER
        if( strncmp( (*walk)->name, env->name, len ) == 0 ) {
#else
        if( strcmp( (*walk)->name, env->name ) == 0 ) {
#endif
            break;
        }
    }
    old = *walk;
    if( old != NULL ) {
        *walk = old->next;          // unlink old entry from chain
        FreeSafe( old );            // ...
    }
    if( env->value != NULL ) {
        env->next = envList;        // we put new entry into chain
        envList = env;              // ...
    } else {
        FreeSafe( env );            // we're deleting the entry
    }
    return( rc );
}


#if !defined(NDEBUG) || defined(DEVELOPMENT)
void SetEnvFini( void )
/*********************/
{
    ENV_TRACKER *cur;

    while( (cur = envList) != NULL ) {
        envList = cur->next;
        FreeSafe( cur );
    }
}
#endif
