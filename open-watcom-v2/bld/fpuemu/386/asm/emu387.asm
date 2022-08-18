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
;* Description:  387 emulator top level source file.
;*               
;*
;*****************************************************************************


.386p
        name    emu387

_PLDT = 1 ;     _Phar Lap Development Tools
_NW   = 2 ;     Netware 386
_QNX  = 3 ;     QNX/386

ifdef __WIN387__

        include win30vxd.inc

VxD_LOCKED_CODE_SEG

else
_DATA segment dword public 'DATA'
_DATA ends
DGROUP  group   _DATA
        assume  ds:DGROUP

_TEXT   segment dword public 'CODE'
        assume  cs:_TEXT

endproc         macro   dsym
         dsym   endp
                endm

endif

modstart        macro   modname
                endm

xdefp           macro   xsym
                ifndef NDEBUG
                public  xsym
                endif
                endm

defp            macro   dsym
         dsym   proc    near
                endm

xrefp           macro   dsym
                endm

ifdef __WIN387__
startdata       macro
VxD_LOCKED_DATA_SEG
                endm
enddata         macro
VxD_LOCKED_DATA_ENDS
                endm
endmod          macro
VxD_LOCKED_CODE_ENDS
                endm
else
endmod          macro
_TEXT           ends
                endm
startdata       macro
_DATA   segment dword public 'DATA'
                endm
enddata         macro
_DATA   ends
                endm
endif

include struct.inc
include fstatus.inc
include fpconsts.inc
include 386fpemu.inc
include flda386.asm
include fldc386.asm
include fldd386.asm
include fldm386.asm
include ldi4386.asm
include ldi8386.asm
include i4ld386.asm
include i8ld386.asm
include fdld386.asm
include ldfs386.asm
include ldfd386.asm
include fsld386.asm
include 386round.inc
include 386atan.inc
include 386fprem.inc
include 386fxam.inc
include 386log.inc
include 386sind.inc
include 386trig.inc
include 386f2xm1.inc
include sqrt386.asm
include 386poly.inc

        endmod
        end
