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
;* Description:  convert 4-byte integer into long double
;*
;*****************************************************************************


ifdef _BUILDING_MATHLIB

include mdef.inc
include struct.inc

        modstart    i4ld086, word

endif

        xrefp   __dw_normalize

        xdefp   __I4LD
        xdefp   __U4LD

;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
;  
;   __I4LD, __U4LD - convert 4-byte integer into long double
;  
;       input:  DX:AX - long
;ifdef _BUILDING_MATHLIB
;               SS:BX - pointer to long double result
;else
;               DS:BX - pointer to long double result
;endif
;  
;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

;       __I4LD - convert long into long double
;       __U4LD - convert unsigned long into long double

        defp    __I4LD

        _guess xx1              ; guess
          or      DX,DX         ; - if number is negative
          _if     s             ; - then
            not   DX            ; - - negate the value
            neg   AX            ; - - ...
            sbb   DX,-1         ; - - ...
            push  CX            ; - - save CX
            mov   CX,0C01Eh     ; - - set exponent
          _else                 ; - else

        defp    __U4LD          ; - convert unsigned long to long double

            push  CX            ; - - save CX
            mov   CX,DX         ; - - check for zero value
            or    CX,AX         ; - - ...
        _quif z,xx1             ; quit if zero
            mov   CX,0401Eh     ; - - set exponent
          _endif                ; - endif
          call __dw_normalize   ; - normalize 32-bit fraction
        _endguess               ; endguess
ifdef _BUILDING_MATHLIB
        push    DS              ; save DS
        push    SS              ; fpc code assumes parms are relative to SS
        pop     DS              ; ...
endif
        mov     8[BX],CX        ; store exponent
        mov     6[BX],DX        ; fraction
        mov     4[BX],AX        ; ...
        sub     AX,AX           ; rest is 0
        mov     2[BX],AX        ; ...
        mov     [BX],AX         ; ...
ifdef _BUILDING_MATHLIB
        pop     DS              ; restore DS
endif
        pop     CX              ; restore CX
        ret                     ; return

        endproc __U4LD
        endproc __I4LD


ifdef _BUILDING_MATHLIB

        endmod

        endf    equ end
else
        endf    equ <>

endif

endf
