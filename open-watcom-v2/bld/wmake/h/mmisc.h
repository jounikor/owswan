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
* Description:  misc.c interfaces. - Note it is not mmisc.c
*
****************************************************************************/


#ifndef _MISC_H
#define _MISC_H  1

extern char         *FixName( char *name );
extern bool         FNameEq( const char *a, const char *b );
#ifdef USE_FAR
extern bool         FarFNameEq( const char FAR *a, const char FAR *b );
#else
#define FarFNameEq  FNameEq
#endif
extern char const   *DoWildCard( const char *base );
extern void         DoWildCardClose( void );
extern int          KWCompare( const void *p1, const void *p2 );
extern char         *SkipWS( const char *p );
extern char         *FindNextWS( const char *str );
extern char         *FindNextWSorEqual( const char *str );

#endif
