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
* Description:  definitions for run-time routines
*
****************************************************************************/


// PROTOTYPES:

PTREE RunTimeCall(              // GENERATE A RUN-TIME CALL PARSE SUBTREE
    PTREE expr,                 // - expression for operands
    TYPE type,                  // - type for function return
    RTF rt_code )               // - code for function
;
bool RunTimeIsThrow(            // TEST IF FUNCTION IS A C++ THROW
    SYMBOL func )               // - function symbol
;
SYMBOL RunTimeCallSymbol(       // GET SYMBOL FOR A RUN-TIME CALL
    RTF rt_code )               // - code for call
;
const char *RunTimeCodeString(  // GET IMPORT STRING FOR RUN-TIME FUNCTION FROM RTF CODE
    RTF rt_code )               // - code for function
;
