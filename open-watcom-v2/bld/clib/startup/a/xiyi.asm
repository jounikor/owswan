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
;* Description:  Segment definitions for Open Watcom C/C++16.
;*
;*****************************************************************************


        name    sgdef086

include langenv.inc
include xinit.inc

        assume  nothing

DGROUP group XIB,XI,XIE,YIB,YI,YIE

XIB     segment word public 'DATA'
_Start_XI label byte
        public  "C",_Start_XI
XIB     ends

XIE     segment word public 'DATA'
_End_XI label byte
        public  "C",_End_XI
XIE     ends

YIB     segment word public 'DATA'
_Start_YI label byte
        public  "C",_Start_YI
YIB     ends

YIE     segment word public 'DATA'
_End_YI label byte
        public  "C",_End_YI
YIE     ends

        end
