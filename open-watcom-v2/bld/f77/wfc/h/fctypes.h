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
* Description:  get typing info procedure prototypes
*
****************************************************************************/


extern  cg_type         MkCGType( PTYPE typ );
extern  cg_type         GetType( unsigned_16 typ_info );
extern  cg_type         GetType1( unsigned_16 typ_info );
extern  cg_type         GetType2( unsigned_16 typ_info );
extern  cg_type         F77ToCGType( sym_id sym );
extern  cg_type         ResCGType( cg_type typ1, cg_type typ2 );
extern  bool            DataPointer( cg_type typ );
extern  bool            TypeCGInteger( cg_type typ );
extern  bool            TypePointer( cg_type typ );
extern  cg_type         PromoteToBaseType( cg_type typ );
