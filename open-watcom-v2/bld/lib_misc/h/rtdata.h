/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2022 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Internal CLIB structures and variables.
*
****************************************************************************/


#ifndef _RTDATA_H_INCLUDED
#define _RTDATA_H_INCLUDED

/* DOS based platforms have stdaux/stdprn in addition to stdin/out/err */
#if defined( __DOS__ ) || defined( __WINDOWS__ )
    #define NUM_STD_STREAMS 5
#else
    #define NUM_STD_STREAMS 3
#endif

#if defined( __NT__ ) || defined( __OS2__ ) || defined( __UNIX__ )
    struct __pipe_info {
        int                 isPipe;     /* non-zero if it's a pipe */
        int                 pid;        /* PID of spawned process */
    };
#endif

typedef struct __stream_link {
    struct __stream_link *  next;
    struct __iobuf *        stream;
    unsigned char *         _base;          /* location of buffer */
    int                     _orientation;   /* wide/byte/not oriented */
    int                     _extflags;      /* extended flags */
    unsigned char           _tmpfchar;      /* tmpfile number */
    unsigned char           _filler[sizeof(int)-1];/* explicit padding */
#if defined( __NT__ ) || defined( __OS2__ ) || defined( __UNIX__ )
    struct __pipe_info      pipeInfo;       /* pipe-related fields */
#endif
} __stream_link;

#if defined( _M_IX86 ) && !defined( __UNIX__ )
typedef struct _87state {
  #if defined( _M_I86 )
    char data[94];          /* 16-bit save area size */
  #else
    char data[108];         /* 32-bit save area size */
  #endif
} _87state;
#endif

#define _FP_BASE(__fp)          ((__fp)->_link->_base)
#ifndef __NETWARE__
    #define _FP_ORIENTATION(__fp)   ((__fp)->_link->_orientation)
    #define _FP_EXTFLAGS(__fp)      ((__fp)->_link->_extflags)
#endif
    #define _FP_TMPFCHAR(__fp)      ((__fp)->_link->_tmpfchar)
#ifndef __NETWARE__
    #define _FP_PIPEDATA(__fp)      ((__fp)->_link->pipeInfo)
#endif

extern __stream_link        *__OpenStreams;
extern __stream_link        *__ClosedStreams;
extern char                 * _WCNEAR __env_mask;  /* ptr to char array of flags */
extern void                 (*__FPE_handler_exit)( void );
#if !defined( __NETWARE__ )
_WCRTDATA extern  int       _cbyte;
_WCRTDATA extern  int       _cbyte2;
          extern  int       _child;
_WCRTDATA extern  unsigned  _curbrk;
          extern  int       _commode;
  #if defined( __WATCOMC__ ) && defined( _M_IX86 )
    #pragma aux             _child "_*";
  #endif
#endif

#if !defined( _M_I86 )
#if defined( __DOS__ )
_WCRTDATA extern char       _WCFAR *_Envptr;
#elif defined( __LINUX__ )
_WCRTDATA extern char       **_Envptr;
#else
_WCRTDATA extern char       *_Envptr;
#endif
#endif

#if defined( _M_IX86 ) && !defined( __UNIX__ )
    extern void                 (*__Save8087)(_87state *);/* Ptr to FP state save rtn (spawn) */
    extern void                 (*__Rest8087)(_87state *);/* Ptr to FP state restore rtn (spawn) */
#endif
extern unsigned short           _8087cw;    /* control word initializer */
extern unsigned char            _no87;      /* NO87 environment var defined */
extern unsigned char            _8087;      /* type of 8087/emulator present */
extern unsigned char            _real87;    /* 8087 coprocessor hardware present */
#if defined( __WATCOMC__ ) && defined( _M_IX86 )
    #pragma aux                 _8087cw "_*";
    #pragma aux                 _no87 "_*";
    #pragma aux                 _8087 "_*";
    #pragma aux                 _real87 "_*";
#endif
extern unsigned char            __uselfn;   /* LFN support available flag */
#if defined( __LINUX__ )
extern unsigned char            _osrev;     /* revision number of the Linux kernel version */
#endif
#if defined( __DOS__ ) || defined( __WINDOWS__ )
extern unsigned char            __isPC98;   /* is NEC PC-98 hardware */
  #if defined( __WATCOMC__ ) && defined( _M_IX86 )
    #pragma aux                 __isPC98 "_*";
  #endif
#endif

#define _RWD_ostream            __OpenStreams
#define _RWD_cstream            __ClosedStreams
#define _RWD_iob                __iob
#if !defined( __NETWARE__ )
    #define _RWD_threadid       _threadid
#endif
#define _RWD_environ            environ
#define _RWD_wenviron           _wenviron
#define _RWD_env_mask           __env_mask
#define _RWD_abort              __abort
#define _RWD_sigtab             __SIGNALTABLE
#define _RWD_FPE_handler_exit   __FPE_handler_exit
#define _RWD_fmode              _fmode
#if !defined( __NETWARE__ )
    #define _RWD_cbyte          _cbyte
    #define _RWD_cbyte2         _cbyte2
    #define _RWD_child          _child
    #define _RWD_amblksiz       _amblksiz
    #define _RWD_curbrk         _curbrk
    #define _RWD_dynend         _dynend
#endif
#if defined( __DOS__ ) || defined( _M_I86 ) && !defined( __QNX__ )
    #define _RWD_psp            _psp
#endif
#if defined( _M_IX86 ) && !defined( __UNIX__ )
    #define _RWD_Save8087       __Save8087
    #define _RWD_Rest8087       __Rest8087
#endif
#define _RWD_8087cw             _8087cw
#define _RWD_no87               _no87
#define _RWD_8087               _8087
#define _RWD_real87             _real87
#if defined( _M_I86 )
    #define _RWD_HShift         _HShift
    #define _RWD_osmode         _osmode
    #define _osmode_REALMODE()  (_RWD_osmode == 0)
    #define _osmode_PROTMODE()  (_RWD_osmode)
#endif
#if !defined( __NETWARE__ )
    #define _RWD_osmajor        _osmajor
    #define _RWD_osminor        _osminor
    #if defined( __NT__ )
        #define _RWD_osbuild    _osbuild
        #define _RWD_osver      _osver
        #define _RWD_winmajor   _winmajor
        #define _RWD_winminor   _winminor
        #define _RWD_winver     _winver
    #endif
    #if defined( __LINUX__ )
        #define _RWD_osrev      _osrev
    #endif
#endif
#define _RWD_tmpfnext           __tmpfnext
#define _RWD_nexttok            _NEXTTOK
#define _RWD_nextftok           _NEXTFTOK
#define _RWD_nextmbtok          _NEXTMBTOK
#define _RWD_nextmbftok         _NEXTMBFTOK
#define _RWD_nextwtok           _NEXTWTOK
#define _RWD_tzname             tzname
#define _RWD_timezone           timezone
#define _RWD_daylight           daylight
#define _RWD_dst_adjust         __dst_adjust
#define _RWD_start_dst          __start_dst
#define _RWD_end_dst            __end_dst
#define _RWD_asctime            _RESULT
#define _RWD_cvtbuf             _CVTBUF
#if defined( __NETWARE__ )
    #define _RWD_ioexit         __ioexit
#endif
#define _RWD_tmpnambuf          _TMPNAMBUF
#define _RWD_randnext           _RANDNEXT

#define _RWD_uselfn             __uselfn

#if defined( __DOS__ ) || defined( __WINDOWS__ )
#define _RWD_isPC98             __isPC98
#endif

extern      void    (*__abort)( void );     // Defined in abort.c

#endif // _RTDATA_H_INCLUDED
