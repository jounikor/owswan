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


typedef enum {
    WF_CODE = 1,
    WF_DATA = 2
} walk_find;

#define SKIP_ENTRIES 50
typedef struct {
    int                 numrows;
    int                 i;
    void                *list;
    void                **skip;
    int                 sort_size;
    walk_find           type;
    boolbit             d2_only : 1;
} name_list;

#define SNT_QUALIFIED   SNT_LAST
#define SNT_PARSEABLE   (SNT_QUALIFIED + 1)

extern void             NameListAddModules( name_list *name, mod_handle mod, bool d2_only, bool dup_ok );
extern int              NameListNumRows( name_list *name );
extern unsigned         NameListName( name_list *name, int member_idx, char *buff, symbol_name_type snt );
extern void             NameListInit( name_list *name, walk_find type );
extern void             NameListFree( name_list *name );
extern address          NameListAddr( name_list *name, int member_idx );
extern sym_handle       *NameListHandle( name_list *name, int member_idx );
