#ifndef _LIBSWC_H_INCLUDED
#define _LIBSWC_H_INCLUDED
#pragma once

/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#include <stdint.h>
#include <stdbool.h>
#include "inlines.h"

/* These consume BSS space and are setup by the crt0 */
extern uint8_t __wsc_data_bank;
extern uint8_t __wsc_first_bank;

/* Inline ASM helpers for ROM and SRAM bank switching */
extern void SET_BANK_ROM0(uint8_t __bank);
extern void SET_BANK_ROM1(uint8_t __bank);
extern void SET_BANK_ROM2(uint8_t __bank);
extern void SET_BANK_SRAM(uint8_t __bank);

/* Calculate the ROM Bank relative to start of additional data bank segments */
#define GET_DATA_BANK(n) ((n+__wsc_data_bank) & 0xff)

/* Other helper inline ASM functions */
extern void OUTB(uint8_t __port, uint8_t __value);
extern void OUTW(uint8_t __port, uint16_t __value);
extern uint8_t INB(uint8_t __port);
extern uint16_t INW(uint8_t __port);
extern void enable_irq(void);
extern void disable_irq(void);
extern void ack_hw_irq(uint8_t irq);
extern void ack_hw_irq_mask(uint8_t irq);

/* IO Map defines.. stolen from http://daifukkat.su/docs/wsman/ */
#define REG_DISP_CTRL			0x00
#define REG_BACK_COLOR		0x01
#define REG_LINE_CUR			0x02
#define REG_LINE_CMP			0x03
#define REG_SPR_BASE			0x04
#define REG_SPR_FIRST			0x05
#define REG_SPR_COUNT			0x06
#define REG_MAP_BASE			0x07
#define REG_SCR2_WIN_X0		0x08
#define REG_SCR2_WIN_Y0		0x09
#define REG_SCR2_WIN_X1		0x0a
#define REG_SCR2_WIN_Y1		0x0b
#define REG_SPR_WIN_X0		0x0c
#define REG_SPR_WIN_Y0		0x0d
#define REG_SPR_WIN_X1		0x0e
#define REG_SPR_WIN_Y1		0x0f
#define REG_SCR1_X			0x10
#define REG_SCR1_Y			0x11
#define REG_SCR2_X			0x12
#define REG_SCR2_Y			0x13
#define REG_LCD_CTRL			0x14
#define REG_LCD_ICON			0x15
#define REG_LCD_VTOTAL		0x16
#define REG_LCD_VSYNC			0x17
#define REG_PALMONO_POOL_0	0x1C
#define REG_PALMONO_POOL_1	0x1d
#define REG_PALMONO_POOL_2	0x1e
#define REG_PALMONO_POOL_3	0x1f
#define REG_PALMONO_0_LOW		0x20
#define REG_PALMONO_0_HGH		0x21
#define REG_PALMONO_1_LOW		0x22
#define REG_PALMONO_1_HGH		0x23
#define REG_PALMONO_2_LOW		0x24
#define REG_PALMONO_2_HGH		0x25
#define REG_PALMONO_3_LOW		0x26
#define REG_PALMONO_3_HGH		0x27
#define REG_PALMONO_4_LOW		0x28
#define REG_PALMONO_4_HGH		0x29
#define REG_PALMONO_5_LOW		0x2a
#define REG_PALMONO_5_HGH		0x2b
#define REG_PALMONO_6_LOW		0x2c
#define REG_PALMONO_6_HGH		0x2d
#define REG_PALMONO_7_LOW		0x2e
#define REG_PALMONO_7_HGH		0x2f
#define REG_PALMONO_8_LOW		0x30
#define REG_PALMONO_8_HGH		0x31
#define REG_PALMONO_9_LOW		0x32
#define REG_PALMONO_9_HGH		0x33
#define REG_PALMONO_A_LOW		0x34
#define REG_PALMONO_A_HGH		0x35
#define REG_PALMONO_B_LOW		0x36
#define REG_PALMONO_B_HGH		0x37
#define REG_PALMONO_C_LOW		0x38
#define REG_PALMONO_C_HGH		0x39
#define REG_PALMONO_D_LOW		0x3a
#define REG_PALMONO_D_HGH		0x3b
#define REG_PALMONO_E_LOW		0x3c
#define REG_PALMONO_E_HGH		0x3d
#define REG_PALMONO_F_LOW		0x3e
#define REG_PALMONO_F_HGH		0x3f
#define REG_DMA_SRC_LOW		0x40
#define REG_DMA_SRC_MID		0x41
#define REG_DMA_SRC_HGH		0x42

#define REG_DMA_DST_LOW     0x44
#define REG_DMA_DST_MID		0x45
#define REG_DMA_LEN_LOW		0x46
#define REG_DMA_LEN_HGH		0x47
#define REG_DMA_CTRL			0x48
#define REG_SDMA_SRC_LOW		0x4a
#define REG_SDMA_SRC_MID		0x4b
#define REG_SDMA_SRC_HGH		0x4c
#define REG_SDMA_LEN_LOW		0x4e
#define REG_SDMA_LEN_MID		0x4r
#define REG_SDMA_LEN_HGH		0x50
#define REG_SDMA_CTRL			0x52
#define REG_DISP_MODE			0x60
#define REG_WSC_SYSTEM		0x62
#define REG_HYPER_CTRL		0x6a
#define REG_HYPER_CHAN_CTRL	0x6b
#define REG_SND_CH1_PITCH_LOW	0x80
#define REG_SND_CH1_PITCH_HGH	0x81
#define REG_SND_CH2_PITCH_LOW	0x82
#define REG_SND_CH2_PITCH_HGH	0x83
#define REG_SND_CH3_PITCH_LOW	0x84
#define REG_SND_CH3_PITCH_HGH	0x85
#define REG_SND_CH4_PITCH_LOW	0x86
#define REG_SND_CH4_PITCH_HGH	0x87
#define REG_SND_CH1_VOL		0x88
#define REG_SND_CH2_VOL		0x89
#define REG_SND_CH3_VOL		0x8a
#define REG_SND_CH4_VOL		0x8b
#define REG_SND_SWEEP_VALUE	0x8c
#define REG_SND_SWEEP_TIME	0x8d
#define REG_SND_NOISE			0x8e
#define REG_SND_WAVE_BASE		0x8f
#define REG_SND_CTRL			0x90
#define REG_SND_OUTPUT		0x91
#define REG_SND_RANDOM_LOW	0x92
#define REG_SND_RANDOM_HGH	0x93
#define REG_SND_VOICE_CTRL	0x94
#define REG_SND_HYPERVOICE	0x95
#define REG_SND_9697_LOW		0x96
#define REG_SND_9697_HGH		0x97
#define REG_SND_9899_LOW		0x98
#define REG_SND_9899_HGH		0x99
#define REG_SND_9A			0x9a
#define REG_SND_9B			0x9b
#define REG_SND_9C			0x9c
#define REG_SND_9D			0x9d
#define REG_SND_9E			0x9e
#define REG_HW_FLAGS			0xa0
#define REG_TMR_CTRL			0xa2
#define REG_HTMR_FREQ_LOW		0xa4
#define REG_HTMR_FREQ_HGH		0xa5
#define REG_VTMR_FREQ_LOW		0xa6
#define REG_VTMR_FREQ_HGH		0xa7
#define REG_HTMR_CTR_LOW		0xa8
#define REG_HTMR_CTR_HGH		0xa9
#define REG_VTMR_CTR_LOW		0xaa
#define REG_VTMR_CTR_HGH		0xab
#define REG_INT_BASE			0xb0
#define REG_SER_DATA			0xb1
#define REG_INT_ENABLE		0xb2
#define REG_SER_STATUS		0xb3
#define REG_INT_STATUS		0xb4
#define REG_KEYPAD			0xb5
#define REG_INT_ACK			0xb6
#define REG_IEEP_DATA_LOW		0xba
#define REG_IEEP_DATA_HGH		0xbb
#define REG_IEEP_ADDR_LOW		0xbc
#define REG_IEEP_ADDR_HGH		0xbd
#define REG_IEEP_STATUS		0xbe
#define REG_IEEP_CMD			0xbe
#define REG_BANK_ROM2			0xc0
#define REG_BANK_SRAM			0xc1
#define REG_BANK_ROM0			0xc2
#define REG_BANK_ROM1			0xc3
#define REG_EEP_DATA_LOW		0xc4
#define REG_EEP_DATA_HGH		0xc5
#define REG_EEP_ADDR_LOW		0xc6
#define REG_EEP_ADDR_HGH		0xc7
#define REG_EEP_STATUS		0xc8
#define REG_EEP_CMD			0xc8
#define REG_RTC_STATUS		0xca
#define REG_RTC_CMD			0xca
#define REG_RTC_DATA			0xcb
#define REG_GPO_EN			0xcc
#define REG_GPO_DATA			0xcd
#define REG_WW_FLASH_CE		0xce

/* RWG_INT_ENABLE, _ACK, _STATUS */
#define HWINT_SER_TX_MASK		(1<<0)	
#define HWINT_KEY_MASK			(1<<1)
#define HWINT_CART_MASK			(1<<2)
#define HWINT_SER_RX_MASK		(1<<3)
#define HWINT_LINE_MASK			(1<<4)
#define HWINT_VBLANK_TMR_MASK 	(1<<5)
#define HWINT_VBLANK_MASK		(1<<6)
#define HWINT_HBLANK_TMR_MASK  	(1<<7)

#define HWINT_SER_TX		0	
#define HWINT_KEY			1
#define HWINT_CART			2
#define HWINT_SER_RX		3
#define HWINT_LINE			4
#define HWINT_VBLANK_TMR 	5
#define HWINT_VBLANK		6
#define HWINT_HBLANK_TMR  	7

#define HWINT_SER_TX_INDEX		(0<<2)	
#define HWINT_KEY_INDEX			(1<<2)
#define HWINT_CART_INDEX		(2<<2)
#define HWINT_SER_RX_INDEX		(3<<2)
#define HWINT_LINE_INDEX		(4<<2)
#define HWINT_VBLANK_TMR_INDEX 	(5<<2)
#define HWINT_VBLANK_INDEX		(6<<2)
#define HWINT_HBLANK_TMR_INDEX  (7<<2)

#define VBLANK_TMR_MODE_MASK	(1<<3)
#define VBLANK_TMR_MODE_ONCE	(0<<3)
#define VBLANK_TMR_MODE_REP		(1<<3)
#define VBLANK_TMR_ENABLE_MASK	(1<<2)
#define VBLANK_TMR_ENABLE		(1<<2)
#define VBLANK_TMR_DISABLE		(0<<2)

#define HBLANK_TMR_MODE_MASK	(1<<1)
#define HBLANK_TMR_MODE_ONCE	(0<<1)
#define HBLANK_TMR_MODE_REP		(1<<1)
#define HBLANK_TMR_ENABLE_MASK	(1<<0)
#define HBLANK_TMR_ENABLE		(1<<0)
#define HBLANK_TMR_DISABLE		(0<<0)


/* CPU IRQs and IVT */
#define CPUINT_DIV_INDEX	(0<<2)
#define CPUINT_STEP_INDEX  	(1<<2)
#define CPUINT_NMI_INDEX	(2<<2)
#define CPUINT_BREAK_INDEX	(3<<2)
#define CPUINT_INTO_INDEX	(4<<2)
#define CPUINT_BOUNDS_INDEX	(5<<2)
#define CPUINT_INVALID_INDEX	(6<<2)
#define PUINT_ESCAPE_INDEX	(7<<2)

/* Memory addresses */
#define DEFADDR_SPR			0x0e00
#define DEFADDR_SCR0		0x1000
#define DEFADDR_SCR1		0x1800
#define DEFADDR_2BPP_TILES	0x2000
#define DEFADDR_2BPP_SPR	0x2000
#define DEFADDR_4BPP_SPR	0x4000
#define DEFADDR_4BPP_TILES  0x8000
#define DEFADDR_4BPP_TILES0 0x4000
#define DEFADDR_4BPP_TILES1 0x8000
#define FIXADDR_PAL_0_7		0xfe00
#define FIXADDR_PAL_8_15	0xff00

/* REG_DISP_CTRL */
#define SCR2_WIN_ENABLE		(1<<5)
#define SCR2_WIN_MODE_MASK	(1<<4)
#define SCR2_WIN_MODE_INSIDE	(0<<4)
#define SCR2_WIN_MODE_OUTSIDE	(1<<4)
#define SPR_WIN_ENABLE		(1<<3)
#define SPR_ENABLE			(1<<2)
#define SCR2_ENABLE			(1<<1)
#define SCR1_ENABLE			(1<<0)

/* REG_LCD_CTRL */
#define LCD_SLEEP_MASK		(1<<0)
#define LCD_CONTRAST_MASK	(1<<1)
#define LCD_SLEEP			(0<<0)
#define LCD_ENABLE			(1<<0)
#define LCD_CONTRAST_LOW	(0<<0)
#define LCD_CONTRAST_HGH	(1<<0)

/* REG_LCD_ICON */
#define ICON_BIG_CIRCLE		(1<<5)
#define ICON_MED_CIRCLE		(1<<4)
#define ICON_SML_CIRCLE		(1<<3)
#define ICON_HORIZ			(1<<2)
#define ICON_VERT			(1<<1)
#define ICON_SLEEP			(1<<0)

/* REG_KEYPAD */

#define KEY_BUTTON_ENABLE	(1<<6)
#define KEY_X_ENABLE		(1<<5)
#define KEY_Y_ENABLE		(1<<4)
#define KEY_BX4Y4_R			(1<<3)
#define KEY_AX3Y3_R			(1<<2)
#define KEY_START_X2Y2_R	(1<<1)
#define KEY_X1Y1_R			(1<<0)

/* REG_DISP_MODE */
#define DISP_BPP_MASK		(1<<7)
#define DISP_4BPP			(1<<7)	/* =1 */
#define DISP_2BPP			(0<<7)	/* =0 */
#define DISP_MONO			(0<<6)	/* =0 */
#define DISP_COLOR			(1<<6)	/* =1 */
#define DISP_FORMAT_PLANAR	(0<<5)	/* =0 */
#define DISP_FORMAT_PACKED	(1<<5)	/* =1 */

/* REG_WSC_SYSTEM */
#define SYS_SYSTEM_MASK		(1<<7)
#define SYS_SYSTEM_WSC		(0<<7)
#define SYS_SYSTEM_SC		(1<<7)
#define SYS_POWER_OFF		(1<<0)

/* libwscl.lib function prototypes */
typedef void (* __far irq_handler_f)(void);

void set_hw_irq(irq_handler_f handler, uint8_t irq);
void dma_copy(void * __near dst, void * __far src, int bytes);
void dma_copy_ram(void * __near dst, void * __near src, int bytes);
extern void memcpy(void * __near dst, void * __far src, int bytes);


#endif  /* _LIBSWC_H_INCLUDED */
