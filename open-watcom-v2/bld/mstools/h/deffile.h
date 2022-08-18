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


#ifndef _DEFFILE_H
#define _DEFFILE_H

/*
 * Data type for handling options which can take several values.  Similar
 * to the OPT_STRING type.
 */
typedef struct StringList {
    struct StringList * next;
    char *              str;
} StringList;


/*
 * Define the .def file info structure.
 */
typedef struct DefInfo {
    char *              name;
    int                 makeDll;
    char *              internalDllName;
    char *              baseAddr;
    char *              stub;
    StringList *        description;
    StringList *        exports;
    char *              stacksize;
    char *              heapsize;
    char *              version;
} DefInfo;


/*
 * Function prototypes.
 */
#ifdef __TARGET_AXP__
    extern DefInfo *ParseDefFile( const char *filename, int dofuzzylink );
#else
    extern DefInfo *ParseDefFile( const char *filename );
#endif

extern void     FreeDefInfo( DefInfo *p );

#endif
