;*****************************************************************************
;*
;*                            Open Watcom Project
;*
;* Copyright (c) 2002-2022 The Open Watcom Contributors. All Rights Reserved.
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


include mdef.inc
include struct.inc

        modstart int386xa

        xdefp   __int386x_
        xdefp   _DoINTR_

;struct REGS {
;        unsigned int    eax, ebx, ecx, edx, esi, edi;
;        unsigned int    cflag;
;};

RS_EAX  equ     0
RS_EBX  equ     4
RS_ECX  equ     8
RS_EDX  equ     12
RS_ESI  equ     16
RS_EDI  equ     20
RS_F    equ     24

;struct SREGS {
;        unsigned short es, cs, ss, ds, fs, gs;
;};

SS_ES  equ     0
SS_CS  equ     2
SS_SS  equ     4
SS_DS  equ     6
SS_FS  equ     8
SS_GS  equ     10

;
;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
;<>
;<>  int __int386x( unsigned char intno, union REGS *inregs, union REGS *outregs, struct SREGS *segregs )
;<>     parm caller [eax] [edi] [edx] [ebx]
;<>
;<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
        defp    __int386x_

        pushfd                          ; save CPU flags
        push    ebp                     ; save regs
        push    es                      ; ...
        push    ds                      ; ...
        push    edx                     ; save EDX (outregs)
        push    ebx                     ; save EBX (segregs)
        xor     edx,edx                 ; set interrupt flags to 0
        call    dointerrupt             ; load registers and invoke interrupt handler
        push    ds                      ; save new DS
        push    edx                     ; save new EDX
        mov     ebp,esp                 ; get access to stack
        mov     ds,word ptr (8+8)[ebp]  ; restore DS
        mov     edx,(8+4)[ebp]          ; get address of outregs struct
        mov     RS_EAX[edx],eax         ; update registers
        mov     RS_EBX[edx],ebx         ; ...
        mov     RS_ECX[edx],ecx         ; ...
        pop     RS_EDX[edx]             ; ...
        mov     RS_ESI[edx],esi         ; ...
        mov     RS_EDI[edx],edi         ; ...
        sbb     eax,eax                 ; calc. value of carry flag
        and     eax,1                   ; ...
        mov     RS_F[edx],eax           ; save carry flag status
        pop     eax                     ; get DS for update
        pop     ebx                     ; restore EBX address of segregs
        mov     word ptr SS_ES[ebx],es  ; update regs
        mov     word ptr SS_DS[ebx],ax  ; ...
        pop     edx                     ; restore regs
        pop     ds                      ; ...
        pop     es                      ; ...
        pop     ebp                     ; ...
        popfd                           ; restore CPU flags
        ret                             ; return
        endproc __int386x_

        defp    dointerrupt
        lea     eax,[eax+eax*2]         ; calc interrupt # times 3
        lea     eax,inttable[eax]       ; calc address of "int nn" instruction
        push    eax                     ; push address of "int nn" instruction
        mov     ah,dl                   ; load flags
        sahf                            ; ...
        mov     es,word ptr SS_ES[ebx]  ; load regs
        mov     bp,word ptr SS_DS[ebx]  ; ...
        mov     eax,RS_EAX[edi]         ; ...
        mov     ebx,RS_EBX[edi]         ; ...
        mov     ecx,RS_ECX[edi]         ; ...
        mov     edx,RS_EDX[edi]         ; ...
        mov     esi,RS_ESI[edi]         ; ...
        mov     edi,RS_EDI[edi]         ; ...
        mov     ds,bp                   ; ...
        ret                             ; return to "int nn" instruction

;struct REGPACKX {
;        unsigned int   eax, ebx, ecx, edx, ebp, esi, edi;
;        unsigned short ds, es, fs, gs;
;        unsigned int   flags;
;};

RP_EAX  equ     0
RP_EBX  equ     4
RP_ECX  equ     8
RP_EDX  equ     12
RP_EBP  equ     16
RP_ESI  equ     20
RP_EDI  equ     24
RP_DS   equ     28
RP_ES   equ     30
RP_FS   equ     32
RP_GS   equ     34
RP_F    equ     36

;void _DoINTR( unsigned char intno, struct REGPACK *regs, unsigned char flags )
;/************ EAX **************** EBX ***************** EDX ****************/

        defp    _DoINTR_
        pushfd                          ; save CPU flags
        pushad                          ; save regs
        push    gs                      ; ...
        push    fs                      ; ...
        push    es                      ; ...
        push    ds                      ; ...
        push    ebx                     ; ... (pointer to REGPACK)
        call    dointr386               ; load registers and invoke interrupt handler
        push    ds                      ; save regs
        push    ebp                     ; ...
        push    ebx                     ; ...
        mov     ebp,esp                 ; get access to stack
        mov     ds,word ptr (12+4)[ebp] ; restore DS
        mov     ebx,(12+0)[ebp]         ; restore EBX address of REGPACK
        mov     RP_EAX[ebx],eax         ; update registers
        pop     RP_EBX[ebx]             ; ...
        mov     RP_ECX[ebx],ecx         ; ...
        mov     RP_EDX[ebx],edx         ; ...
        pop     RP_EBP[ebx]             ; ...
        mov     RP_ESI[ebx],esi         ; ...
        mov     RP_EDI[ebx],edi         ; ...
        pop     eax                     ; ...
        mov     word ptr RP_DS[ebx],ax  ; ...
        mov     word ptr RP_ES[ebx],es  ; ...
        mov     word ptr RP_FS[ebx],fs  ; ...
        mov     word ptr RP_GS[ebx],gs  ; ...
        pushfd                          ; ...
        pop     RP_F[ebx]               ; ...
        add     esp,4+4                 ; restore regs
        pop     es                      ; ...
        pop     fs                      ; ...
        pop     gs                      ; ...
        popad                           ; ...
        popfd                           ; restore CPU flags
        ret
        endproc _DoINTR_

        defp    dointr386
        lea     eax,[eax+eax*2]         ; calc interrupt # times 3
        lea     eax,inttable[eax]       ; calc address of "int nn" instruction
        push    eax                     ; push address of "int nn" instruction
        mov     ah,dl                   ; load flags
        sahf                            ; ...
        mov     eax,RP_EAX[ebx]         ; load regs
        mov     ecx,RP_ECX[ebx]         ; ...
        mov     edx,RP_EDX[ebx]         ; ...
        mov     ebp,RP_EBP[ebx]         ; ...
        mov     esi,RP_ESI[ebx]         ; ...
        mov     edi,RP_EDI[ebx]         ; ...
        mov     es,word ptr RP_DS[ebx]  ; ...
        push    es                      ; ...
        mov     es,word ptr RP_ES[ebx]  ; ...
        mov     fs,word ptr RP_FS[ebx]  ; ...
        mov     gs,word ptr RP_GS[ebx]  ; ...
        mov     ebx,RP_EBX[ebx]         ; ...
        pop     ds                      ; ...
        ret                             ; return to "int nn" instruction
dointr386       endp

inttable:
        int     0
        ret
        int     1
        ret
        int     2
        ret
        int     3
        nop
        ret
        int     4
        ret
        int     5
        ret
        int     6
        ret
        int     7
        ret
        int     8
        ret
        int     9
        ret
        int     10
        ret
        int     11
        ret
        int     12
        ret
        int     13
        ret
        int     14
        ret
        int     15
        ret
        int     16
        ret
        int     17
        ret
        int     18
        ret
        int     19
        ret
        int     20
        ret
        int     21
        ret
        int     22
        ret
        int     23
        ret
        int     24
        ret
        int     25
        ret
        int     26
        ret
        int     27
        ret
        int     28
        ret
        int     29
        ret
        int     30
        ret
        int     31
        ret
        int     32
        ret
        int     33
        ret
        int     34
        ret
        int     35
        ret
        int     36
        ret
        int     37
        ret
        int     38
        ret
        int     39
        ret
        int     40
        ret
        int     41
        ret
        int     42
        ret
        int     43
        ret
        int     44
        ret
        int     45
        ret
        int     46
        ret
        int     47
        ret
        int     48
        ret
        int     49
        ret
        int     50
        ret
        int     51
        ret
        int     52
        ret
        int     53
        ret
        int     54
        ret
        int     55
        ret
        int     56
        ret
        int     57
        ret
        int     58
        ret
        int     59
        ret
        int     60
        ret
        int     61
        ret
        int     62
        ret
        int     63
        ret
        int     64
        ret
        int     65
        ret
        int     66
        ret
        int     67
        ret
        int     68
        ret
        int     69
        ret
        int     70
        ret
        int     71
        ret
        int     72
        ret
        int     73
        ret
        int     74
        ret
        int     75
        ret
        int     76
        ret
        int     77
        ret
        int     78
        ret
        int     79
        ret
        int     80
        ret
        int     81
        ret
        int     82
        ret
        int     83
        ret
        int     84
        ret
        int     85
        ret
        int     86
        ret
        int     87
        ret
        int     88
        ret
        int     89
        ret
        int     90
        ret
        int     91
        ret
        int     92
        ret
        int     93
        ret
        int     94
        ret
        int     95
        ret
        int     96
        ret
        int     97
        ret
        int     98
        ret
        int     99
        ret
        int     100
        ret
        int     101
        ret
        int     102
        ret
        int     103
        ret
        int     104
        ret
        int     105
        ret
        int     106
        ret
        int     107
        ret
        int     108
        ret
        int     109
        ret
        int     110
        ret
        int     111
        ret
        int     112
        ret
        int     113
        ret
        int     114
        ret
        int     115
        ret
        int     116
        ret
        int     117
        ret
        int     118
        ret
        int     119
        ret
        int     120
        ret
        int     121
        ret
        int     122
        ret
        int     123
        ret
        int     124
        ret
        int     125
        ret
        int     126
        ret
        int     127
        ret
        int     128
        ret
        int     129
        ret
        int     130
        ret
        int     131
        ret
        int     132
        ret
        int     133
        ret
        int     134
        ret
        int     135
        ret
        int     136
        ret
        int     137
        ret
        int     138
        ret
        int     139
        ret
        int     140
        ret
        int     141
        ret
        int     142
        ret
        int     143
        ret
        int     144
        ret
        int     145
        ret
        int     146
        ret
        int     147
        ret
        int     148
        ret
        int     149
        ret
        int     150
        ret
        int     151
        ret
        int     152
        ret
        int     153
        ret
        int     154
        ret
        int     155
        ret
        int     156
        ret
        int     157
        ret
        int     158
        ret
        int     159
        ret
        int     160
        ret
        int     161
        ret
        int     162
        ret
        int     163
        ret
        int     164
        ret
        int     165
        ret
        int     166
        ret
        int     167
        ret
        int     168
        ret
        int     169
        ret
        int     170
        ret
        int     171
        ret
        int     172
        ret
        int     173
        ret
        int     174
        ret
        int     175
        ret
        int     176
        ret
        int     177
        ret
        int     178
        ret
        int     179
        ret
        int     180
        ret
        int     181
        ret
        int     182
        ret
        int     183
        ret
        int     184
        ret
        int     185
        ret
        int     186
        ret
        int     187
        ret
        int     188
        ret
        int     189
        ret
        int     190
        ret
        int     191
        ret
        int     192
        ret
        int     193
        ret
        int     194
        ret
        int     195
        ret
        int     196
        ret
        int     197
        ret
        int     198
        ret
        int     199
        ret
        int     200
        ret
        int     201
        ret
        int     202
        ret
        int     203
        ret
        int     204
        ret
        int     205
        ret
        int     206
        ret
        int     207
        ret
        int     208
        ret
        int     209
        ret
        int     210
        ret
        int     211
        ret
        int     212
        ret
        int     213
        ret
        int     214
        ret
        int     215
        ret
        int     216
        ret
        int     217
        ret
        int     218
        ret
        int     219
        ret
        int     220
        ret
        int     221
        ret
        int     222
        ret
        int     223
        ret
        int     224
        ret
        int     225
        ret
        int     226
        ret
        int     227
        ret
        int     228
        ret
        int     229
        ret
        int     230
        ret
        int     231
        ret
        int     232
        ret
        int     233
        ret
        int     234
        ret
        int     235
        ret
        int     236
        ret
        int     237
        ret
        int     238
        ret
        int     239
        ret
        int     240
        ret
        int     241
        ret
        int     242
        ret
        int     243
        ret
        int     244
        ret
        int     245
        ret
        int     246
        ret
        int     247
        ret
        int     248
        ret
        int     249
        ret
        int     250
        ret
        int     251
        ret
        int     252
        ret
        int     253
        ret
        int     254
        ret
        int     255
        ret
        endproc dointerrupt

        endmod
        end
