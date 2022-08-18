; need to resolve this extension business
;_OS=_PLDT
; fixme:
; This file contains translations that should be made in the asm source files.
; They are being done here as a stopgap measure.
;
ifdef __PENPOINT__
    _PENPOINT=1
endif
ifdef __OS2__
    ifdef __386__
        _OS220=1
    endif
endif
ifdef __NT__
    _NT=1
endif
ifdef __NETWARE__
    __NET386__=1
endif
ifdef __REGISTER__
    STACK=0
endif
ifdef __STACK__
    STACK=1
    __3S__=1
endif
;
; date          by              reason
; ====          ==              ======
;                               call to set the cr0 register
;                               split the emu libs out of mathlib
;include struct3.inc
;include struct.inc
; Structured macros for MASM.

        __label = 0
        __depth = 0
        __inner_loop = 0

_guess  macro   name
        __depth = __depth + 1
        __label = __label + 1
        _set    __astk,%__depth,__label,<>
        __label = __label + 1
        _set    __estk,%__depth,__label,<>
        ifnb    <name>
          name = __label
        endif
        endm

_quif   macro   cc,name
        ifnb    <name>
          _set  __elbl,,name,<>
          _j    cc,_l,%__elbl
        else
          _set  __albl,,__astk,%__depth
          _j    cc,_l,%__albl
        endif
        endm

_quit   macro   name
        _quif   ,name
        endm

_admit  macro
        _set    __albl,,__astk,%__depth
        __label = __label + 1
        _set    __astk,%__depth,__label,<>
        _set    __elbl,,__estk,%__depth
        _j      ,_l,%__elbl
        _label  _l,%__albl
        endm

_endguess macro
        _set    __albl,,__astk,%__depth
        _label  _l,%__albl
        _set    __elbl,,__estk,%__depth
        _label  _l,%__elbl
        __depth = __depth - 1
        endm

_loop   macro   name
        _guess  name
        _set    __albl,,__astk,%__depth
        _label  _m,%__albl
        _set    __elbl,,__estk,%__depth
        _label  _m,%__elbl
        _set    __llbl,%__depth,%__inner_loop,<>
        _set    __inner_loop,,%__depth,<>
        endm

_loopif macro   cc,name
        ifnb    <name>
          _set  __elbl,,name,<>
          _j    cc,_m,%__elbl
        else
          _set  __albl,,__astk,%__inner_loop
          _j    cc,_m,%__albl
        endif
        endm

_until  macro   cc
        _set    __albl,,__astk,%__depth
        _jn     cc,_m,%__albl
        _set    __inner_loop,,__llbl,%__depth
        _endguess
        endm

_endloop macro
        _set    __albl,,__astk,%__depth
        _j      ,_m,%__albl
        _set    __inner_loop,,__llbl,%__depth
        _endguess
        endm

_if     macro   cc
        _guess
        _set    __albl,,__astk,%__depth
        _jn     cc,_l,%__albl
        endm

_else   macro
        _admit
        endm

_endif  macro
        _endguess
        endm

_set    macro   base1,ext1,base2,ext2
        base1&ext1 = base2&ext2
        endm

_label  macro   base,ext
base&ext:
        endm

_j      macro   cc,base,ext
        j&cc        base&ext
        endm

_jn     macro   cc,base,ext
        jn&cc       base&ext
        endm

jnna    macro   label
        ja      label
        endm

jnnae   macro   label
        jae     label
        endm

jnnb    macro   label
        jb      label
        endm

jnnbe   macro   label
        jbe     label
        endm

jnnc    macro   label
        jc      label
        endm

jnne    macro   label
        je      label
        endm

jnng    macro   label
        jg      label
        endm

jnnge   macro   label
        jge     label
        endm

jnnl    macro   label
        jl      label
        endm

jnnle   macro   label
        jle     label
        endm

jnno    macro   label
        jo      label
        endm

jnnp    macro   label
        jp      label
        endm

jnns    macro   label
        js      label
        endm

jnnz    macro   label
        jz      label
        endm

jnpe    macro   label
        jpo     label
        endm

jnpo    macro   label
        jpe     label
        endm

j       macro   label
        jmp     short label
        endm

jn      macro   label
        nop
        endm

_shl    macro   reg,count
        add     reg,reg
        endm

_rcl    macro   reg,count
        adc     reg,reg
        endm
;include extender.inc
;
;       386 DOS Extender Equates for __Extender variable
;
X_ERGO          equ     0
X_RATIONAL      equ     1
X_PHARLAP_V2    equ     2
X_PHARLAP_V3    equ     3
X_PHARLAP_V4    equ     4
X_PHARLAP_V5    equ     5
X_INTEL         equ     9

.386p
.387
        name    dosinite

extrn   __int7          : near
extrn   __int7_pl3      : near
extrn   __hook387       : near
extrn   __unhook387     : near

_DATA segment dword public 'DATA'
        extrn   __8087     : byte
        extrn   __8087cw   : word
        extrn   __Extender : byte
        extrn   __X32VM    : byte
        extrn   __D16Infoseg : word
        extrn   __x386_zero_base_selector : word
        extrn   __GDAptr   : dword
sysregs dd      14 dup(0)       ; only need 12, but just in case
oldidt  dd      0,0
old7off dd      0
old7seg dw      0
msw     dw      0
hooked  db      0
_DATA ends

DGROUP  group _DATA

_TEXT segment word public 'CODE'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;      void _sys_init_387_emulator( int )
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

assume   cs:_TEXT,ds:DGROUP

MP      equ     02h
EM      equ     04h
ET      equ     10h

GDA_SERV equ    48      ; offset into GDA to extender service routine addr

;       ebp is not 0 if NO87 environment variable is present

public  __sys_init_387_emulator
__sys_init_387_emulator proc near
        push    es                      ; save some registers
        push    ecx                     ; ...
        push    ebx                     ; ...
        push    edx                     ; ...
        fninit                          ; initialize math coprocessor
        push    eax                     ; get some stack space
        fnstcw  [esp]                   ; store control word in memory
        pop     eax                     ; get CW into ax
        cmp     ah,03h                  ; coprocessor is present
        _if     ne                      ; if no coprocessor
          inc   ebp                     ; - pretend NO87 was set
        _endif                          ; endif
        or      ebp,ebp                 ; if we want emulation
        _if     ne                      ; then
          call  hook_in_emulator        ; - then hook in emulator
        _endif                          ; endif
        finit                           ; initialize the '80387'
        fldcw   word ptr __8087cw       ; load control word
        fldz                            ; put 387 into 4 empty / 4 full state
        fldz                            ; ...
        fldz                            ; ...
        fldz                            ; ...
        pop     edx                     ; restore registers
        pop     ebx                     ; ...
        pop     ecx                     ; ...
        pop     es                      ; ...
        ret                             ; return to caller
__sys_init_387_emulator endp

hook_in_emulator proc near
        mov     byte ptr __8087,3       ; pretend we have a 387
        mov     byte ptr hooked,1       ; indicate we hooked it
        smsw    msw                     ; save the EM and MP bits
        and     word ptr msw,EM or MP   ; ...
        sub     esp,8                   ; allocate temp
        sidt    fword ptr [esp]         ; find out where IDT table is
        mov     ebx,2[esp]              ; ...
        add     ebx,7*8                 ; point to entry for int 7
        add     esp,8                   ; deallocate temp
        _guess                          ; guess: X-32VM
          cmp   byte ptr __X32VM,0      ; - quit if not X-32VM
          _quif e                       ; - ...
          call  create_IDT_entry        ; - create entry for IDT
          mov   es,__x386_zero_base_selector
          mov   es:[ebx],edx            ; - set int 7
          mov   es:4[ebx],ecx           ; - ...
          call  _set_EM_MP_bits         ; - set EM and MP bits
;       _admit                          ; guess: Ergo (OS/386)
          cmp   byte ptr __Extender,X_ERGO ; - quit if not Ergo
          _quif ne                      ; - ...
          mov   ax,2507h                ; - hook int7
          push  ds                      ; - ...
          mov   cx,cs                   ; - ...
          mov   ds,cx                   ; - ...
          lea   edx,__int7              ; - ...
          int   21h                     ; - ...
          pop   ds                      ; - restore ds
          mov   al,EM                   ; - ...
          mov   ah,0f3H                 ; - set EM bit in cr0 (turn off MP)
          int   21h                     ; - ...
        _admit                          ; guess: Intel Code Builder
          cmp   byte ptr __Extender,X_INTEL ; - quit if not Intel
          _quif ne                      ; - ...
          call  create_IDT_entry        ; - create entry for IDT
          mov   [ebx],edx               ; - fill in entry
          mov   4[ebx],ecx              ; - ...
;         mov   ecx,cr0                 ; - get cr0
;         or    ecx,EM                  ; - flip on the EM bit
;         and   ecx,not MP              ; - don't want WAIT instructions
;         mov   cr0,ecx                 ; - store it back
          mov   eax,00000E02h           ; - "set cr0" function
          mov   ebx,EM                  ; - want EM bit turned on
          mov   ecx,__GDAptr            ; - get address of GDA
          call  dword ptr [ecx+GDA_SERV]          ; - call extender service routine
        _admit                          ; guess: Rational DOS/4G
          cmp   byte ptr __Extender,X_RATIONAL   ; - quit if not DOS/4G
          _quif ne                      ; - ...
          mov   dx,__D16Infoseg         ; - get segment address of _d16info
          sub   eax,eax                 ; - set offset to 0
          call  __hook387               ; - hook into int7
        _admit                          ; admit: we're under Phar Lap
          sub   dx,dx                   ; - no info ptr
          sub   eax,eax
          call  __hook387               ; - try windows hookup
          cmp   al,1                    ; - did it work?
          _quif e                       ; - yes, done
          call  hook_pharlap            ; - hook int7 under Phar Lap
        _endguess                       ; endguess
        ret                             ; return
hook_in_emulator endp

create_IDT_entry proc near
        lea     ECX,__int7              ; set new int7 handler
        mov     DX,CS                   ; get CS as our segment selector
        shl     EDX,16                  ; shift to top
        mov     DX,CX                   ; bottom word is low part of addr
        mov     CX,CS                   ; get CS for CPL bits
        and     CL,3                    ; isolate CPL bits
        shl     CX,13                   ; shift into place for DPL
        or      CH,8Eh                  ; set P=1, and INTERRUPT gate
        ret                             ; return
create_IDT_entry endp

hook_pharlap proc near
        _guess                          ; guess: version 4
if 0
          cmp   byte ptr __Extender,X_PHARLAP_V4 ; - chk for v4 or higher
          _quif l                       ; - quit if not version 4 or higher
          mov   BL,7                    ; - interrupt #7
          mov   ECX,0                   ; - read IDT
          lea   EDX,oldidt              ; - point to save area
          mov   AX,253Dh                ; - code to read/write IDT
          int   21h                     ; - read old value
          _quif c                       ; - quit if error
          call  create_IDT_entry        ; - create entry for IDT
          push  ECX                     ; - push new IDT onto stack
          push  EDX                     ; - ...
          mov   EDX,ESP                 ; - point to new IDT
          mov   ECX,1                   ; - write IDT
          mov   AX,253Dh                ; - code to read/write IDT
          int   21h                     ; - write new value
          pop   ECX                     ; - clean up stack
          pop   ECX                     ; - ...
          _quif c                       ; - quit if error
          call  _set_EM_MP_bits         ; - set EM and MP bits
        _admit                          ; guess: version 3
endif
          mov   cl,7                    ; - get old int7 handler
          mov   ax,2502h                ; - ...
          int   21H                     ; - ...
          mov   old7off,ebx             ; - save it
          mov   old7seg,es              ; - ...
          cmp   byte ptr __Extender,X_PHARLAP_V3 ; - chk for v3 or higher
          mov   cl,7                    ; - set new int7 handler
          mov   ax,2504h                ; - ...
          push  ds                      ; - ...
          push  cs                      ; - ...
          pop   ds                      ; - ...
          _quif l                       ; - quit if not version 3 or higher
          lea   edx,__int7_pl3          ; - set new int7 handler
          int   21H                     ; - ...
          pop   ds                      ; - restore ds
          call  _set_EM_MP_bits         ; - set EM and MP bits
        _admit                          ; admit: Phar Lap version <= 2
          lea   edx,__int7              ; - set new int7 handler
          int   21H                     ; - ...
          pop   ds                      ; - restore ds
          mov   ecx,cr0                 ; - get cr0
          or    ecx,EM                  ; - flip on the EM bit
          and   ecx,not MP              ; - don't want WAIT instructions
          mov   cr0,ecx                 ; - store it back
        _endguess                       ; endguess
        ret                             ; return
hook_pharlap endp

_set_EM_MP_bits proc near
        xor     ebx,ebx                 ; read system registers
        lea     edx,sysregs             ; ...
        mov     ax,2535H                ; ...
        int     21h                     ; ...
        or      sysregs,EM              ; flip on the EM bit
        and     sysregs,not MP          ; don't want WAIT instructions
        inc     ebx                     ; write back system registers
        int     21h                     ; ...
        ret                             ; return
_set_EM_MP_bits endp
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;      void _sys_fini_387_emulator( void )
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

public __sys_fini_387_emulator
__sys_fini_387_emulator proc near
        cmp     byte ptr hooked,1       ; if we have not hooked an interrupt
        _if     ne                      ; then
          ret                           ; - return
        _endif                          ; endif
        push    ecx                     ; save some regs
        push    ebx                     ; ...
        push    edx                     ; ...
        mov     al,__Extender           ; get extender setting
        _guess                          ; guess: X-32VM
          cmp   byte ptr __X32VM,0      ; - quit if not X-32VM
          _quif e                       ; - ...
          call  _reset_EM_MP_bits       ; - reset EM and MP bits
        _admit                          ; guess: Intel Code Builder
          cmp   al,X_INTEL              ; - quit if not Intel
          _quif ne                      ; - ...
;         mov   ecx,cr0                 ; - get cr0
;         and   ecx,not (EM or MP)      ; - turn off EM and MP bits
;         mov   dx,msw                  ; - restore to old values
;         or    cx,dx                   ; - ...
;         mov   cr0,ecx                 ; - store it back
          mov   eax,00000E02h           ; - "set cr0" function
          mov   ebx,0                   ; - want EM bit turned off
          mov   ecx,__GDAptr            ; - get address of GDA
          call  dword ptr [ecx+GDA_SERV]          ; - call extender service routine
        _admit                          ; guess: Ergo DOS extender (OS/386)
          cmp   al,X_ERGO               ; - quit if not OS/386
          _quif ne                      ; - ...
          mov   ax,msw                  ; - get old EM and MP bits
          mov   ah,0f3h                 ; - reset EM/MP request
          int   21h                     ; - ...
        _admit                          ; guess: Rational DOS/4G
          cmp   al,X_RATIONAL           ; - quit if not DOS/4G
          _quif ne                      ; - ...
          mov   dx,__D16Infoseg         ; - get segment address of _d16info
          sub   eax,eax                 ; - set offset to 0
          call  __unhook387             ; - unhook from int7
        _admit                          ; admit - we're under Phar Lap
          sub   edx,edx                 ; - null selector
          call  __unhook387             ; - unhook from windows
          cmp   al,1                    ; - did it work?
          _quif e                       ; - yes, done
          call  unhook_pharlap          ; - unhook emulator
        _endguess                       ; endguess
        mov     byte ptr hooked,0       ; indicate not hooked anymore
        pop     edx                     ; restore regs
        pop     ebx                     ; ...
        pop     ecx                     ; ...
        ret                             ; return
__sys_fini_387_emulator endp

unhook_pharlap proc near
        _guess                          ; guess: version 4
if 0
          cmp   byte ptr __Extender,X_PHARLAP_V4 ; - chk for v4 or higher
          _quif l                       ; - quit if not version 4 or higher
          mov   BL,7                    ; - interrupt #7
          mov   ECX,1                   ; - write IDT
          lea   EDX,oldidt              ; - point to save area
          mov   AX,253Dh                ; - code to read/write IDT
          int   21h                     ; - restore old value
          _quif c                       ; - quit if error
          call  _reset_EM_MP_bits       ; - reset EM and MP bits
        _admit                          ; admit: not version 4
endif
          mov   cl,7                    ; - restore int7 handler
          mov   ax,2504h                ; - ...
          mov   edx,old7off             ; - ...
          push  ds                      ; - save ds
          mov   ds,old7seg              ; - ...
          int   21H                     ; - ...
          pop   ds                      ; - restore ds
          cmp   byte ptr __Extender,X_PHARLAP_V3 ; - quit if version < 3
          _quif l                       ; - ...
          call  _reset_EM_MP_bits       ; - reset EM and MP bits
        _admit                          ; admit: Pharlap V2
          mov   ecx,cr0                 ; - get cr0
          and   ecx,not (EM or MP)      ; - turn off EM and MP bits
          mov   dx,msw                  ; - restore to old values
          or    cx,dx                   ; - ...
          mov   cr0,ecx                 ; - store it back
        _endguess                       ; endguess
        ret                             ; return
unhook_pharlap endp

_reset_EM_MP_bits proc near
        xor     ebx,ebx                 ; read system registers
        lea     edx,sysregs             ; ...
        mov     ax,2535H                ; ...
        int     21h                     ; ...
        xor     ecx,ecx                 ; restore EM and MP bits
        mov     cx,msw                  ; ...
        mov     sysregs,ecx             ; ...
        inc     ebx                     ; write back system registers
        int     21h                     ; ...
        ret                             ; return
_reset_EM_MP_bits endp

_TEXT   ends

        end
