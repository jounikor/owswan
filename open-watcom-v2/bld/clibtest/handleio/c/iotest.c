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
* Description:  Non-exhaustive test of the C library handle I/O functions.
*
****************************************************************************/


#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/locking.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __SW_BW
  #include <wdefwin.h>
#endif

#define VERIFY( exp ) \
    if( !(exp) ) {                                      \
        fprintf( stdout,                                \
            "%s: ***FAILURE*** at line %d of %s.\n",    \
                ProgramName, __LINE__,                  \
                strlwr(__FILE__) );                     \
        fprintf( stdout, "%s\n", strerror(errno) );     \
        fflush( stdout );                               \
        NumErrors++;                                    \
        exit( EXIT_FAILURE );                           \
    }

#define VERIFYX( exp ) \
    if( !(exp) ) {                                      \
        fprintf( stdout,                                \
            "%s: ***FAILURE*** at line %d of %s.\n",    \
                ProgramName, __LINE__,                  \
                strlwr(__FILE__) );                     \
        fprintf( stdout, "%s\n", strerror(errno) );     \
        fflush( stdout );                               \
        NumErrors++;                                    \
        exit( EXIT_FAILURE );                           \
    } else {                                            \
        fprintf( stdout, "%s: OK at line %d of %s.\n",  \
                ProgramName, __LINE__,                  \
                strlwr(__FILE__) );                     \
        fflush( stdout );                               \
    }

void TestOpenClose( void );
void TestReadWrite( void );
void TestSize( void );
void TestFileno( void );
void TestDup( void );
void TestLocking( void );
void TestOsHandle( void );
void TestUnlink( void );


char ProgramName[128];                          /* executable filename */
int NumErrors = 0;                              /* number of errors */



/****
***** Program entry point.
****/

int main( int argc, char *argv[] )
{
#ifdef __SW_BW
    FILE *my_stdout;
    my_stdout = freopen( "tmp.log", "a", stdout );
    if( my_stdout == NULL ) {
        fprintf( stderr, "Unable to redirect stdout\n" );
        return( EXIT_FAILURE );
    }
#endif

    /* unused parameters */ (void)argc;

    /*** Initialize ***/
    strcpy( ProgramName, strlwr(argv[0]) );     /* store executable filename */

    /*** Test various functions ***/
    TestOpenClose();                            /* opening and closing */
    TestReadWrite();                            /* reading and writing */
    TestSize();                                 /* file size stuff */
    TestFileno();                               /* fileno() */
    TestDup();                                  /* handle duplication */
#if !defined( __UNIX__ ) && !defined( __RDOS__ )
    TestLocking();                              /* file locking */
    TestOsHandle();                             /* OS <--> POSIX handles */
#endif
    TestUnlink();                               /* file deletion */

    /*** Print a pass/fail message and quit ***/
    if( NumErrors != 0 ) {
        fprintf( stdout, "%s: FAILURE (%d errors).\n", ProgramName, NumErrors );
    } else {
        fprintf( stdout, "Tests completed (%s).\n", ProgramName );
    }
    fflush( stdout );
#ifdef __SW_BW
    if( NumErrors != 0 ) {
        fprintf( stderr, "%s: FAILURE (%d errors).\n", ProgramName, NumErrors );
    } else {
        fprintf( stderr, "Tests completed (%s).\n", ProgramName );
    }
    fflush( stderr );
    fclose( my_stdout );
    _dwShutDown();
#endif
    if( NumErrors != 0 )
        return( EXIT_FAILURE );
    return( EXIT_SUCCESS );
}



/****
***** Test creat(), open(), and close().
****/

void TestOpenClose( void )
{
    int                 handle;
    int                 status;

    handle = creat( "iotest.tmp", S_IREAD|S_IWRITE );
    VERIFY( handle != -1 );

    status = close( handle );
    VERIFY( status == 0 );

    handle = open( "iotest.tmp", O_RDWR|O_EXCL|O_CREAT, S_IREAD );
    VERIFY( handle == -1 );

    handle = open( "iotest.tmp", O_RDWR|O_EXCL|O_TRUNC );
    VERIFY( handle != -1 );

    status = close( handle );
    VERIFY( status == 0 );

    handle = sopen( "iotest.tmp", O_RDWR|O_TRUNC, SH_DENYNO );
    VERIFY( handle != -1 );

    errno = 0;
    setmode( handle, O_BINARY );
    VERIFY( errno == 0 );

    status = close( handle );
    VERIFY( status == 0 );
}



/****
***** Test read() and write().
****/

void TestReadWrite( void )
{
    int                 handle;
    int                 status;
    char                buf1[20] = "Hello world!";
    char                buf2[20];
    long                offset;

    handle = open( "iotest.tmp", O_RDWR|O_EXCL|O_TRUNC );
    VERIFY( handle != -1 );

    status = write( handle, buf1, strlen(buf1) );
    VERIFY( status == strlen(buf1) );

    offset = lseek( handle, 0L, SEEK_SET );
    VERIFY( offset == 0L );

    status = read( handle, buf2, 30 );
    VERIFY( status == strlen(buf1) );
    VERIFY( !memcmp( buf1, buf2, strlen(buf1) ) );

    status = close( handle );
    VERIFY( status == 0 );
}



/****
***** Test chsize(), filelength(), lseek(), tell(), and eof().
****/

void TestSize( void )
{
    int                 handle;
    int                 status;
    long                offset;
    int                 count;
    char                ch;
    struct stat         statBuf;

    handle = open( "iotest.tmp", O_RDWR );
    VERIFY( handle != -1 );

    status = chsize( handle, 0L );
    VERIFY( status == 0 );

    offset = filelength( handle );
    VERIFY( offset == 0 );

    status = chsize( handle, 8192L );
    VERIFY( status == 0 );

    for( count=0; count<8192; count++ ) {
        status = read( handle, &ch, 1 );
        VERIFY( status == 1 );
        VERIFY( ch == 0x00 );
    }
    VERIFY( eof(handle) == 1 );

    offset = filelength( handle );
    VERIFY( offset == 8192 );

    offset = lseek( handle, 16384, SEEK_SET );
    VERIFY( offset == 16384 );

    offset = tell( handle );
    VERIFY( offset == 16384 );

    status = write( handle, "!", 1 );
    VERIFY( status == 1 );

    offset = lseek( handle, 8192, SEEK_SET );
    VERIFY( offset == 8192 );

    for( count=0; count<8192; count++ ) {
        status = read( handle, &ch, 1 );
        VERIFY( status == 1 );
        VERIFY( ch == 0x00 );
    }

    status = read( handle, &ch, 1 );
    VERIFY( ch == '!' );

    status = fstat( handle, &statBuf );
    VERIFY( status == 0 );
    VERIFY( statBuf.st_size == 16385 );

    status = close( handle );
    VERIFY( status == 0 );
}



/****
***** Test fileno().
****/

void TestFileno( void )
{
    FILE *              fp;
    int                 handle;
    int                 status;
    long                size;

    fp = fopen( "iotest.tmp", "rb" );
    VERIFY( fp != NULL );

    handle = fileno( fp );
    VERIFY( handle != -1 );

    size = filelength( handle );
    VERIFY( size == 16385 );

    status = close( handle );
    VERIFY( status == 0 );
}



/****
***** Test dup() and dup2().
****/

void TestDup( void )
{
    int                 handle1, handle2;
    int                 status;
    long                offset;

    handle1 = open( "iotest.tmp", O_RDWR );
    VERIFY( handle1 != -1 );

    handle2 = dup( handle1 );
    VERIFY( handle2 != -1 );

    offset = lseek( handle1, 8192, SEEK_SET );
    VERIFY( offset == 8192 );

    offset = tell( handle2 );
    VERIFY( offset == 8192 );

    status = close( handle2 );
    VERIFY( status == 0 );

    handle2 = handle1 + 1;
    status = dup2( handle1, handle2 );
    // NB: the return value of dup2() differs between POSIX and traditional
    // DOSish implementations!
    VERIFY( status != -1 );

    offset = lseek( handle1, 42, SEEK_SET );
    VERIFY( offset == 42 );

    offset = tell( handle2 );
    VERIFY( offset == 42 );

    status = close( handle2 );
    VERIFY( status == 0 );

    status = close( handle1 );
    VERIFY( status == 0 );
}


#if !defined( __UNIX__ ) && !defined( __RDOS__ )

/****
***** Test lock(), locking(), and unlock().
****/

void TestLocking( void )
{
    int                 handle;
    int                 status;

    handle = open( "iotest.tmp", O_RDWR );
    VERIFYX( handle != -1 );

    status = lock( handle, 0, 16384 );
    VERIFYX( status == 0 );

    status = locking( handle, LK_LOCK, 16385 );
    VERIFYX( status == -1 );

    status = unlock( handle, 0, 16384 );
    VERIFYX( status == 0 );

    status = locking( handle, LK_UNLCK, 16385 );
    VERIFYX( status == -1 );

    status = close( handle );
    VERIFYX( status == 0 );
}



/****
***** Test _os_handle() and _hdopen().
****/

void TestOsHandle( void )
{
    int                 osHandle;
    int                 posixHandle;
    int                 status;

    posixHandle = open( "iotest.tmp", O_RDWR );
    VERIFY( posixHandle != -1 );

    osHandle = _os_handle( posixHandle );
    VERIFY( _os_handle( _hdopen(osHandle,O_RDWR) ) == osHandle );

    status = close( posixHandle );
    VERIFY( status == 0 );
}

#endif


/****
***** Test umask() and unlink().
****/

void TestUnlink( void )
{
    int                 handle;
    mode_t              oldMask;
    int                 status;

    status = unlink( "iotest.tmp" );
    VERIFY( status == 0 );

    oldMask = umask( S_IWRITE );

    handle = open( "iotest.tmp", O_RDWR|O_CREAT|O_TRUNC, S_IREAD|S_IWRITE );
    VERIFY( handle != -1 );

    status = close( handle );
    VERIFY( status == 0 );

#if !defined( __UNIX__ ) && !defined( __RDOS__ ) // This call would succeed
    status = unlink( "iotest.tmp" );
    VERIFY( status != 0 );
#endif

    status = chmod( "iotest.tmp", S_IREAD|S_IWRITE );
    VERIFY( status == 0 );

    status = unlink( "iotest.tmp" );
    VERIFY( status == 0 );
}
