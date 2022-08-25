#ifndef _INLINES_H_INCLUDED
#define _INLINES_H_INCLUDED

#pragma aux  SET_BANK_ROM0 = 	\
	"out 0xc2,al"				\
	__parm   [al];

#pragma aux  SET_BANK_ROM1 = 	\
	"out 0xc3,al"				\
	__parm   [al];

#pragma aux  SET_BANK_ROM2 = 	\
	"out 0xc0,al"				\
	__parm   [al];

#pragma aux  SET_BANK_SRAM = 	\
	"out 0xc1,al"				\
	__parm   [al];

#pragma aux  OUTB = 	\
	"out dx,al"			\
	__parm   [dx] [al];

#pragma aux  OUTW = 	\
	"out dx,ax"			\
	__parm   [dx] [ax];

#pragma aux  INB = 		\
	"in al,dx"			\
	__parm   [dx]		\
	__value  [al];

#pragma aux  INW = 		\
	"in ax,dx"			\
	__parm   [dx]		\
	__value  [ax];

#pragma aux enable_irq = \
    "sti";

#pragma aux disable_irq = \
    "cli";

#pragma aux ack_hw_irq  = \
    "mov al,0x1"        \
    "shl al,cl"			\
    "out 0xb6,al"       \
    __modify [al] 	  	\
    __parm [cl];
                                                                                                                   
#pragma aux ack_hw_irq_mask = \
    "out 0xb6,al"    \
    __parm [al];






#pragma aux memcpy =        \
    "xor    ax,ax"          \
    "mov    es,ax"          \
    "rep movsb"             \
    parm [di][ds si][cx]    \
    modify [es ax];

#endif /* _INLINES_H_INCLUDED */
