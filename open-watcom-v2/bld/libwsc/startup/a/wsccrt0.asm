;*****************************************************************************
;*
;* Description:  WSC 16-bit startup code.
;*
;*****************************************************************************


include mdef.inc


.186



        name    wsccrt0
 
DGROUP group _NULL,_DATA,CONST,STRINGS,_BSS,_BSS2,STACK

REG_INT_BASE    equ 0B0h
REG_INT_ENABLE  equ 0B2h
REG_INT_STATUS  equ 0B4h
REG_INT_ACK     equ 0B6h
REG_BANK_ROM1   equ 0C3h

POS_METADATA    equ 0xffe6      ; -26 bytes - sync with wscrom.h

        public  "C",__wsc_first_bank
        public  "C",__wsc_data_bank

_BSS    segment word public 'BSS'
        public __begbss
        public __enddata
__enddata   label word far
        ; Order and location of these two MUST not change!
__wsc_first_bank db     1 dup(?)
__wsc_data_bank db      1 dup(?)
__begbss    label word far
_BSS    ends
_BSS2    segment word public 'BSS'
        public __endbss
__endbss label word far
_BSS2    ends

_NULL   segment word public 'BEGDATA'
        public __begnull
__begnull   label word far
        db  0x100 dup(?)
_NULL   ends

_DATA   segment word public 'DATA'
        public __begdata
__begdata   label word far
_DATA   ends

CONST   segment word public 'DATA'
CONST   ends

STRINGS segment word public 'DATA'
STRINGS ends

STACK   segment byte stack 'STACK'
        public  __stack_top
__stack_top     label word far
STACK   ends

_TEXT   segment word public 'CODE'
        public  __begtext
__begtext       label word far
_TEXT   ends

_FAR_DATA segment word public 'FAR_DATA'
        public  __begfardata
__begfardata    label word far
_FAR_DATA ends



STARTUP  segment word public 'BEGCODE'

        public  _cstart_
        public  __romstart
        assume  cs:STARTUP
        assume  ds:DGROUP
        assume  es:DGROUP

        extrn   main_  : proc far

if _MODEL and _BIG_CODE
        public  _big_code_
_big_code_      label   far
else
        public  _small_code_
_small_code_    label   near
endif

_cstart_        proc    far
__romstart      label   far
        ; Following initialization is far from "optimized".. but as a first
        ; priority is to get it working..
        ; All DATA, BSS and STACK segments are in the first 64K..
        ;
        ; AX contains the segment where code and data starts.
        mov     bx,ax

        xor     ax,ax
        ; On reset ES and SS are set to $0000
        ;mov     es,ax
        ;mov     ss,ax

        ; Initialize stack
        mov     sp,OFFSET __stack_top

        ; Clear low RAM.. we do not care if this is WS or WSC..
        ; This also cleas BSS area.
        cld
        mov     di,ax
        mov     cx,0x8000
        rep stosw

        ; initialize data
        mov     ax,0xf000
        mov     ds,ax
        mov     di,OFFSET __begdata
        mov     si,POS_METADATA
        mov     cx,OFFSET __enddata
        sub     cx,di
        jz      nodata
        sub     si,cx
        rep movsb
nodata:
        ; Initialize the first and data bank numbers.
        ; Note that __wsc_first_bank and __wsc_data_bank are the first variables
        ; to be initialized into the BSS are so DI already points at proper location.
        ;mov     bp,OFFSET __wsc_first_bank
        ;mov     WORD PTR [ss:bp],bx
        mov     WORD PTR [es:di],bx

        ; Initialize WSC part of the system.. we are running in ROM0 area..
        ; Some stupid irq handler initializations. Move WS/WSC hw irq
        ; base to start from address 0000:0020
        mov     al,0x08
        out     REG_INT_BASE,al

        ; All uninitialized irqs will halt the system..
        mov     ax,OFFSET halt_system
        mov     cx,8+8
        xor     di,di
        rep stosw

        xor     al,al
        out     REG_INT_ENABLE,al
        not     al
        out     REG_INT_ACK,al

        ; The assumption at this point is that
        ; ROM0 is set to ??
        ; ROM1 is set to the first BANK
        ; ROM2 is set to the last BANK
        ; There's no return from main().
        sti
        jmp     main_

        public  halt_system
halt_system     label   near
        cli
        hlt
forever:
        jmp forever

;

_cstart_ endp

STARTUP   ends
;ROMDATA segment word public 'DGROUP'
;        public  __romend
;        public  __begromdata
;__begromdata    label   far
;__romend        label   far
;ROMDATA ends
;ROMDATAE    segment word public 'DGROUP'
;        public  __endromdata
;__endromdata    label   far
;ROMDATAE    ends

        end     _cstart_
