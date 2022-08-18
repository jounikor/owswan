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
;* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
;*               DESCRIBE IT HERE!
;*
;*****************************************************************************


;;
;; COREAD       : i/o co-routines for READ
;;

.386p
.8087

include ptypes.inc
include struct.inc
include mdef.inc

        xrefp   "C",DoRead
        xrefp   RT@SetIOCB

        dataseg

        xrefd   "C",IORslt,        word

        enddata

        modstart coread, byte

        extrn   SwitchToRT      : near
        extrn   IOChar          : near
        extrn   IOStr           : near
        extrn   IOArr           : near
        extrn   IOChArr         : near
        extrn   RdWrCommon      : near


        xdefp   RT@IORead
        defp    RT@IORead
        call    RT@SetIOCB              ; initialize i/o
        mov     EAX,offset DoRead       ; indicate read
        jmp     RdWrCommon              ; start read operation
        endproc RT@IORead


        xdefp   RT@InpLOG1
        defp    RT@InpLOG1              ; input LOGICAL*1 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_LOG_1            ; return LOGICAL*1 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpLOG1


        xdefp   RT@InpLOG4
        defp    RT@InpLOG4              ; input LOGICAL*4 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_LOG_4            ; return LOGICAL*4 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpLOG4


        xdefp   RT@InpINT1
        defp    RT@InpINT1              ; input INTEGER*1 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_INT_1            ; return INTEGER*1 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpINT1


        xdefp   RT@InpINT2
        defp    RT@InpINT2              ; input INTEGER*2 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_INT_2            ; return INTEGER*2 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpINT2


        xdefp   RT@InpINT4
        defp    RT@InpINT4              ; input INTEGER*4 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_INT_4            ; return INTEGER*4 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpINT4


        xdefp   RT@InpREAL
        defp    RT@InpREAL              ; input REAL*4 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_REAL_4           ; return REAL*4 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpREAL


        xdefp   RT@InpDBLE
        defp    RT@InpDBLE              ; input REAL*8 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_REAL_8           ; return REAL*8 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpDBLE


        xdefp   RT@InpXTND
        defp    RT@InpXTND              ; input REAL*16 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_REAL_16          ; return REAL*16 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpXTND


        xdefp   RT@InpCPLX
        defp    RT@InpCPLX              ; input REAL*8 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_CPLX_8           ; return COMPLEX*8 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpCPLX


        xdefp   RT@InpDBCX
        defp    RT@InpDBCX              ; input COMPLEX*16 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_CPLX_16          ; return COMPLEX*16 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpDBCX


        xdefp   RT@InpXTCX
        defp    RT@InpXTCX              ; input COMPLEX*32 value
        mov     dword ptr IORslt,EAX    ; place destination in IORslt
        mov     EAX,PT_CPLX_32          ; return COMPLEX*32 type
        jmp     SwitchToRT              ; return to caller of IOType()
        endproc RT@InpXTCX


        xdefp   RT@InpCHAR
        defp    RT@InpCHAR              ; input CHARACTER*n value
        jmp     IOChar                  ; do character read
        endproc RT@InpCHAR


        xdefp   RT@InpSTR
        defp    RT@InpSTR               ; input CHARACTER*n value
        jmp     IOStr                   ; do character read
        endproc RT@InpSTR


        xdefp   RT@InpArr
        defp    RT@InpArr               ; input array
        jmp     IOArr                   ; do array read
        endproc RT@InpArr


        xdefp   RT@InpChArr
        defp    RT@InpChArr             ; input character array
        jmp     IOChArr                 ; do character array read
        endproc RT@InpChArr

        endmod
        end
