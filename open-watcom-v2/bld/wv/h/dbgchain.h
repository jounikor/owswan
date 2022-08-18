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


typedef struct call_chain {
    location_context    lc;
    char        *source_line;
    char        *symbol;
    unsigned    sym_len;
    boolbit     open : 1;
} call_chain;

typedef struct traceback {
    call_chain  *chain;
    int         allocated_size;
    int         total_depth;
    int         current_depth;
    int         clean_size;
} traceback;


typedef struct cached_traceback {
    traceback   a;
    traceback   b;
    traceback   *curr;
    traceback   *prev;
} cached_traceback;


extern address      FindNextIns( address a );
extern void         InitTraceBack( cached_traceback *tb );
extern void         FiniTraceBack( cached_traceback *tb );
extern void         UpdateTraceBack( cached_traceback *tb );
extern call_chain   *GetCallChain( cached_traceback *tb, int row );
extern void         ShowCalls( void );
extern void         UnWindToFrame( call_chain *chain, int row, int rows );
