/*
 * (c) 2022 Jouni 'mr.spiv' Korhonen
 * Some testing with chunky modes and raster irqs.
 * Maybe a rotozoomer some day.
 */

#include <stdint.h>
#include <libwsc.h>
#include "banks.h"

uint16_t g_hcnt;
//uint16_t* __far g_chunky_buffer;
uint16_t __far *g_chunky;
volatile uint8_t g_wait_vb;

#define BG_MAP 0x2000   /* SCR0 */
#define FG_MAP 0x2800   /* SCR1 */


// Stripe 0001 0010 0011 0100 0101 0110 0111 1000 
#pragma pack(2)

const uint8_t __far bg_tiles[] = {
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,

    0x11,0x11,0x22,0x22,
    0x11,0x11,0x22,0x22,
    0x11,0x11,0x22,0x22,
    0x11,0x11,0x22,0x22,
    0x11,0x11,0x22,0x22,
    0x11,0x11,0x22,0x22,
    0x11,0x11,0x22,0x22,
    0x11,0x11,0x22,0x22,

    0x33,0x33,0x44,0x44,
    0x33,0x33,0x44,0x44,
    0x33,0x33,0x44,0x44,
    0x33,0x33,0x44,0x44,
    0x33,0x33,0x44,0x44,
    0x33,0x33,0x44,0x44,
    0x33,0x33,0x44,0x44,
    0x33,0x33,0x44,0x44,

    0x55,0x55,0x66,0x66,
    0x55,0x55,0x66,0x66,
    0x55,0x55,0x66,0x66,
    0x55,0x55,0x66,0x66,
    0x55,0x55,0x66,0x66,
    0x55,0x55,0x66,0x66,
    0x55,0x55,0x66,0x66,
    0x55,0x55,0x66,0x66,

    0x77,0x77,0x88,0x88,
    0x77,0x77,0x88,0x88,
    0x77,0x77,0x88,0x88,
    0x77,0x77,0x88,0x88,
    0x77,0x77,0x88,0x88,
    0x77,0x77,0x88,0x88,
    0x77,0x77,0x88,0x88,
    0x77,0x77,0x88,0x88,

    0x99,0x99,0xaa,0xaa,
    0x99,0x99,0xaa,0xaa,
    0x99,0x99,0xaa,0xaa,
    0x99,0x99,0xaa,0xaa,
    0x99,0x99,0xaa,0xaa,
    0x99,0x99,0xaa,0xaa,
    0x99,0x99,0xaa,0xaa,
    0x99,0x99,0xaa,0xaa,

    0xbb,0xbb,0xcc,0xcc,
    0xbb,0xbb,0xcc,0xcc,
    0xbb,0xbb,0xcc,0xcc,
    0xbb,0xbb,0xcc,0xcc,
    0xbb,0xbb,0xcc,0xcc,
    0xbb,0xbb,0xcc,0xcc,
    0xbb,0xbb,0xcc,0xcc,
    0xbb,0xbb,0xcc,0xcc,

    0xdd,0xdd,0xee,0xee,
    0xdd,0xdd,0xee,0xee,
    0xdd,0xdd,0xee,0xee,
    0xdd,0xdd,0xee,0xee,
    0xdd,0xdd,0xee,0xee,
    0xdd,0xdd,0xee,0xee,
    0xdd,0xdd,0xee,0xee,
    0xdd,0xdd,0xee,0xee,

    0xff,0xff,0x00,0x00,
    0xff,0xff,0x00,0x00,
    0xff,0xff,0x00,0x00,
    0xff,0xff,0x00,0x00,
    0xff,0xff,0x00,0x00,
    0xff,0xff,0x00,0x00,
    0xff,0xff,0x00,0x00,
    0xff,0xff,0x00,0x00
};

/* FG tiles start here */
const uint8_t __far fg_tiles[] = {
    0x00,0x00,0xaa,0xaa,
    0x00,0x00,0xaa,0xaa,
    0x00,0x00,0xaa,0xaa,
    0x00,0x00,0xaa,0xaa,
    0x00,0x00,0xaa,0xaa,
    0x00,0x00,0xaa,0xaa,
    0x00,0x00,0xaa,0xaa,
    0x00,0x00,0xaa,0xaa,

    0x00,0x00,0xbb,0xbb,
    0x00,0x00,0xbb,0xbb,
    0x00,0x00,0xbb,0xbb,
    0x00,0x00,0xbb,0xbb,
    0x00,0x00,0xbb,0xbb,
    0x00,0x00,0xbb,0xbb,
    0x00,0x00,0xbb,0xbb,
    0x00,0x00,0xbb,0xbb,

    0x00,0x00,0xcc,0xcc,
    0x00,0x00,0xcc,0xcc,
    0x00,0x00,0xcc,0xcc,
    0x00,0x00,0xcc,0xcc,
    0x00,0x00,0xcc,0xcc,
    0x00,0x00,0xcc,0xcc,
    0x00,0x00,0xcc,0xcc,
    0x00,0x00,0xcc,0xcc
};

const uint16_t __far bg_line[] = {
    0x0001,0x0002,
    0x0003,0x0004,
    0x0005,0x0006,
    0x0007,0x0008,
    0x0201,0x0202,
    0x0203,0x0204,
    0x0205,0x0206,
    0x0207,0x0208,
    0x0401,0x0402,
    0x0403,0x0404,
    0x0405,0x0406,
    0x0407,0x0408,
    0x0601,0x0602,
    0x0603,0x0604,
    0x0000,0x0000,
    0x0000,0x0000
};

const uint16_t __far fg_line[] = {
    0x0000,0x0000,
    0x0000,0x0000,
    0x0000,0x0000,
    0x0000,0x0609,  // 9
    0x0000,0x0000,
    0x0000,0x0000,
    0x0000,0x0000,
    0x0000,0x060a,  // a
    0x0000,0x0000,
    0x0000,0x0000,
    0x0000,0x0000,
    0x0000,0x060b,  // b
    0x0000,0x0000,
    0x0000,0x0000,
    0x0000,0x0000
};

#define S 16

uint16_t __far offset_table[56] = {
    S+0,2,4,6,8,10,12,14,16,18,
    20,22,24,26,28,30,32,34,36,38,
    40,42,44,46,48,50,52,54,56,58,
    60,62,64,66,68,70,72,74,76,78,
    80,82,84,86,88,90,92,94,96,98,
    100,102,104,106,108,110
};


typedef void (*ram_fn)(int16_t __far *);

#define SMC_START_OFFS 21
#define SMC_OFFS_STEP 8

#define SMC_CODE_SIZE   0x1cf
#define SMC0 0xc000
#define SMC1 (SMC0+SMC_CODE_SIZE)


/* #pragma aux smc_asm_line parm [ds bx]
 * This did not work.. by default the pointer is
 * passed in DX:AX.. so we conform to that..
 */

void __declspec(naked) smc_asm_line(uint16_t __far *base)
{
    /* Supposed to be $1cf bytes of code.. DO NOT MODIFY without
     * changing related defines and support functions
     *
     * In any case this is great example (pun intended) of
     * self-modifying code done in C and inline asm.. 
     */
    _asm {
        pusha
        push ds
        push es

        ;; Pass parameters into proper registers..
        mov bx,ax
        mov ds,dx
        
        ;; Constants
        xor ax,ax
        mov es,ax
        mov si,0xfe02       ;; FIXADDR_PAL_0_7 + 2

        mov ax,ds:[bx]
        mov es:[si],ax

        ;; 1. SMC starts +25 bytes from starts
        ;; step to ext SMC is 4 bytes
        mov ax,ds:0x1111[bx]
        mov es:0x02[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x04[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x06[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x08[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x0a[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x0c[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x0e[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x10[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x12[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x14[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x16[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x18[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x1a[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x1c[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x72[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x20[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x22[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x24[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x26[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x28[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x2a[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x2c[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x2e[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x30[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x32[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x34[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x36[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x38[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x3a[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x3c[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x74[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x40[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x42[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x44[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x46[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x48[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x4a[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x4c[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x4e[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x50[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x52[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x54[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x56[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x58[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x5a[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x5c[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x76[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x60[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x62[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x64[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x66[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x68[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x6a[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x6c[si],ax
        mov ax,ds:0x1111[bx]
        mov es:0x6e[si],ax

        ;;
        pop es
        pop ds
        popa
        retf
    }
}


void __interrupt vb_irq(void)
{
    *(uint16_t *)(FIXADDR_PAL_0_7) = 0;
    g_hcnt = 0;
    g_wait_vb++;
    g_chunky = (uint16_t __far *)MAKE_FAR_PTR(0x20000+offset_table[0]);
    OUTB(REG_LINE_CMP,g_hcnt);
    ack_hw_irq(HWINT_VBLANK);
}

void __interrupt hline_irq(void)
{
    uint16_t __far *s = g_chunky;

#if 1
    ram_fn gen_line = (ram_fn)SMC0;
    gen_line(s);
#else
    uint16_t * __near pal = (uint16_t *)(FIXADDR_PAL_0_7+2);

    /* This produces actually decent code.. */
    pal[1-1] = s[0];        /* SRC0, palette 0, colors 1->15 */
    pal[2-1] = s[1];
    pal[3-1] = s[2];
    pal[4-1] = s[3];
    pal[5-1] = s[4];
    pal[6-1] = s[5];
    pal[7-1] = s[6];
    pal[8-1] = s[7];
    pal[9-1] = s[8];
    pal[10-1] = s[9];
    pal[11-1] = s[10];
    pal[12-1] = s[11];
    pal[13-1] = s[12];
    pal[14-1] = s[13];
    pal[15-1] = s[14];
    pal[58-1] = s[15];      /* SRC1, palette 4, color 10 */
    pal[17-1] = s[16];      /* SRC0, palette 1, colors 1->15 */
    pal[18-1] = s[17];
    pal[19-1] = s[18];
    pal[20-1] = s[19];
    pal[21-1] = s[20];
    pal[22-1] = s[21];
    pal[23-1] = s[22];
    pal[24-1] = s[23];
    pal[25-1] = s[24];
    pal[26-1] = s[25];
    pal[27-1] = s[26];
    pal[28-1] = s[27];
    pal[29-1] = s[28];
    pal[30-1] = s[29];
    pal[31-1] = s[30];
    pal[59-1] = s[31];      /* SRC1, palette 4, color 11 */
    pal[33-1] = s[32];      /* SRC0, palette 3, colors 1-15 */
    pal[34-1] = s[33];
    pal[35-1] = s[34];
    pal[36-1] = s[35];
    pal[37-1] = s[36];
    pal[38-1] = s[37];
    pal[39-1] = s[38];
    pal[40-1] = s[39];
    pal[41-1] = s[40];
    pal[42-1] = s[41];
    pal[43-1] = s[42];
    pal[44-1] = s[43];
    pal[45-1] = s[44];
    pal[46-1] = s[45];
    pal[47-1] = s[46];
    pal[60-1] = s[47];      /* SRC1, palette 4, color 12 */
    pal[49-1] = s[48];      /* SRC0, palette 4, colors 1->9 */
    pal[50-1] = s[49];
    pal[51-1] = s[50];
    pal[52-1] = s[51];
    pal[53-1] = s[52];
    pal[54-1] = s[53];
    pal[55-1] = s[54];
    pal[56-1] = s[55];
#endif
    g_chunky += 128;

    g_hcnt += 4;
    OUTB(REG_LINE_CMP,g_hcnt);
    ack_hw_irq(HWINT_LINE);
}

void move_smc(void)
{
    uint8_t __far *s, __far *d;
    int n;

    d = (uint8_t *)MAKE_FAR_PTR(SMC0);
    s = (uint8_t *)smc_asm_line;

    for (n = 0; n < SMC_CODE_SIZE; n++) {
        *d++ = s[n];
    }
    for (n = 0; n < SMC_CODE_SIZE; n++) {
        *d++ = s[n];
    }
}

void prep_smc(uint8_t __near *fn, uint16_t __far *offs, int num)
{
    // Skip first as it is implicitly given by the start of the pointer..
    int pos = SMC_START_OFFS;

    // Fix offsets
    while (--num > 0) {
        *((uint16_t __near *)(fn+pos)) = *++offs;
        pos += SMC_OFFS_STEP;
    }
}



__declspec(noreturn) void __far main(void)
{
    int n;
    uint16_t * m;
    uint16_t color = 10;

    disable_irq();
    OUTB(REG_DISP_MODE,DISP_4BPP|DISP_COLOR|DISP_FORMAT_PACKED);

    /* Move tile data.. total 17 of those */
    memcpy((void *)0x4000,(void *)bg_tiles,sizeof(bg_tiles));
    memcpy((void *)(0x4000+sizeof(bg_tiles)),(void *)fg_tiles,sizeof(fg_tiles));

    /* Create BG and FG tile maps.. */

    memcpy((void*)BG_MAP,bg_line,64);
    dma_copy_ram((void*)(BG_MAP+64),(void*)BG_MAP,64*17);

    /* Create BG and FG tile maps.. */
    memcpy((void*)FG_MAP,fg_line,64);
    dma_copy_ram((void*)(FG_MAP+64),(void*)FG_MAP,64*17);

    /* ... */

    set_hw_irq(vb_irq,HWINT_VBLANK);
    set_hw_irq(hline_irq,HWINT_LINE);

    OUTB(REG_SCR1_X,0);
    OUTB(REG_SCR1_Y,0);
    OUTB(REG_SCR2_Y,0);
    OUTB(REG_SCR2_X,0);
    OUTB(REG_DISP_CTRL,SCR1_ENABLE|SCR2_ENABLE);
    OUTB(REG_MAP_BASE,0x54);
    OUTB(REG_LCD_CTRL,LCD_ENABLE|LCD_CONTRAST_HGH);
    OUTB(REG_BACK_COLOR,0);

    OUTB(REG_LINE_CMP,0);

    /* Page in the gfx to $2000:0000 */
    SET_BANK_ROM0(__wsc_data_bank+BANK_APINA_RAW);

    /* prepare SMC */
    move_smc();
    prep_smc(SMC0,offset_table,56);

    /* */
    g_wait_vb = 0;

    enable_irq();
    
    while (1) {
        while (g_wait_vb == 0);
        
        color++;
        g_wait_vb = 0;
        *(uint16_t *)(FIXADDR_PAL_0_7) = 0xf00;
    }
}



