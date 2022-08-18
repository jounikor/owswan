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
* Description:  Print module compilation statistics.
*
****************************************************************************/


#include "cvars.h"


void InitStats( void )
{
    CompFlags.stats_printed = false;
    CompFlags.extra_stats_wanted = false;
    SrcLineCount = 0;
    IncLineCount = 0;
    WngCount = 0;
    ErrCount = 0;
    GblSymCount = 0;
    LclSymCount = 0;
    MacroCount = 0;
    MacroSize = 0;
    EnumCount = 0;
    TagCount = 0;
    FieldCount = 0;
    TypeCount = 0;
    TmpSymCount = 0;
    LitPoolSize = 0;
    LitCount = 0;
    /* still useful for internal debugging */
    FuncCount = 0;
}

void PrintStats( void )
{
    FCB         *nest_fcb;
    char        *fname;
    char        msgbuf[MAX_MSG_LEN];
    int         len;

    if( !CompFlags.stats_printed ) {
        len = 0;
        fname = WholeFName;
        if( fname == NULL )
            fname = "";
        len += sprintf( &msgbuf[len], "%s: ", fname );
        if( SrcLineCount == 0 ) {
            nest_fcb = SrcFile;
            if( nest_fcb != NULL ) {
                while( nest_fcb->prev_file != NULL ) {
                    IncLineCount += nest_fcb->src_line_cnt;
                    nest_fcb = nest_fcb->prev_file;
                }
                SrcLineCount = nest_fcb->src_line_cnt;
            }
        }
        len += sprintf( &msgbuf[len], "%u lines, ", SrcLineCount );
        if( IncLineCount != 0 ) {
            len += sprintf( &msgbuf[len], "included %u, ", IncLineCount );
        }
        len += sprintf( &msgbuf[len], "%u warnings, ", WngCount );
        len += sprintf( &msgbuf[len], "%u errors", ErrCount );
        BannerMsg( msgbuf );
        CompFlags.stats_printed = true;
    }
}
