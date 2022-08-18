/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  High level code generation routines. Lots of action here.
*
****************************************************************************/


extern void             GenProlog( void );
extern void             GenEpilog( void );
extern void             AddCacheRegs( void );
extern void             InitStackDepth( block * );
extern int              AskDisplaySize( level_depth level );
extern type_length      PushSize( type_length );
extern void             PushLocals( void );
extern void             SetTempLocation( name *, type_length );
extern type_length      TempLocation( name * );
extern bool             TempAllocBefore( void *, void * );
extern unsigned         DepthAlign( unsigned );
extern void             GenCallLabel( pointer );
extern void             GenLabelReturn( void );
extern void             GenObjCode( instruction * );

extern bool             DoVerify( vertype kind, instruction *ins );
