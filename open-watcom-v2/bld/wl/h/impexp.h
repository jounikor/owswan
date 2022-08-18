/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Declarations related to exports and imports.
*
****************************************************************************/


typedef struct entry_export {
    struct entry_export     *next;
    name_strtab             name;
    ordinal_t               ordinal;
    unsigned_16             iopl_words;
    boolbit                 isresident  : 1;
    boolbit                 isexported  : 1;
    boolbit                 ismovable   : 1;
    boolbit                 isanonymous : 1;
    boolbit                 isfree      : 1;
    boolbit                 isprivate   : 1;
    boolbit                 isiopl      : 1;
    symbol                  *sym;
    char                    *impname;
    targ_addr               addr;
} entry_export;

extern ordinal_t        FindEntryOrdinal( targ_addr, group_entry * );
extern void             FreeExportList( void );
extern void             AddToExportList( entry_export * );
extern void             MSExportKeyword( const length_name *, const length_name *, unsigned, ordinal_t );
extern void             MSImportKeyword( symbol *, const length_name *, const length_name *, ordinal_t );
extern dll_sym_info     *AllocDLLInfo( void );
extern void             FreeImport( dll_sym_info * );
extern entry_export     *AllocExport( const char *, size_t );
extern char             *ImpModuleName( dll_sym_info * );
extern bool             IsSymElfImported( symbol * );
extern bool             IsSymElfExported( symbol * );
extern bool             IsSymElfImpExp( symbol * );
extern void             KillDependantSyms( symbol * );
extern void             CheckExport( const char *, ordinal_t, bool );
