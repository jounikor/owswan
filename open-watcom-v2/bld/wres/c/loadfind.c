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
* Description:  Locate resources in an executable file.
*
****************************************************************************/


#include <string.h>
#include "wresall.h"
#include "wresset2.h"
#include "wresrtns.h"
#include "machtype.h"
#include "wdbginfo.h"
/* Include patch signature header shared with BPATCH */
#include "patchsig.h"


long    WResFileShift = 0;

bool FindResourcesX( PHANDLE_INFO hinfo, bool res_file )
/*************************************************************
 * set position of resource info in the file (WResFileShift)
 * it is 0 if it is external resource file (GUI project)
 * or look for the resource information in a debugger record at the end of file
 */
{
    long                currpos;
    long                offset;
    master_dbg_header   header;
    bool                notfound;
    char                buffer[sizeof( PATCH_LEVEL )];

    notfound = !res_file;
    WResFileShift = 0;
    if( notfound ) {
        offset = sizeof( master_dbg_header );
        if( !WRESSEEK( hinfo->fp, -(long)sizeof( PATCH_LEVEL ), SEEK_END ) ) {
            if( WRESREAD( hinfo->fp, buffer, sizeof( PATCH_LEVEL ) ) == sizeof( PATCH_LEVEL ) ) {
                if( memcmp( buffer, PATCH_LEVEL, PATCH_LEVEL_HEAD_SIZE ) == 0 ) {
                    offset += sizeof( PATCH_LEVEL );
                }
            }
        }
        WRESSEEK( hinfo->fp, -offset, SEEK_END );
        currpos = WRESTELL( hinfo->fp );
        for( ;; ) {
            WRESREAD( hinfo->fp, &header, sizeof( master_dbg_header ) );
            if( header.signature == WAT_RES_SIG ) {
                notfound = false;
                WResFileShift = currpos - header.debug_size + sizeof( master_dbg_header );
                break;
            } else if( header.signature == WAT_DBG_SIGNATURE ||
                       header.signature == FOX_SIGNATURE1 ||
                       header.signature == FOX_SIGNATURE2 ) {
                currpos -= header.debug_size;
                WRESSEEK( hinfo->fp, currpos, SEEK_SET );
            } else {        /* did not find the resource information */
                break;
            }
        }
    }
    return( notfound );
}

bool FindResources( PHANDLE_INFO hinfo )
{
    return( FindResourcesX( hinfo, false ) );
}
