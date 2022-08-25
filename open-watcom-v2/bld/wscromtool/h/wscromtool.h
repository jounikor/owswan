#ifndef _WSCROMTOOL_H_DEFINED
#define _WSCROMTOOL_H_DEFINED

#define POSIXLY_CORRECT

/* Only 2001 mappers.. */
typedef struct {
    uint8_t size_byte;
    int32_t size_max;
} rom_size_t;

/*
 * Prototypes for generic helper functions..
 */

int find_rom_size(int romlen, rom_size_t *size);
int find_rom_size_min8Mb(int romlen, rom_size_t *size);


#endif  /* _WSCROMTOOL_H_DEFINED */
