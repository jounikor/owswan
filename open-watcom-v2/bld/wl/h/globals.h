/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2022 The Open Watcom Contributors. All Rights Reserved.
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


#ifdef export
#define GLOB       /**/
#else
#define GLOB       extern
#endif

#define SECTOR_SIZE     512
#define MAX_HEADROOM    _4KB

/* linker global variables */
GLOB outfilelist    *OutFiles;      /* list of files which will be written to   */
GLOB char           *MapFName;      /* name of map file                         */
GLOB f_handle       MapFile;        /* handle of map file                       */
GLOB targ_addr      StackAddr;      /* initial stack address                    */
GLOB linkflag       LinkFlags;      /* flags to indicate presence of parms      */
GLOB stateflag      LinkState;      /* flags to indicate linker state           */
GLOB mapflag        MapFlags;       /* flags dealing with the map file          */
GLOB file_list      *ObjLibFiles;   /* list of library files (for searching)    */
GLOB mod_entry      *LibModules;    /* library modules (for processing)         */
GLOB group_entry    *Groups;        /* pointer to defined groups                */
GLOB group_entry    *AbsGroups;     /* pointer to defined absolute groups       */
GLOB group_entry    *DataGroup;     /* pointer to DGROUP                        */
GLOB group_entry    *IDataGroup;    /* pointer to .idata                        */
GLOB byte           *ObjBuff;       /* offset into object buffer                */
GLOB mod_entry      *CurrMod;       /* pointer to current mod_entry struct      */
GLOB byte           *EOObjRec;      /* end of object buffer                     */
GLOB targ_addr      CurrLoc;        /* current location pointer                 */
GLOB int            RecNum;         /* record number of the obj file            */
GLOB seg_leader     *CurrentSeg;    /* current segment being allocated          */
GLOB section        *Root;          /* pointer to root information              */
GLOB section        *CurrSect;      /* current section for file processing      */
GLOB offset         StackSize;      /* size of stack                            */
GLOB section        *NonSect;       /* non-overlay-class section                */
GLOB path_entry     *ObjPath;       /* path for object files                    */
GLOB path_entry     *UsrLibPath;    /* path for library files (LibPath)         */
GLOB char           *LibPath;       /* LIB environment path                     */
GLOB char           *ExePath;       /* PATH environment path                    */
GLOB char           *TokBuff;       /* Multi-purpose large buffer               */
GLOB unsigned       TokSize;        /* size of above buffer                     */
GLOB offset         PackCodeLimit;  /* Maximum size of a packed code seg        */
GLOB offset         PackDataLimit;  /* Maximum size of a packed data seg        */
GLOB symbol         *HeadSym;       /* head of symbol table list                */
GLOB dbi_flags      DBIFlag;
GLOB struct fmt_data FmtData;       /* format specific data                     */
GLOB obj_format     ObjFormat;
GLOB mod_entry      *FakeModule;    /* Module containing all the extra sections */
GLOB unsigned_32    NumImports;
GLOB unsigned_32    NumExports;

#undef GLOB
