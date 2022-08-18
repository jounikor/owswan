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
* Description:  Codegen instructions specific to MIPS.
*
****************************************************************************/


_G_( G_NO ),
_G_( G_CALL ),
_G_( G_CALLI ),
_G_( G_BINARY ),
_G_( G_BINARY_IMM ),
_G_( G_UNARY ),
_G_( G_LOAD ),
_G_( G_LEA_HIGH ),
_G_( G_LEA ),
_G_( G_MOVE_UI ),
_G_( G_LOAD_ADDR ),
_G_( G_STORE ),
_G_( G_MOVE ),
_G_( G_CVTTS ),
_G_( G_CVTTQ ),
_G_( G_CVTQS ),
_G_( G_CVTQT ),
_G_( G_CONDBR ),
_G_( G_EXTRACT_LOW ),
_G_( G_EXTRACT_HIGH ),
_G_( G_LOAD_UA ),
_G_( G_STORE_UA ),
_G_( G_BINARY_FP ),
_G_( G_MOVE_FP ),
_G_( G_DEBUG ),
_G_( G_CHOP ),
_G_( G_ZERO ),
_G_( G_SIGN ),
_G_( G_MI8TOFREG ),
_G_( G_FREGTOMI8 ),
_G_( G_BYTE_CONST ),
