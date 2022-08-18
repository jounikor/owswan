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
* Description:  Implementation of _popen() for OS/2 and Win32.
*
****************************************************************************/


#include "variety.h"
#include "widechar.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef __WIDECHAR__
    #include <wctype.h>
#else
    #include <ctype.h>
#endif
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <string.h>
#if defined( __NT__ )
    #include <windows.h>
#elif defined( __OS2__ )
    #include <wos2.h>
#endif
#include "bool.h"
#include "rtdata.h"
#include "liballoc.h"
#include "osver.h"


/*
 * Returns the number of whitespace-delimited words in 'command'.  If 'words'
 * is non-NULL, then each word will have memory allocated to hold it and will
 * be copied into the appropriate space in the array (e.g. the second word
 * will be copied into words[1]).  The array must be large enough, so just
 * call parse_words with 'words'==NULL before to get the proper size, then
 * do an alloca((n+1)*sizeof(CHAR_TYPE*)).  The caller is responsible for freeing
 * any memory allocated for the words.  Returns -1 on error.
 */

static int parse_words( const CHAR_TYPE *command, CHAR_TYPE **words )
/*******************************************************************/
{
    int                 numWords = 0;
    const CHAR_TYPE     *p = command;
    const CHAR_TYPE     *pLookAhead;
    int                 error = 0;
    size_t              len;

    while( *p != NULLCHAR ) {
        /*** Skip any leading whitespace ***/
        while( __F_NAME(isspace,iswspace)( (UCHAR_TYPE)*p ) ) {
            p++;
        }

        /*** Handle the word ***/
        if( *p == NULLCHAR )
            break;
        pLookAhead = p;
        while( *pLookAhead != NULLCHAR && !__F_NAME(isspace,iswspace)( (UCHAR_TYPE)*pLookAhead ) ) {
            pLookAhead++;
        }
        if( words != NULL ) {
            len = pLookAhead - p;       /* # of chars, excluding the null */
            words[numWords] = lib_malloc( (len + 1) * sizeof( CHAR_TYPE ) );
            if( words[numWords] == NULL ) {     /* break on error */
                error = 1;
                break;
            }
            __F_NAME(strncpy,wcsncpy)( words[numWords], p, len );
            words[numWords][len] = NULLCHAR;
        }

        p = pLookAhead;
        numWords++;
    }

    /*** If an error occurred, free any memory we've allocated ***/
    if( error ) {
        for( numWords--; numWords >= 0; numWords-- ) {
            lib_free( words[numWords] );
        }
        return( -1 );
    } else {
        if( words != NULL ) {
            words[numWords] = NULL;    /* last string */
        }
    }

    return( numWords );
}


/* Returns non-zero on success */

static int spawn_it( FILE *fp, const CHAR_TYPE *command )
/*******************************************************/
{
    int                 pid;
    int                 numWords;
    CHAR_TYPE           **words;    /* note: [0] and [1] used for "cmd.exe /c" */

    /*** Create an argv array from the command string ***/
    numWords = parse_words( command, NULL );
    if( numWords == -1 ) {
        return( 0 );
    }
    words = alloca( (numWords + 2 + 1) * sizeof( CHAR_TYPE * ) );
    if( words == NULL ) {
        return( 0 );
    }
    numWords = parse_words( command, &words[2] );
    if( numWords == -1 ) {
        return( 0 );
    }

    /*** Use CMD.EXE under NT and OS/2, and COMMAND.COM under Win95 ***/
#if defined( __OS2__ )
    words[0] = STRING( "cmd.exe" );
#else
    if( WIN32_IS_WIN95 ) {
        words[0] = STRING( "command.com" ); /* 95 */
    } else {
        words[0] = STRING( "cmd.exe" );     /* NT */
    }
#endif
    words[1] = STRING( "/c" );

    /*** Spawn the process ***/
    pid = __F_NAME(spawnvp,_wspawnvp)( P_NOWAIT, words[0], (const CHAR_TYPE **)&words[0] );
    if( pid == -1 ) {
        return( 0 );
    }
    _FP_PIPEDATA(fp).pid = pid;

    /*** Free any memory used by parse_words ('words' freed on return) ***/
    for( numWords--; numWords >= 2; numWords-- ) {
        lib_free( words[numWords] );
    }
    return( 1 );
}


/* Returns non-zero on success */

static int connect_pipe( FILE *fp, const CHAR_TYPE *command, int *handles,
                         bool readOrWrite, bool textOrBinary )
/************************************************************************/
{
#if defined( __NT__ )
    BOOL                rc;
    HANDLE              osHandle;
    HANDLE              oldHandle;
#elif defined( __OS2__ )
    APIRET              rc;
    HFILE               osHandle;
    HFILE               oldHandle;
#endif

    /* unused parameters */ (void)textOrBinary;

    if( !readOrWrite ) {
        /*** Change the standard input handle for process inheritance ***/
#if defined( __NT__ )
        osHandle = GetStdHandle( STD_INPUT_HANDLE );        /* get old */
        if( osHandle == INVALID_HANDLE_VALUE ) {
            return( 0 );
        }
        oldHandle = osHandle;
        rc = SetStdHandle( STD_INPUT_HANDLE,                /* set new */
                           (HANDLE)_os_handle(handles[0]) );
        if( rc == FALSE ) {
            SetStdHandle( STD_INPUT_HANDLE, oldHandle );
            return( 0 );
        }
#elif defined( __OS2__ )
        oldHandle = (HFILE)-1;                /* duplicate standard input */
        rc = DosDupHandle( STDIN_FILENO, &oldHandle );
        if( rc != NO_ERROR )
            return( 0 );
        osHandle = STDIN_FILENO;            /* use new standard input */
        rc = DosDupHandle( (HFILE)_os_handle(handles[0]), &osHandle );
        if( rc != NO_ERROR ) {
            DosClose( oldHandle );
            return( 0 );
        }
#endif

        /*** Spawn the process and go home ***/
        if( spawn_it( fp, command ) == 0 ) {
            return( 0 );
        }
#if defined( __NT__ )
        SetStdHandle( STD_INPUT_HANDLE, oldHandle );
#elif defined( __OS2__ )
        osHandle = STDIN_FILENO;
        rc = DosDupHandle( oldHandle, &osHandle );
#endif
        close( handles[0] );        /* parent process should close this */
    } else {
        /*** Change the standard output handle for process inheritance ***/
#if defined( __NT__ )
        osHandle = GetStdHandle( STD_OUTPUT_HANDLE );       /* get old */
        if( osHandle == INVALID_HANDLE_VALUE ) {
            return( 0 );
        }
        oldHandle = osHandle;
        rc = SetStdHandle( STD_OUTPUT_HANDLE,               /* set new */
                           (HANDLE)_os_handle(handles[1]) );
        if( rc == FALSE ) {
            SetStdHandle( STD_OUTPUT_HANDLE, oldHandle );
            return( 0 );
        }
#elif defined( __OS2__ )
        oldHandle = (HFILE)-1;              /* duplicate standard input */
        rc = DosDupHandle( STDOUT_FILENO, &oldHandle );
        if( rc != NO_ERROR ) {
            return( 0 );
        }
        osHandle = STDOUT_FILENO;           /* use new standard input */
        rc = DosDupHandle( (HFILE)_os_handle(handles[1]), &osHandle );
        if( rc != NO_ERROR ) {
            DosClose( oldHandle );
            return( 0 );
        }
#endif

        /*** Spawn the process and go home ***/
        if( spawn_it( fp, command ) == 0 ) {
            return( 0 );
        }
#if defined( __NT__ )
        SetStdHandle( STD_OUTPUT_HANDLE, oldHandle );
#elif defined( __OS2__ )
        osHandle = STDOUT_FILENO;
        rc = DosDupHandle( oldHandle, &osHandle );
#endif
        close( handles[1] );        /* parent process should close this */
    }

    return( 1 );
}


_WCRTLINK FILE *__F_NAME(_popen,_wpopen)( const CHAR_TYPE *command, const CHAR_TYPE *mode )
/*****************************************************************************************/
{
#if defined(__NT__)
    HANDLE              osHandle;
    BOOL                rc;
    int                 handleMode;
#elif defined( __OS2__ )
    APIRET              rc;
    OS_UINT             handleState;
#endif
    FILE *              fp;
    int                 handles[2];
    bool                textOrBinary;
    bool                readOrWrite;


    /*** Parse the mode string ***/
    switch( mode[0] ) {         /* read or write */
    case STRING( 'r' ):
        readOrWrite = true;
        break;
    case STRING( 'w' ):
        readOrWrite = false;
        break;
    default:
        return( NULL );
    }
    switch( mode[1] ) {         /* text or binary */
    case STRING( 't' ):
        textOrBinary = true;
        break;
    case STRING( 'b' ):
        textOrBinary = false;
        break;
    default:
        textOrBinary = ( _RWD_fmode != _O_BINARY );
    }

    /*** Create the pipe at the OS level ***/
    if( _pipe( handles, 0, (textOrBinary ? _O_TEXT : _O_BINARY) ) == -1 ) {
        return( NULL );
    }

    /*** Make read handle non-inheritable if reading ***/
    if( readOrWrite ) {
#if defined( __NT__ )
        rc = DuplicateHandle( GetCurrentProcess(),
                              (HANDLE)_os_handle(handles[0]),
                              GetCurrentProcess(), &osHandle, 0,
                              FALSE, DUPLICATE_SAME_ACCESS );
        if( rc == FALSE ) {
            return( 0 );
        }
        close( handles[0] );        /* don't need this any more */
        handleMode = _O_RDONLY | (textOrBinary ? _O_TEXT : _O_BINARY);
        handles[0] = _hdopen( (int)osHandle, handleMode );
        if( handles[0] == -1 ) {
            CloseHandle( osHandle );
            close( handles[1] );
            return( 0 );
        }
#elif defined( __OS2__ )
        rc = DosQFHandState( (HFILE)_os_handle(handles[0]), &handleState );
        if( rc != NO_ERROR ) {
            return( 0 );
        }
        handleState |= OPEN_FLAGS_NOINHERIT;
        handleState &= 0x00007F88;  /* some bits must be zero */
        rc = DosSetFHandState( (HFILE)_os_handle(handles[0]), handleState );
        if( rc != NO_ERROR ) {
            return( 0 );
        }
#endif
    } else {
        /*** Make write handle non-inheritable if writing ***/
#if defined (__NT__ )
        rc = DuplicateHandle( GetCurrentProcess(),
                              (HANDLE)_os_handle(handles[1]),
                              GetCurrentProcess(), &osHandle, 0,
                              FALSE, DUPLICATE_SAME_ACCESS );
        if( rc == FALSE ) {
            return( 0 );
        }
        close( handles[1] );        /* don't need this any more */
        handleMode = _O_WRONLY | (textOrBinary ? _O_TEXT : _O_BINARY);
        handles[1] = _hdopen( (int)osHandle, handleMode );
        if( handles[1] == -1 ) {
            CloseHandle( osHandle );
            close( handles[0] );
            return( 0 );
        }
#elif defined( __OS2__ )
        rc = DosQFHandState( (HFILE)_os_handle(handles[1]), &handleState );
        if( rc != NO_ERROR ) {
            return( 0 );
        }
        handleState |= OPEN_FLAGS_NOINHERIT;
        handleState &= 0x00007F88;  /* some bits must be zero */
        rc = DosSetFHandState( (HFILE)_os_handle(handles[1]), handleState );
        if( rc != NO_ERROR ) {
            return( 0 );
        }
#endif
    }

    /*** Create the pipe's FILE* ***/
    fp = __F_NAME(fdopen,_wfdopen)( handles[(readOrWrite ? 0 : 1)], mode );
    if( fp == NULL ) {
        close( handles[0] );
        close( handles[1] );
        return( NULL );
    }
    _FP_PIPEDATA(fp).isPipe = 1;
    _FP_PIPEDATA(fp).pid = -1;

    /*** Spawn the process ***/
    if( connect_pipe( fp, command, handles, readOrWrite, textOrBinary ) ) {
        return( fp );
    } else {
        close( handles[0] );
        close( handles[1] );
        return( NULL );
    }
}
