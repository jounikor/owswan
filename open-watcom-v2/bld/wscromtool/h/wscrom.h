#ifndef _WSCROM_DEFINED
#define _WSCROM_DEFINED

typedef enum {
    __begdata=0,
    __enddata,
    __begbss,
    __endbss,
    __romstart,
    __sym_type_max,
    __romend,
    __begromdata,
} sym_type_e;


#define __BEGDATA       "__begdata"
#define __ENDDATA       "__enddata"
#define __BEGBSS        "__begbss"
#define __EBDBSS        "__endbss"
#define __ROMSTART      "__romstart"
#define __ROMEND        "__romend"
#define __BEGROMDATA    "__begromdata"




/*

Offset  Size  Description
-01h    001h  Must be zero!
000h    001h  Publisher ID (see publisher list)
001h    001h  System (0=WonderSwan, 1=WonderSwan Color)
                BIOS seems to ignore this.
002h    001h  Game ID (TODO: make a list?)
                Corresponds to the last 2 digits of the SKU.
003h    001h  Game revision
004h    001h  ROM size
                000h  1Mbit? (128KB)
                001h  2Mbit? (256KB)
                002h  4Mbit (512KB)
                003h  8Mbit (1MB)
                004h  16Mbit (2MB)
                005h  24Mbit (3MB)
                006h  32Mbit (4MB)
                007h  48Mbit (6MB)
                008h  64Mbit (8MB)
                009h  128Mbit (16MB)
005h    001h  Save size/type
                000h  None
                001h  64Kbit SRAM (8KB)
                002h  256Kbit SRAM (32KB)
                003h  1Mbit SRAM (128KB)
                004h  2Mbit SRAM (256KB)
                005h  4Mbit SRAM (512KB)
                010h  1Kbit EEPROM
                020h  16Kbit EEPROM
                050h  8Kbit EEPROM?
006h    001h  Flags
                b2  ROM access speed (0=3 cycle, 1=1 cycle)
                b1  ROM bus width (0=16-bit, 1=8-bit)
                b0  Orientation (0=Horizontal, 1=Vertical)
007h    001h  RTC present (0=no, 1=yes)
008h    002h  16-bit sum of all ROM words except this one
                This is zero for WonderWitch.

*/

/* Initial bootstrap code.. 15 bytes */

uint8_t bootstrap_large[] = {
    0xb8,0x00,0x00,             // mov ax,0x0000
    0xe6,0xc2,                  // out 0xc2,al
    0xea,0x00,0x00,0x00,0x40,   // jmp 0x4000:0x0000
    0xEA,0x06,0x00,0xFE,0xFF    // jmp 0xfffe:0x0006
};

uint8_t bootstrap[] = {
    0xb8,0x00,0x00,             // mov ax,0x0000
    0xe6,0xc2,                  // out 0xc2,al
    0xea,0x00,0x00,0x00,0x20,   // jmp 0x3000:0x0000
    0xEA,0x06,0x00,0xFE,0xFF    // jmp 0xfffe:0x0006
};


#define BOOTSTRAP_SIZE sizeof(bootstrap)
#define BOOTSTRAP_BANK_OFFSET 1
#define DATA_BANK_OFFSET 2
#define BYTES_8MBITS    (1024*1024)
#define LARGE_OFFSET 0x40000

#define MAX_BANKS 256

typedef struct {
    uint8_t size_byte;
    char* size_str;
} sram_size_t;

typedef struct __attribute__((packed)) {
    uint8_t bootstrap[BOOTSTRAP_SIZE];
    uint8_t null;
    uint8_t publisher;
    uint8_t system;
    uint8_t game_id;
    uint8_t game_revision;
    uint8_t rom_size;
    uint8_t save_size;
    uint8_t flags;
    uint8_t rtc;
    uint8_t checksum[2];
} rom_metadata_t;


#endif  /* _WSCROM_DEFINED */
