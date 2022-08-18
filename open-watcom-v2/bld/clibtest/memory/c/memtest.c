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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/*
 *  MEMTEST.C
 *  Non-exhaustive test of the C library memory manipulation functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined( _M_IX86 )
    #include <i86.h>
#endif
#ifdef __SW_BW
  #include <wdefwin.h>
#endif


#define VERIFY( exp ) \
    if( !(exp) ) {                                          \
        printf( "%s: ***FAILURE*** at line %d of %s.\n",    \
                ProgramName, __LINE__,                      \
                strlwr(__FILE__) );                         \
        NumErrors++;                                        \
        exit( EXIT_FAILURE );                               \
    }

void TestCompare( void );
void TestCompareF( void );
void TestCopy( void );
void TestCopyF( void );
void TestOverlap( void );
void TestOverlapF( void );
void TestMisc( void );


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
    strcpy( ProgramName, strlwr(argv[0]) );     /* store filename */

    /*** Test various functions ***/
    TestCompare();                              /* comparing stuff */
    TestCopy();                                 /* copying stuff */
    TestOverlap();                              /* test overlapping copy */
    TestMisc();                                 /* other stuff */

#if defined( _M_IX86 )
    TestCompareF();
    TestCopyF();
    TestOverlapF();
#endif

    /*** Print a pass/fail message and quit ***/
    if( NumErrors != 0 ) {
        printf( "%s: FAILED.\n", ProgramName );
    } else {
        printf( "Tests completed (%s).\n", ProgramName );
    }
#ifdef __SW_BW
    if( NumErrors != 0 ) {
        fprintf( stderr, "%s: FAILED.\n", ProgramName );
    } else {
        fprintf( stderr, "Tests completed (%s).\n", ProgramName );
    }
    fclose( my_stdout );
    _dwShutDown();
#endif

    if( NumErrors != 0 )
        return( EXIT_FAILURE );
    return( EXIT_SUCCESS );
}



/****
***** Test memcmp(), memicmp(), and memchr().
****/

void TestCompare( void )
{
    char                buf[80];
    char *              ptr;
    int                 status;

    strcpy( buf, "Foo !" );                     /* initialize string */

    status = memcmp( buf, "Foo", 3 );           /* compare */
    VERIFY( status == 0 );

    status = memcmp( buf, "Foo!", 4 );          /* compare */
    VERIFY( status != 0 );

    status = memicmp( buf, "FOO", 3 );          /* compare */
    VERIFY( status == 0 );

    status = memicmp( buf, "fOo!", 4 );         /* compare */
    VERIFY( status != 0 );

    ptr = memchr( buf, '~', 6 );                /* try to find a tilde */
    VERIFY( ptr == NULL );

    ptr = memchr( buf, '!', 6 );                /* find the '!' */
    VERIFY( ptr == buf+4 );
}


#if defined( _M_IX86 )
void TestCompareF( void )
{
    char                buf[80];
    char __far *        ptr;
    int                 status;

    strcpy( buf, "Foo !" );                     /* initialize string */

    status = _fmemcmp( buf, "Foo", 3 );         /* compare */
    VERIFY( status == 0 );

    status = _fmemcmp( buf, "Foo!", 4 );        /* compare */
    VERIFY( status != 0 );

    ptr = _fmemchr( buf, '~', 6 );              /* try to find a tilde */
    VERIFY( ptr == NULL );

    ptr = _fmemchr( buf, '!', 6 );              /* find the '!' */
    VERIFY( ptr == buf+4 );
}
#endif



/****
***** Test memcpy(), memccpy(), and movedata().
****/

void TestCopy( void )
{
    char                bufA[80], bufB[80];

    strcpy( bufB, "Hello, world!" );            /* initialize bufB */
    memccpy( bufA, bufB, 'o', strlen(bufB)+1 ); /* copy "Hello" to bufA */
    VERIFY( !memcmp(bufA, "Hello", 5) );        /* ensure copied ok */

    memcpy( bufA, bufB, strlen(bufB)+1 );       /* copy to bufA */
    VERIFY( !strcmp(bufA, bufB) );              /* ensure copied ok */
}


#if defined( _M_IX86 )
void TestCopyF( void )
{
    char __far          bufA[80], bufB[80];
    char __far          testStr[] = "Foo Bar Goober Blah";

    strcpy( bufB, "Hello, world!" );            /* initialize bufB */
    _fmemccpy( bufA, bufB, 'o', strlen(bufB)+1 );   /* copy "Hello" to bufA */
    VERIFY( !_fmemcmp(bufA, "Hello", 5) );      /* ensure copied ok */

    _fmemcpy( bufA, bufB, strlen(bufB)+1 );     /* copy to bufA */
    VERIFY( !_fstrcmp(bufA, bufB) );            /* ensure copied ok */

    movedata( _FP_SEG(bufA), _FP_OFF(bufA),     /* copy data */
              _FP_SEG(testStr), _FP_OFF(testStr),
              _fstrlen(testStr) );
    VERIFY( !_fmemcmp(bufA, testStr, _fstrlen(testStr)) );
}
#endif



/****
***** Test memmove().
****/

void TestOverlap( void )
{
    char                bufA[80], bufB[80];

    strcpy( bufA, " Hello, world!" );           /* initialize string */

    memmove( bufB, bufA, strlen(bufA)+1 );      /* copy string */
    VERIFY( !strcmp(bufB, bufA) );

    memmove( bufA, bufA+1, strlen(bufA+1) );    /* shift one character over */
    VERIFY( !strcmp(bufA, "Hello, world!!") );

    memmove( bufA+1, bufA, strlen(bufA+1) );
    VERIFY( !strcmp(bufA, "HHello, world!") );
}


#if defined( _M_IX86 )
void TestOverlapF( void )
{
    char                bufA[80], bufB[80];

    strcpy( bufA, " Hello, world!" );           /* initialize string */

    _fmemmove( bufB, bufA, strlen(bufA)+1 );    /* copy string */
    VERIFY( !strcmp(bufB, bufA) );

    _fmemmove( bufA, bufA+1, strlen(bufA+1) );  /* shift one character over */
    VERIFY( !strcmp(bufA, "Hello, world!!") );

    _fmemmove( bufA+1, bufA, strlen(bufA+1) );
    VERIFY( !strcmp(bufA, "HHello, world!") );
}
#endif



/****
***** Test memset(), _fmemset(), and swab().
****/

void TestMisc( void )
{
    char                bufA[80], bufB[80];
    void *              addr;
    int                 count;

    addr = memset( bufA, 0, 80 );            /* zero out memory */
    VERIFY( addr == bufA );

    for( count=0; count<80; count++ )           /* ensure all zero bytes */
        VERIFY( bufA[count] == 0x00 );

    #if defined( _M_IX86 )
    {
        void __far *    addrFar;
        addrFar = _fmemset( bufB, 0, 80 );   /* zero out memory */
        VERIFY( addrFar == bufB );

        for( count=0; count<80; count++ ) {     /* ensure all zero bytes */
            VERIFY( bufB[count] == 0x00 );
        }
    }
    #else
        memset( bufB, 0, 80 );               /* zero out memory */
    #endif

    strcpy( bufA, "eHll,ow rodl" );             /* initialize string */
    swab( bufA, bufB, strlen(bufA) );           /* swap pairs of characters */
    VERIFY( !strcmp(bufB, "Hello, world") );    /* ensure swapped ok */
}
