;*****************************************************************************
;*
;*                            Open Watcom Project
;*
;* Copyright (c) 2002-2017 The Open Watcom Contributors. All Rights Reserved.
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
;* Description:  OS/2 16-bit executable and DLL startup code.
;*
;*****************************************************************************


;       This must be assembled using one of the following commands:
;               wasm cstrto16 -bt=OS2 -ms -2r
;               wasm cstrto16 -bt=OS2 -mm -2r
;               wasm cstrto16 -bt=OS2 -mc -2r
;               wasm cstrto16 -bt=OS2 -ml -2r
;               wasm cstrto16 -bt=OS2 -mh -2r
;
; NOTE: This code must NEVER NEVER NEVER NEVER define any variable or routines
;       which are needed by the C Library. It should also be MINIMAL.
;       This is so we can make CLIB DLLs.
;
include langenv.inc
include mdef.inc
include xinit.inc

        name    cstart

        assume  nothing


        xrefp   __OS2Main
        extrn   __DOSseg__      : byte


DGROUP  group _NULL,_AFTERNULL,CONST,STRINGS,_DATA,XIB,XI,XIE,YIB,YI,YIE,_BSS,STACK

ife _MODEL and _BIG_CODE

; this guarantees that no function pointer will equal NULL
; (WLINK will keep segment 'BEGTEXT' in front)
; This segment must be at least 4 bytes in size to avoid confusing the
; signal function.
; need a symbol defined here to prevent the dead code elimination from
; eliminating the segment.
; (the int 3h is useful for quickly revealing jumps to NULL code pointers)

BEGTEXT  segment word public 'CODE'
        assume  cs:BEGTEXT
forever label   near
        int     3h
        jmp short forever
        public ___begtext
___begtext label byte
        nop
        nop
        nop
        nop
        assume  cs:nothing
BEGTEXT  ends

endif

_TEXT   segment para public 'CODE'

        assume  ds:DGROUP

        INIT_VAL        equ 0101h
        NUM_VAL         equ 16

_NULL   segment para public 'BEGDATA'
public  __nullarea
__nullarea  label   word
        dw      NUM_VAL dup(INIT_VAL)
_NULL   ends

_AFTERNULL segment word public 'BEGDATA'
        dw      0                       ; nullchar for string at address 0
_AFTERNULL ends

CONST   segment word public 'DATA'
CONST   ends

STRINGS segment word public 'DATA'
STRINGS ends

_DATA   segment word public 'DATA'
_DATA   ends

_BSS    segment word public 'BSS'
_BSS    ends

STACK   segment para stack 'STACK'
        stklow  label   word
        db      1000h dup(?)
STACK   ends

        assume  nothing
        public  _cstart_
        public  __DLLstart_

        assume  cs:_TEXT

_cstart_ proc far
__DLLstart_ proc far
        assume  ds:DGROUP

        jmp short around

ife _MODEL and _BIG_CODE
        dw      ___begtext              ; make sure dead code elimination
                                        ; doesn't kill BEGTEXT
endif

around:
        push    bx                      ; save registers
        push    cx                      ; ...
        push    dx                      ; ...
        push    si                      ; ...
        push    di                      ; ...
        push    es                      ; ...
        push    bp                      ; ...
        mov     bp,sp                   ; point to saved registers
        push    bx                      ; command offset
        push    ax                      ; environment segment
        mov     cx,ss                   ; ...
        mov     bx,sp                   ; get bottom of stack in cx:dx
        add     bx,4                    ; adjust for two pushes
        mov     dx,ss                   ; get stklow dx:ax
        mov     ax,offset DGROUP:stklow ; ...
        call    __OS2Main               ; call main startup
        mov     sp,bp                   ; restore stack pointer
        pop     bp                      ; restore registers
        pop     es                      ; ...
        pop     di                      ; ...
        pop     si                      ; ...
        pop     dx                      ; ...
        pop     cx                      ; ...
        pop     bx                      ; ...
        ret                             ; return

__DLLstart_ endp
_cstart_ endp

;
; copyright message
;
include msgcpyrt.inc

_TEXT   ends

        end     _cstart_
