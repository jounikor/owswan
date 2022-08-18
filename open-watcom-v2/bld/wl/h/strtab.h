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


#ifndef _STRTAB_H_INCLUDED
#define _STRTAB_H_INCLUDED

typedef struct stringblock STRINGBLOCK;

typedef struct {
    STRINGBLOCK *data;
    size_t      currbase;
} stringtable;

typedef void    write_strtable_fn( void *, const char *, size_t );

extern void     InitStringTable( stringtable *, bool );
extern char     *AddBufferStringTable( stringtable *, const void *, size_t );
extern char     *AddSymbolStringTable( stringtable *, const char *, size_t );
extern char     *AddStringStringTable( stringtable *, const char * );
extern size_t   AddStringStringTableOffs( stringtable *, const char * );
extern void     AddCharStringTable( stringtable *, char );
extern void     ZeroStringTable( stringtable *, size_t );
extern void     WriteStringTable( stringtable *, write_strtable_fn *, void * );
extern void     FiniStringTable( stringtable * );
extern size_t   GetStringTableSize( stringtable * );

#endif
