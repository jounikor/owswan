/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  PE Dump Utility common structures and constants.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "watcom.h"
#include "exepe.h"
#include "exedos.h"
#include "bool.h"


#ifndef COMMON_H
#define COMMON_H

#define RT_NONE                 0
#define RT_COUNT                15

typedef enum {
    TABLE,
    DATA
} TableOrData;

typedef enum {
    NAME,
    ID
} NameOrID;

typedef struct ExeFile {
    FILE                        *file;
    dos_exe_header               dosHdr;
    pe_header                    pexHdr;
    unsigned_32                  pexHdrAddr;
    pe_object                    resObj;
    long                         resObjAddr;
    struct ResTableEntry        *tabEnt;
} ExeFile;

typedef struct ResTableEntry {
    resource_dir_header  header;
    struct ResDirEntry  *dirs;
} ResTableEntry;

typedef struct ResDirEntry {
    resource_dir_entry   dir;
    ResTableEntry       *table;
    struct ResDataEntry *data;
    NameOrID             nameID;
    TableOrData          entryType;
    unsigned_16         *name;
    unsigned_16          nameSize;
} ResDirEntry;

typedef struct ResDataEntry {
    resource_entry       entry;
} ResDataEntry;


#endif
