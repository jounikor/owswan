/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Declaration of RFX functions for remote side.
*
****************************************************************************/


extern error_handle     RemoteRename( const char *, const char * );
extern error_handle     RemoteMkDir( const char * );
extern error_handle     RemoteRmDir( const char * );
extern error_handle     RemoteSetDrv( int );
extern int              RemoteGetDrv( void );
extern error_handle     RemoteSetCWD( const char * );
extern long             RemoteGetFileAttr( const char * );
extern error_handle     RemoteSetFileAttr( const char * , long );
extern long             RemoteGetFreeSpace( int );
extern error_handle     RemoteDateTime( sys_handle , int *, int *, int );
extern error_handle     RemoteGetCwd( int, char *, trap_elen );
extern unsigned         RemoteNameToCanonical( char *name, char *fullname, trap_elen fullname_len );
extern error_handle     RemoteFindFirst( const char *, rfx_find *, trap_elen , int );
extern int              RemoteFindNext( rfx_find *, trap_elen );
extern error_handle     RemoteFindClose( rfx_find *, trap_elen );
