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
* Description:  Base routine for instruction creation prototypes
*
****************************************************************************/


extern void             InitIns( void );
extern bool             InsFrlFree( void );
extern void             FreeIns( instruction * );
extern instruction      *NewIns( opcnt );
extern instruction      *MakeNop( void );
extern instruction      *MakeNary( opcode_defs, name *, name *, name *, type_class_def, type_class_def, opcnt );
extern instruction      *MakeMove( name *, name *, type_class_def );
extern instruction      *MakeUnary( opcode_defs, name *, name *, type_class_def );
extern instruction      *MakeConvert( name *, name *, type_class_def, type_class_def );
extern instruction      *MakeRound( name *, name *, type_class_def, type_class_def );
extern instruction      *MakeBinary( opcode_defs, name *, name *, name *, type_class_def );
extern instruction      *MakeCondition( opcode_defs, name *, name *, byte, byte, type_class_def );
