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


#ifndef _WPASMFIL_H
#define _WPASMFIL_H

typedef struct src_info {
    unsigned long               line;
    void                        *src_file;
} src_info;

typedef struct asm_info {
    address                     addr;
    clicks_t                    tick_count;
} asm_info;

typedef struct wp_asmline {
    boolbit                     source_line : 1;
    union {
        asm_info                asm_line;
        src_info                src;
    } u;
} wp_asmline;

typedef struct wp_asm_groups {
    wp_asmline                  *asm_lines;
} wp_asm_groups;

typedef struct wp_asmfile {
    wp_asm_groups               *asm_data;
    FILE                        *fp;
    char                        *asm_buff;
    clicks_t                    max_time;
    size_t                      asm_buff_len;
    int                         asm_rows;
    int                         asm_groups;
    int                         entry_line;
} wp_asmfile;

#define MAX_ASM_LINE_INDEX      (SHRT_MAX / sizeof( wp_asmline ))
#define MAX_ASM_LINE_SIZE       (MAX_ASM_LINE_INDEX * sizeof( wp_asmline ))

extern wp_asmline   *WPGetAsmLoc( wp_asmfile *wpasm_file, int row, int *group_loc, int *row_loc );
extern wp_asmfile   *WPAsmOpen( sio_data *curr_sio, int src_row, bool quiet );
extern void         WPAsmClose( wp_asmfile *wpasm_file );
extern char         *WPAsmGetLine( a_window wnd, int line );
extern int          WPAsmFindSrcLine( sio_data *curr_sio, int line );

#endif
