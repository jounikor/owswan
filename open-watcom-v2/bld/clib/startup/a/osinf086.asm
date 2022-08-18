;*****************************************************************************
;*
;*                            Open Watcom Project
;*
;*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
;*
;*  ========================================================================
;*
;*    This file contains Original Code and/or Modifications of Original
;*    Code as defined in and that are subject to the Sybase Open Watcom
;*    Public License version 1.0 (the 'License'). You may not use this file
;*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
;*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
;*    provided with the Original Code and Modifications, and is also
;*    available at www.sybase.com/developer/opensource.
;*
;*    The Original Code and all software distributed under the License are
;*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
;*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
;*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
;*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
;*    NON-INFRINGEMENT. Please see the License for the specific language
;*    governing rights and limitations under the License.
;*
;*  ========================================================================
;*
;* Description:  OS information structure for 16-bit QNX.
;*
;*****************************************************************************


include langenv.inc
include mdef.inc

        xrefp   "C",__DetOSInfo

        name    osinf

_BSS   segment word public 'BSS'
        public  "C",__87
        public  "C",__r87
        public  "C",_osmajor
        public  "C",_osminor
        public  "C",_HShift

__87            db      ?
__r87           db      ?
_osmajor        db      ?
_osminor        db      ?
_HShift         db      ?
_BSS   ends

include xinit.inc
        ; Has to get run *before* 80(x)87 initialization
        xinit   __DetOSInfo,1

        end
