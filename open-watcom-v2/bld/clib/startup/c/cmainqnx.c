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
* Description:  Runtime library startup for QNX.
*
****************************************************************************/


#include "variety.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <i86.h>
#include <limits.h>
#include <sys/magic.h>
#include <sys/proc_msg.h>
#include <sys/kernel.h>
#include <sys/utsname.h>
#include "rtstack.h"
#include "rtfpehdl.h"
#include "exitwmsg.h"
#include "initfini.h"
#include "thread.h"
#include "cmain.h"
#include "cinit.h"
#include "rtdata.h"
#include "fltsupp.h"
#include "slibqnx.h"
#include "_environ.h"


#if defined( _M_I86 )
#define __FAR __far
#else
#define __FAR
#endif

extern int main( int, char **, char ** );
#if defined( _M_I86 )
#pragma aux main __modify [__sp]
#else
#pragma aux main __modify [__esp]
#endif

void    __near *_endheap;                   /* temporary work-around */
char    *__near __env_mask;
char    ** _WCDATA environ;                 /* pointer to environment variables */
int     _argc;                              /* argument count  */
char    **_argv;                            /* argument vector */

#if defined( _M_I86 )

pid_t                   _my_pid;        /* some sort of POSIX dodad */
struct  _proc_spawn     *__cmd;         /* address of spawn msg */
int (__far * (__far *__f))();           /* Shared library jump table    */
extern  void __user_init( void );
#define __user_init() ((int(__far *)(void)) __f[1])()

#endif

static void _WCI86FAR __null_FPE_rtn( int fpe_type )
{
    fpe_type = fpe_type;
}

#if defined( _M_I86 )
static _WCNORETURN void _Not_Enough_Memory( void )
{
    __fatal_runtime_error( "Not enough memory", 1 );
    // never return
}

static void SetupArgs( struct _proc_spawn *cmd )
{
    register char *cp, **cpp, *mp, **argv;
    register int argc, envc, i;

    /*
     * Set up argv[] and count argc.
     */
    argc = cmd->argc + 1;
    /* The argc + 1 provides for a NULL pointer at end */
    argv = cpp = (char **)malloc( (argc + 1) * sizeof( char * ) );
    if( argv == NULL ) {
        _Not_Enough_Memory();
        // never return
    }

    cp = cmd->data;
    for( i = argc; i != 0; --i ) {
        *cpp++ = cp;
        while( *cp++ != '\0' ) {
            ;
        }
    }
    *cpp = NULL;
    if( *cp == '\0' )
        ++cp;

    /*
     * Set up envp and *environ.
     */

    envc = cmd->envc;

    environ = cpp = (char **)malloc( ENVARR_SIZE( envc ) );
    if( environ == NULL ) {
        _Not_Enough_Memory();
        // never return
    }
    __env_mask = mp = (char *)&cpp[ envc + 1 ];
    for( i = envc ; i != 0 ; --i ) {
        *mp++ = 0;  /* indicate environment string not alloc'd */
        *cpp++ = cp;
        while( *cp++ != '\0' ) {
            ;
        }
    }
    --cpp;                                      /* Back up to the __CWD     */
    if( !strncmp( "__CWD=", *cpp, 6 ) ) {       /* Did spawn pass __CWD ?   */
        /*
        Copy the cwd passed in an envar into magic and remove it from the
        environment. For old programs, also check the Q_CWD envar.
        The +6 skips over the __CWD=
        */
        if ( (__MAGIC.sptrs[3] = (char __FAR *)strdup( *cpp + 6 ) ) == NULL ) {
            _Not_Enough_Memory();
            // never return
        }
    } else {
        ++cpp;      /* __CWD not passed, point to normal end of environment */
    }
    *cpp = NULL;    /* Null terminate the environment                       */

    --cpp;                                      /* Back up to the __PFX     */
    if( !strncmp( "__PFX=", *cpp, 6 ) ) {       /* Did spawn pass __PFX ?   */
        /*
        Copy the pfx passed in an envar into magic and remove it from the
        environment. The +6 skips over the __PFX=
        */
        if ( (__MAGIC.sptrs[4] = (char __FAR *)strdup( *cpp + 6 ) ) == NULL ) {
            _Not_Enough_Memory();
            // never return
        }
    } else {
        ++cpp;      /* __PFX not passed, point to normal end of environment */
    }
    *cpp = NULL;    /* Null terminate the environment                       */
    _argc = argc - 1;
    _argv = &argv[1];
}

/* Define shared library callback routines */

static void __far * __SLIB_CALLBACK _s_malloc( int i )
{
    register void *p1;

    return( (p1 = calloc( i, 1 )) != NULL ? p1 : (void *)NULL );
}

static void __far * __SLIB_CALLBACK _s_calloc( int i, int n )
{
    register void *p1;

    return( (p1 = calloc( i, n )) != NULL ? p1 : (void *)NULL );
}

static void __far * __SLIB_CALLBACK _s_realloc( char __far *p, int n )
{
    register void *p1;

    return( (p1 = realloc( SLIB2CLIB( void, p ), n )) != NULL ? p1 : (void *)NULL );
}

static void  __SLIB_CALLBACK _s_free( void __far *p )
{
    free( SLIB2CLIB( void, p ) );
}

static char __far * __SLIB_CALLBACK _s_getenv( const char __far *p )
{
    register char *p1;

    return( (p1 = getenv( SLIB2CLIB( char, p ) )) ? p1 : (char *)NULL );
}

static char __far * __SLIB_CALLBACK _s_EFG_printf(
    char __far      *buffer,
    va_list __far   *pargs,
    void __far      *specs )
{
    return( (*__EFG_printf)( SLIB2CLIB( char, buffer ), SLIB2CLIB( va_list, pargs ), SLIB2CLIB( void, specs ) ) );
}

static void setup_slib( void )
{
    __f = __MAGIC.sptrs[0];         /* Set pointer to slib function table   */
    __MAGIC.malloc = &_s_malloc;    /* Pointers to slib callback routines   */
    __MAGIC.calloc = &_s_calloc;
    __MAGIC.realloc = &_s_realloc;
    __MAGIC.free = &_s_free;
    __MAGIC.getenv = &_s_getenv;
    __MAGIC.sptrs[1] = &_s_EFG_printf;
    __MAGIC.dgroup = _FP_SEG( &_STACKLOW );
}

void _CMain( free, n, cmd, stk_bot, pid )
    void                __near *free;       /* start of free space                  */
    short unsigned      n;                  /* number of bytes                      */
    struct _proc_spawn  __near *cmd;        /* pointer to spawn msg                 */
    short unsigned      stk_bot;            /* bottom of stack                      */
    pid_t               pid;                /* process id                           */
{

    _my_pid = pid;                          /* save POSIX process id                */
    __cmd = cmd;                            /* save address of spawn msg            */
    _STACKLOW = stk_bot;                    /* set stack low                        */
    _curbrk = (unsigned)free;               /* current end of dynamic memory        */
                                            /* pointer to top of memory owned by
                                               process                              */
    _RWD_FPE_handler = __null_FPE_rtn;
    _endheap = (char __near *)free + n;
    if( _endheap < free )
        _endheap = (char __near *)~0xfU;
    setup_slib();

    __InitRtns( 255 );                      /* call special initializer routines    */
    __MAGIC.sptrs[2] = cmd;                 /* save ptr to spawn message */
    SetupArgs( cmd );

    /*
       Invoke the "last chance" init code in the slib before executing the
       user's code. To avoid calling this vector on an older slib, we
       compare the first two entries in the jump table. Slib's without
       a __user_init function will have the first two entries both point to
       open, which we definately don't want to call as __user_init. Later on
       we can delete the compare and always call __user_init().
    */
    if ( (long)__f[0] != (long)__f[1] )
       __user_init();

    /* Invoke main skipping over the full path of the loaded command */
    exit( main( _argc, _argv, environ ) );    /* 02-jan-91 */
    // never return
}
#else

#pragma aux _s_EFG_printf __far __parm [__eax] [__edx] [__ebx]
static char *_s_EFG_printf(
    char    *buffer,
    va_list *pargs,
    void    *specs )
{
    return (*__EFG_printf)( buffer, pargs, specs );
}

extern unsigned short   _cs( void );
#pragma aux _cs = "mov  ax,cs" __value [__ax] __modify __exact __nomemory [__ax]

extern void setup_es( void );
#pragma aux setup_es = \
        "push ds"   \
        "pop es"    \
    __modify __exact __nomemory [__es]

void _CMain( int argc, char **argv, char **arge )
{
    union {
        void            *p;
        void            (*f)();
        unsigned short  s;
    }           tmp;
    int         def_errno;
    thread_data *tdata;

    _argc               = argc;
    _argv               = argv;
    environ             = arge;
    tmp.p               = &def_errno;
    __setmagicvar( &tmp.p, _m_errno_ptr );
    tmp.f               = (void (*)())_s_EFG_printf;
    __setmagicvar( &tmp.f, _m_efgfmt_off );
    tmp.s               = _cs();
    __setmagicvar( &tmp.s, _m_efgfmt_cs );
    _RWD_FPE_handler    = __null_FPE_rtn;
    __InitRtns( INIT_PRIORITY_THREAD );
    tdata = __alloca( __ThreadDataSize );
    memset( tdata, 0, __ThreadDataSize );
    // tdata->__allocated = 0;
    tdata->__data_size = __ThreadDataSize;
    __QNXInit( tdata );
    __InitRtns( 255 );
    setup_es();
    exit( main( argc, argv, arge ) );
    // never return
}
#endif
