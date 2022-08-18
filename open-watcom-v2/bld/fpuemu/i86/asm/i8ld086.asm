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
;* Description:  convert 8-byte integer into long double
;*
;*****************************************************************************


ifdef _BUILDING_MATHLIB

include mdef.inc
include struct.inc

        modstart    i8ld086, word

endif

        xrefp   __qw_normalize

        xdefp   __I8LD
        xdefp   __U8LD

;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
;  
;   __I8LD, __U8LD - convert 8-byte integer into long double
;  
;ifdef _BUILDING_MATHLIB
;       input:  SS:AX - pointer to 8-byte integer
;               SS:DX - pointer to long double result
;else
;       input:  AX:BX:CX:DX - 8-byte integer
;               DS:SI       - pointer to long double result
;endif
;  
;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>

;       __I8LD - convert 8-byte integer into long double
;       __U8LD - convert unsigned 8-byte integer into long double

        defp    __U8LD

        push    DI              ; save DI
        xor     DI,DI           ; unsigned value
        jmp     short cont1

        defp    __I8LD

        push    DI              ; save DI
        mov     DI,8000h        ; signed value
cont1:
ifdef _BUILDING_MATHLIB
        push    BP
        push    CX
        push    BX
        push    AX
        push    DX
        mov     BP,AX
        mov     AX,6[BP]        ; get 8-byte integer
        mov     BX,4[BP]        ; ...
        mov     CX,2[BP]        ; ...
        mov     DX,[BP]         ; ...
endif
        _guess xx1              ; guess
          test  AX,DI           ; - check signed negative value
          _if nz                ; - if negative value
            not AX              ; - - negate the value
            not BX              ; - - ...
            not CX              ; - - ...
            neg DX              ; - - ...
            sbb CX,-1           ; - - ...
            sbb BX,-1           ; - - ...
            sbb AX,-1           ; - - ...
            mov DI,0C03Eh       ; - - get exponent and sign
          _else                 ; - else
            mov DI,AX           ; - - check zero value
            or  DI,BX           ; - - ...
            or  DI,CX           ; - - ...
            or  DI,DX           ; - - ...
        _quif z,xx1             ; quit if zero
            mov DI,403Eh        ; - - get exponent and sign
          _endif                ; - endif
          call __qw_normalize   ; - normalize fraction
        _endguess               ; endguess
ifdef _BUILDING_MATHLIB
        pop     BP
        push    BP
        mov     8[BP],DI        ; store exponent
        mov     6[BP],AX        ; fraction
        mov     4[BP],BX        ; ...
        mov     2[BP],CX        ; ...
        mov     [BP],DX         ; ...
        pop     DX
        pop     AX
        pop     BX
        pop     CX
        pop     BP
else
        mov     8[SI],DI        ; store exponent
        mov     6[SI],AX        ; fraction
        mov     4[SI],BX        ; ...
        mov     2[SI],CX        ; ...
        mov     [SI],DX         ; ...
endif
        pop     DI              ; restore DI
        ret                     ; return

        endproc __I8LD
        endproc __U8LD


ifdef _BUILDING_MATHLIB

        endmod

        endf    equ end
else
        endf    equ <>

endif

endf
