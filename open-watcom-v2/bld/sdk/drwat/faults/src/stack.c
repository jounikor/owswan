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
* Description:  Program to test stack unwinding at point of fault.
*
****************************************************************************/


#include <windows.h>

int     Cnt;

static void crash( void ) {
    char __far *a= (char __far *)0L;

    a[0] = 'a';
}

static void test6( void ) {
    crash();
}

static void test5( void ) {
    test6();
}

static void test4( void ) {
    test5();
}

static void test3( void ) {
    test4();
}

static void test2( void ) {
    test3();
}

static void test1( void ) {
    test2();
}

int PASCAL WinMain( HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int cmdshow)
{
    Cnt = 0;
    test1();
    return( 1 );
}
