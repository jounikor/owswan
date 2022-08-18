/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2015-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "variety.h"
#include <mbstring.h>
#include <stdio.h>
#if defined( __NT__ )
    #include <windows.h>
#elif defined( __OS2__ )
    #include <wos2.h>
#endif
#include "farfunc.h"
#include "rtdata.h"
#ifdef __FARFUNC__
    #include "nextftok.h"
#else
    #include "nexttok.h"
#endif
#include "thread.h"



/****
***** Tokenize a string.  Equivalent to strtok().
****/

_WCRTLINK unsigned char _FFAR *_NEARFAR(_mbstok_r,_fmbstok_r)( unsigned char _FFAR *str, const unsigned char _FFAR *delim, unsigned char _FFAR **ptr )
{
    unsigned char _FFAR *   string_start;
    int                     count;
    int                     char_len;

//    if( !__IsDBCS )  return( strtok( str, delim ) );

    if( str == NULL ) {
        str = *ptr;
        if( str == NULL )
            return( NULL );
    }

    /*** Skip characters until we reach one not in 'delim' ***/
    #ifdef __FARFUNC__
        while( !_fmbterm(str) && _fmbschr(delim,_fmbsnextc(str))!=NULL )
            str = _fmbsinc( str );
    #else
        while( !_mbterm(str) && _mbschr(delim,_mbsnextc(str))!=NULL )
            str = _mbsinc( str );
    #endif
    if( _NEARFAR(_mbterm,_fmbterm)(str) )  return( NULL );
    string_start = str;

    /*** Skip characters until we reach one in 'delim' ***/
    #ifdef __FARFUNC__
        while( !_fmbterm(str) && _fmbschr(delim,_fmbsnextc(str))==NULL )
            str = _fmbsinc( str );
    #else
        while( !_mbterm(str) && _mbschr(delim,_mbsnextc(str))==NULL )
            str = _mbsinc( str );
    #endif

    /*** Handle the next token ***/
    if( !_NEARFAR(_mbterm,_fmbterm)(str) ) {
        char_len = _NEARFAR(_mbclen,_fmbclen)( str ); /* get char length */
        for( count=0; count<char_len; count++ )
            str[count] = '\0';                  /* replace delim with NULL */
        str += char_len;                        /* start of next token */
        *ptr = str; /* save next start */
        return( string_start );                 /* return next token start */
    } else {
        *ptr = NULL;/* no more tokens */
        return( string_start );                 /* return same token */
    }
}

_WCRTLINK unsigned char _FFAR *_NEARFAR(_mbstok,_fmbstok)( unsigned char _FFAR *str, const unsigned char _FFAR *delim )
{
    #ifdef __FARFUNC__
        _INITNEXTMBFTOK
        return _fmbstok_r( str, delim, &_RWD_nextmbftok);
    #else
        _INITNEXTMBTOK
        return _mbstok_r( str, delim, &_RWD_nextmbtok);
    #endif
}

