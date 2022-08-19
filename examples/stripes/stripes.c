
/*
 * Compile with:
 *  wcl -bcl=wsc -1 -zc -ml -nt=_TEXT -s -i=$OWROOT/rel/h hello.c libwscl.lib
 *
 */

#include <stdint.h>
#include <libwsc.h>

volatile uint16_t g_counter;
volatile uint16_t g_hcnt;

// Turn stack check off (__STK is not defined in startup codes)
#pragma off (check_stack)

void __interrupt __far  vb_irq(void)
{
    g_counter = g_counter + 0x1;
    g_hcnt = g_counter & 0x0f;
    *((uint16_t *)FIXADDR_PAL_0_7) = g_counter;
    ack_hw_irq(HWINT_VBLANK);
}

void __interrupt __far hline_irq(void)
{
    *((uint16_t *)FIXADDR_PAL_0_7) = g_hcnt;
    ++g_hcnt;
    ack_hw_irq(HWINT_HBLANK_TMR);
}

void __far main(void)
{
    uint8_t cmp, cur;
    uint16_t col;

    disable_irq();
    set_hw_irq(vb_irq,HWINT_VBLANK);
    set_hw_irq(hline_irq,HWINT_HBLANK_TMR);

    OUTB(REG_DISP_MODE,DISP_4BPP|DISP_COLOR|DISP_FORMAT_PLANAR);
    OUTB(REG_SCR1_X,0);
    OUTB(REG_SCR1_Y,0);
    OUTB(REG_SCR2_X,0);
    OUTB(REG_SCR2_Y,0);
    OUTB(REG_SCR2_X,0);
    OUTB(REG_DISP_CTRL,SCR1_ENABLE);
    OUTB(REG_LCD_CTRL,LCD_ENABLE|LCD_CONTRAST_HGH);
    OUTB(REG_BACK_COLOR,0);

    OUTB(REG_HTMR_FREQ_LOW,1);
    OUTB(REG_HTMR_FREQ_HGH,0);
    OUTB(REG_TMR_CTRL,HBLANK_TMR_MODE_REP|HBLANK_TMR_ENABLE);

    enable_irq();
    
    while (1) {
    }
}



