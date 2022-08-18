/*
 *  errno.h/cerrno     Error numbers
 *
 * =========================================================================
 *
 *                          Open Watcom Project
 *
 *    Copyright (c) 2002-2010 Open Watcom Contributors. All Rights Reserved.
 *    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
 *
 *    This file is automatically generated. Do not edit directly.
 *
 * =========================================================================
 */
#ifndef _ERRNO_H_INCLUDED
#define _ERRNO_H_INCLUDED

#ifndef _ENABLE_AUTODEPEND
 #pragma read_only_file;
#endif

#ifndef _COMDEF_H_INCLUDED
 #include <_comdef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ERRNO_DEFINED
#define _ERRNO_DEFINED
 #ifndef errno
  _WCRTLINK extern int *__get_errno_ptr( void );
  #define errno (*__get_errno_ptr())
 #else
  _WCRTDATA extern int errno;
 #endif
#endif

#if defined(__STDC_WANT_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__ == 1

#ifndef _ERRNO_T_DEFINED
 #define _ERRNO_T_DEFINED
 typedef int errno_t;
#endif

#endif /* Safer C Library */

#ifndef _ARCH_DIR
 #if defined(__386__)
  #define _ARCH_DIR i386
 #elif defined(__MIPS__)
  #define _ARCH_DIR mips
 #else
  #error unknown platform
  #define _ARCH_DIR
 #endif
 #define _ARCH_INCLUDE(hdr) <arch/ ## _ARCH_DIR ## / ## hdr ## >
#endif /* !_ARCH_DIR */

#include _ARCH_INCLUDE(err_no.h)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
