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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include <stdio.h>

#define SMTabIntervalGet()              8

#define _SMAlloc( pointer, size )       (pointer) = ProfAlloc( size )
#define _SMFree( pointer )              ProfFree( pointer )

#define sm_file_handle                  FILE *
#define sm_mod_handle                   int
#define sm_cue_fileid                   int
#define sm_read_len                     size_t

#define SM_NO_MOD                       ((sm_mod_handle)-1)
#define SM_BUF_SIZE                     512UL

#define SMSeekStart( fp )               fseek( fp, 0L, SEEK_CUR )
#define SMSeekOrg( fp, offset )         fseek( fp, offset, SEEK_SET )
#define SMSeekEnd( fp )                 fseek( fp, 0L, SEEK_END )
#define SMSeekFail(x)                   ((x) != 0)

#define SMTell( fp )                    ftell( fp )

#define SMOpenRead( name )              fopen( name, "rb" )
#define SMNilHandle( fp)                ( fp == NULL )
#define SMClose( fp )                   fclose( fp )

#define SMReadStream( fp, buff, len )   fread( buff, 1, len, fp )
#define SMReadError( fp, len )          ( ferror( fp ) != 0 )

#define SMFileRemote( fp )              0


extern void *ProfAlloc( size_t size );
extern void ProfFree( void *ptr );
