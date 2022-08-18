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
;* Description:  32-bit chain interrupt handler function.
;*
;*****************************************************************************


        include mdef.inc
        include struct.inc

        name    chainint

_TEXT   segment word public 'CODE'
_TEXT   ends


_TEXT   segment


_chain_intr proc far
        public  "C",_chain_intr
        ; never return to the caller
        ; doesn't have return address on the stack
ifdef __STACK__
        mov     ecx,0[esp]              ; get offset
        mov     eax,4[esp]              ; get segment
else
        mov     ecx,eax                 ; get offset
        mov     eax,edx                 ; get segment
endif
        mov     esp,ebp                 ; reset SP to point to saved registers
        xchg    ecx,40[ebp]             ; restore ecx, & put in offset
        xchg    eax,44[ebp]             ; restore eax, & put in segment
        mov     ebx,56[ebp]             ; restore flags
        and     ebx,0FFFFFCFFh          ; except for IF and TF
        push    ebx                     ; :
        popfd                           ; :
        pop     gs                      ; restore segment registers
        pop     fs                      ;
        pop     es                      ;
        pop     ds                      ;
        pop     edi                     ; . . .
        pop     esi                     ; . . .
        pop     ebp                     ; . . .
        pop     ebx                     ; restore registers
        pop     ebx                     ; . . .
        pop     edx                     ; . . .
        ret                             ; return to previous interrupt handler
_chain_intr     endp

_TEXT   ends
        end
