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

#include "libwsc.h"
#include <stdint.h>
#include <stdbool.h>


void dma_copy(void * dst, void * __far src, int bytes, bool dec)
{
    OUTB(REG_DMA_SRC_LOW,(uint8_t)src);
    OUTB(REG_DMA_SRC_MID,(uint16_t)src >> 8);
    /* FIX: Somewhat ugly __far pointer notmalization.. */
    OUTB(REG_DMA_SRC_HGH,(uint32_t)src >> 28);
    
    OUTB(REG_DMA_DST_LOW,(uint8_t)dst);
    OUTB(REG_DMA_DST_MID,(uint16_t)dst >> 8);
    OUTB(REG_DMA_LEN_LOW,bytes);
    OUTB(REG_DMA_LEN_HGH,bytes >> 8);

    if (dec) {
        OUTB(REG_DMA_CTRL,0xc0);
    } else {
        OUTB(REG_DMA_CTRL,0x80);        
    }
}

