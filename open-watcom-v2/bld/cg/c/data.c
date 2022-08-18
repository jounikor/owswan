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
* Description:  Global code generator data.
*
****************************************************************************/


#define HW_DEFINE_VARS
#include "_cgstd.h"
#include "coderep.h"
#include "cgswitch.h"
#include "data.h"

block                   *HeadBlock;
block                   *BlockList;
block                   *CurrBlock;
int                     InsId;
int                     TempId;
conflict_node           *ConfList;
proc_def                *CurrProc;
name                    *Names[N_CLASS_MAX];
name                    *LastTemp;
name                    *DummyIndex;
source_line_number      SrcLine;
cg_switches             Model;
cg_target_switches      TargetModel;
cg_target_switches      SaveTargetModel;
global_bit_set          MemoryBits;
type_class_def          ClassPointer;
bool                    BlockByBlock;
type_length             MaxStack;
type_def                *TypeBoolean;
type_def                *TypeInteger;
type_def                *TypeUnsigned;
type_def                *TypePtr;
type_def                *TypeNone;
type_def                *TypeProcParm;
type_def                *TypeNearInteger;
type_def                *TypeLongInteger;
type_def                *TypeLongLongInteger;
type_def                *TypeHugeInteger;
hw_reg_set              GivenRegisters;
bool                    BlocksUnTrimmed;
an                      AddrList;
segment_id              DbgLocals;
segment_id              DbgTypes;
unsigned_16             TypeIdx;
int                     InOptimizer;
byte                    OptForSize;
bool                    HaveLiveInfo;
bool                    HaveDominatorInfo;
pointer_uint            FrlSize;
bool                    HaveCurrBlock;
proc_revision           CGProcessorVersion;
