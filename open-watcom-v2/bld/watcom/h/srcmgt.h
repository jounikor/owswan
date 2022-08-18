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
* Description:  Source file management for debugger, profiler, etc.
*
****************************************************************************/


#define FREADLINE_ERROR ((size_t)-1)

struct browser;

extern struct browser   *FOpenSource( const char *name, sm_mod_handle mod, sm_cue_fileid id );
extern void             FDoneSource( struct browser * );
extern unsigned long    FSize( struct browser * );
extern unsigned long    FLastOffset( struct browser * );
extern int              FileIsRemote( struct browser * );
extern char             *FGetName( struct browser * );
extern int              FCurrLine( struct browser * );
extern size_t           FReadLine( struct browser *, int, int, char *, size_t );
extern void             FClearOpenSourceCache( void );
