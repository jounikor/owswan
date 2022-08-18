#ifndef __WATT_DPMI_H
#define __WATT_DPMI_H

#ifndef __WATT_TARGET_H
#error This file must be included after wattcp.h or target.h
#endif

/*
 * The Watcom stackset() functions are from Dan Kegel's RARP implementation
 */
#if defined(__WATCOMC__)
#if defined(_M_I86)  /* 16-bit Watcom */
  extern void stackset (void __far *stack);
  #pragma aux stackset = \
          "mov  ax, ss"  \
          "mov  bx, sp"  \
          "mov  ss, dx"  \
          "mov  sp, si"  \
          "push ax"      \
          "push bx"      \
          __parm [__dx __si]   \
          __modify [__ax __bx];

  extern void stackrestore (void);
  #pragma aux stackrestore = \
          "pop bx"     \
          "pop ax"     \
          "mov ss, ax" \
          "mov sp, bx"             /* don't put esp here */  \
          __modify [__ax __bx];

#else       /* 32-bit Watcom targets */
  #define USES_DPMI_API

  extern void stackset (void *stack);
  #pragma aux stackset =  \
          "mov  ax,  ss"  \
          "mov  ebx, esp" \
          "mov  cx,  ds"  \
          "mov  ss,  cx"  \
          "mov  esp, esi" \
          "push eax"      \
          "push ebx"      \
          __parm [__esi]      \
          __modify [__eax __ebx __ecx];

  extern void stackrestore (void);
  #pragma aux stackrestore = \
          "lss esp, [esp]";

  extern DWORD _get_limit (WORD sel);
  #pragma aux _get_limit = \
          "lsl eax, eax"   \
          __parm [__eax];
#endif

  extern WORD My_CS (void);
  #pragma aux My_CS =  \
          "mov ax, cs" \
          __modify [__ax];

  extern WORD My_DS(void);
  #pragma aux My_DS =  \
          "mov ax, ds" \
          __modify [__ax];

#elif defined(BORLAND386) && (DOSX == WDOSX)
  #define USES_DPMI_API

  #define stackset(stk)  __asm { mov  ax,ss;    \
                                 mov  ebx, esp; \
                                 mov  cx,  ds;  \
                                 mov  ss,  cx;  \
                                 lea  esp, stk; \
                                 push eax;      \
                                 push ebx;      \
                               }
  #define stackrestore() __asm { lss esp, [esp] }

#elif defined(__GNUC__) && (DOSX == WDOSX)
  #define USES_DPMI_API
#endif


#if defined(USES_DPMI_API)

  #include <sys/packon.h>

  struct DPMI_regs {
         DWORD  r_di;
         DWORD  r_si;
         DWORD  r_bp;
         DWORD  reserved;
         DWORD  r_bx;
         DWORD  r_dx;
         DWORD  r_cx;
         DWORD  r_ax;
         WORD   r_flags;
         WORD   r_es, r_ds, r_fs, r_gs;
         WORD   r_ip, r_cs, r_sp, r_ss;
       };

  struct DPMI_callback {
         struct DPMI_regs cb_reg;
         WORD             cb_segment;
         WORD             cb_offset;
       };

  #include <sys/packoff.h>

  #define SEG_OFS_TO_LIN(seg,ofs)  (void*)(((WORD)(seg) << 4) + (WORD)(ofs))

  extern int   dpmi_init           (void);
  extern WORD  dpmi_real_malloc    (WORD size, WORD *sel);
  extern void  dpmi_real_free      (WORD selector);
  extern int   dpmi_lock_region    (void *address, unsigned length);
  extern int   dpmi_unlock_region  (void *address, unsigned length);
  extern void *dpmi_get_real_vector(int intr);
  extern int   dpmi_real_interrupt (int intr, struct DPMI_regs *reg);
  extern int   dpmi_alloc_callback (void (*func)(void), struct DPMI_callback *cb);
  extern int   dpmi_cpu_type       (void);
  extern int   dpmi_dos_yield      (void);

#ifdef BORLAND386
  extern int dpmi_real_interrupt2 (int intr, struct DPMI_regs *reg);
#endif

#endif /* USES_DPMI_API */

#endif /* !__WATT_DPMI_H */
