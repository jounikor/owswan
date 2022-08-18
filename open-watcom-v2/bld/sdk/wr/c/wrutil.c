/****************************************************************************
*
*                            Open Watcom Project
*
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


#include "wrglbl.h"
#include <mbstring.h>
#include "wresall.h"

#include "clibext.h"


static int WRCountChars( const unsigned char *str, char c )
{
    int     count;

    count = 0;

    while( str != NULL && *str != '\0' ) {
        str = _mbschr( str, c );
        if( str != NULL ) {
            count++;
            str = _mbsinc( str );
        }
    }

    return( count );
}

static int WRCountCharsString( const unsigned char *str, const unsigned char *s )
{
    int     count;

    if( str == NULL || s == NULL ) {
        return( 0 );
    }

    count = 0;
    for( ; *str != '\0'; str = _mbsinc( str ) ) {
        if( _mbschr( s, *str ) != NULL ) {
            count++;
        }
    }

    return( count );
}

// changes all occurrences of chars in 'from' to '\to' in str
char *WRAPI WRConvertStringFrom( const char *_str, const char *_from, const char *to )
{
    char                *new;
    unsigned char       *s;
    size_t              len;
    int                 pos;
    const unsigned char *str = (const unsigned char *)_str;
    const unsigned char *from = (const unsigned char *)_from;

    if( str == NULL || from == NULL || to == NULL ) {
        return( NULL );
    }
    len = strlen( _str ) + WRCountCharsString( str, from ) + 1;
    new = MemAlloc( len );
    if( new == NULL ) {
        return( NULL );
    }

    pos = 0;
    for( ; *str != '\0'; str = _mbsinc( str ) ) {
        s = _mbschr( from, *str );
        if( s != NULL ) {
            new[pos++] = '\\';
            new[pos++] = to[s - from];
        } else {
            new[pos++] = str[0];
            if( _mbislead( str[0] ) ) {
                new[pos++] = str[1];
            }
        }
    }
    new[pos] = '\0';

    return( new );
}

// changes all occurrences of 'from' to '\to' in str
char *WRAPI WRConvertFrom( const char *_str, char from, char to )
{
    char                *new;
    size_t              len;
    int                 pos;
    const unsigned char *str = (const unsigned char *)_str;

    if( str == NULL ) {
        return( NULL );
    }

    len = strlen( _str ) + WRCountChars( str, from ) + 1;
    new = MemAlloc( len );
    if( new == NULL ) {
        return( NULL );
    }

    pos = 0;
    for( ; *str != '\0'; str = _mbsinc( str ) ) {
        if( *str == from ) {
            new[pos++] = '\\';
            new[pos++] = to;
        } else {
            new[pos++] = str[0];
            if( _mbislead( str[0] ) ) {
                new[pos++] = str[1];
            }
        }
    }
    new[pos] = '\0';

    return( new );
}

// changes all occurrences of '\from' to 'to' in str
char *WRAPI WRConvertTo( const char *_str, char to, char from )
{
    char                *new;
    size_t              len;
    int                 pos;
    const unsigned char *str = (const unsigned char *)_str;

    if( str == NULL ) {
        return( NULL );
    }

    len = strlen( _str ) + 1;
    new = MemAlloc( len );
    if( new == NULL ) {
        return( NULL );
    }

    pos = 0;
    for( ; *str != '\0'; str = _mbsinc( str ) ) {
        if( !_mbislead( str[0] ) && !_mbislead( str[1] ) &&
            str[0] == '\\' && str[1] == from ) {
            new[pos++] = to;
            str++;
        } else {
            new[pos++] = str[0];
            if( _mbislead( str[0] ) ) {
                new[pos++] = str[1];
            }
        }
    }
    new[pos] = '\0';

    return( new );
}

// changes all occurrences of the \chars in 'from' to 'to' in str
char * WRAPI WRConvertStringTo( const char *_str, const char *to, const char *_from )
{
    char            *new;
    unsigned char   *s;
    size_t          len;
    int             pos;
    const unsigned char *str = (const unsigned char *)_str;
    const unsigned char *from = (const unsigned char *)_from;

    if( str == NULL || to == NULL || from == NULL ) {
        return( NULL );
    }

    len = strlen( _str ) + 1;
    new = MemAlloc( len );
    if( new == NULL ) {
        return( NULL );
    }

    pos = 0;
    for( ; *str != '\0'; str = _mbsinc( str ) ) {
        if( !_mbislead( str[0] ) && !_mbislead( str[1] ) &&
            str[0] == '\\' && (s = _mbschr( from, str[1] )) != NULL ) {
            new[pos++] = to[s - from];
            str++;
        } else {
            new[pos++] = str[0];
            if( _mbislead( str[0] ) ) {
                new[pos++] = str[1];
            }
        }
    }
    new[pos] = '\0';

    return( new );
}

void WRAPI WRMassageFilter( char *_filter )
{
    char            sep;
    unsigned char   *filter = (unsigned char *)_filter;

    sep = '\t';

    while( filter != NULL && *filter != '\0' ) {
        filter = _mbschr( filter, sep );
        if( filter != NULL ) {
            *filter = '\0';
            filter++;
        }
    }
}

#if defined( __NT__ )
    #ifndef MB_ERR_INVALID_CHARS
        #define MB_ERR_INVALID_CHARS 0x00000000
    #endif
#endif

#if defined( __NT__ )

bool WRAPI WRmbcs2unicodeBuf( const char *src, char *dest, size_t len )
{
    uint_16     *new;
    size_t      len1, len2;

    if( dest == NULL ) {
        return( false );
    }

    if( src != NULL ) {
        len1 = MultiByteToWideChar( CP_OEMCP, MB_ERR_INVALID_CHARS, src, -1, NULL, 0 );
        if( len1 == 0 ) {
            return( false );
        }
    } else {
        len1 = 1;
    }

    // if len is -1 then dont bother checking the buffer length
    if( len != WRLEN_AUTO && ( len1 * sizeof( WCHAR ) ) > len ) {
        return( false );
    }

    new = (uint_16 *)dest;

    if( src != NULL ) {
        len2 = MultiByteToWideChar( CP_OEMCP, MB_ERR_INVALID_CHARS, src, -1, (LPWSTR)new, len1 );
        if( len2 != len1 ) {
            return( false );
        }
    } else {
        new[0] = '\0';
    }

    return( true );
}

bool WRAPI WRunicode2mbcsBuf( const char *src, char *dest, size_t len )
{
    size_t      len1, len2;

    if( dest == NULL ) {
        return( false );
    }

    if( src != NULL ) {
        len1 = WideCharToMultiByte( CP_OEMCP, 0L, (LPCWSTR)src, -1, NULL, 0, NULL, NULL );
        if( len1 == 0 ) {
            return( false );
        }
    } else {
        len1 = 1;
    }

    // if len is -1 then dont bother checking the buffer length
    if( len != WRLEN_AUTO && len1 > len ) {
        return( false );
    }

    if( src != NULL ) {
        len2 = WideCharToMultiByte( CP_OEMCP, 0L, (LPCWSTR)src, -1, dest, len1, NULL, NULL );
        if( len2 != len1 ) {
            return( false );
        }
    } else {
        dest[0] = '\0';
    }

    return( true );
}

bool WRAPI WRmbcs2unicode( const char *src, char **dest, size_t *len )
{
    uint_16     *new;
    size_t      len1, len2;

    if( len == NULL ) {
        return( false );
    }

    if( src != NULL ) {
        len1 = MultiByteToWideChar( CP_OEMCP, MB_ERR_INVALID_CHARS, src, -1, NULL, 0 );
        if( len1 == 0 ) {
            return( false );
        }
    } else {
        len1 = 1;
    }

    *len = len1 * sizeof( WCHAR );

    if( dest == NULL ) {
        return( true );
    }

    new = MemAlloc( len1 * sizeof( WCHAR ) );
    if( new == NULL ) {
        return( false );
    }

    if( src != NULL ) {
        len2 = MultiByteToWideChar( CP_OEMCP, MB_ERR_INVALID_CHARS, src, -1, (LPWSTR)new, len1 );
        if( len2 != len1 ) {
            MemFree( new );
            return( false );
        }
    } else {
        new[0] = '\0';
    }

    *dest = (char *)new;

    return( true );
}

bool WRAPI WRunicode2mbcs( const char *src, char **dest, size_t *len )
{
    char        *new;
    size_t      len1, len2;

    if( len == NULL ) {
        return( false );
    }

    if( src != NULL ) {
        len1 = WideCharToMultiByte( CP_OEMCP, 0L, (LPCWSTR)src, -1, NULL, 0, NULL, NULL );
        if( len1 == 0 ) {
            return( false );
        }
    } else {
        len1 = 1;
    }

    *len = len1;

    if( dest == NULL ) {
        return( true );
    }

    new = MemAlloc( len1 );
    if( new == NULL ) {
        return( false );
    }

    if( src != NULL ) {
        len2 = WideCharToMultiByte( CP_OEMCP, 0L, (LPCWSTR)src, -1, new, len1, NULL, NULL );
        if( len2 != len1 ) {
            MemFree( new );
            return( false );
        }
    } else {
        new[0] = '\0';
    }

    *dest = new;

    return( true );
}

#else

bool WRAPI WRmbcs2unicodeBuf( const char *src, char *dest, size_t len )
{
    uint_16     *new;
    size_t      len1;

    if( dest == NULL ) {
        return( false );
    }

    len1 = 1;
    if( src != NULL ) {
        len1 += strlen( src );
    }

    // if len is -1 then dont bother checking the buffer length
    if( len != WRLEN_AUTO && ( len1 * sizeof( uint_16 ) ) > len ) {
        return( false );
    }

    new = (uint_16 *)dest;

    if( src != NULL ) {
        while( len1-- > 0 ) {
            new[len1] = (uint_16)(uint_8)src[len1];
        }
    } else {
        new[0] = 0;
    }

    return( true );
}

bool WRAPI WRunicode2mbcsBuf( const char *src, char *dest, size_t len )
{
    uint_16     *uni_str;
    size_t      len1;

    if( dest == NULL ) {
        return( false );
    }

    len1 = 1;
    if( src != NULL ) {
        len1 += WRStrlen32( src ) / 2;
    }

    // if len is -1 then dont bother checking the buffer length
    if( len != WRLEN_AUTO && len1 > len ) {
        return( false );
    }

    if( src != NULL ) {
        uni_str = (uint_16 *)src;
        while( len1-- > 0 ) {
            dest[len1] = (char)uni_str[len1];
        }
    } else {
        dest[0] = '\0';
    }

    return( true );
}

bool WRAPI WRmbcs2unicode( const char *src, char **dest, size_t *len )
{
    uint_16     *new;
    size_t      len1;

    if( len == NULL ) {
        return( false );
    }

    len1 = 1;
    if( src != NULL ) {
        len1 += strlen( src );
    }

    *len = len1 * sizeof( uint_16 );

    if( dest == NULL ) {
        return( true );
    }

    new = MemAlloc( len1 * sizeof( uint_16 ) );
    if( new == NULL ) {
        return( false );
    }

    if( src != NULL ) {
        while( len1-- > 0 ) {
            new[len1] = (uint_16)(uint_8)src[len1];
        }
    } else {
        new[0] = 0;
    }

    *dest = (char *)new;

    return( true );
}

bool WRAPI WRunicode2mbcs( const char *src, char **dest, size_t *len )
{
    char        *new;
    uint_16     *uni_str;
    size_t      len1;

    if( len == NULL ) {
        return( false );
    }

    len1 = 1;
    if( src != NULL ) {
        len1 += WRStrlen32( src ) / 2;
    }

    *len = len1;

    if( dest == NULL ) {
        return( true );
    }

    new = MemAlloc( len1 );
    if( new == NULL ) {
        return( false );
    }

    if( src != NULL ) {
        uni_str = (uint_16 *)src;
        while( len1-- > 0 ) {
            new[len1] = (char)uni_str[len1];
        }
    } else {
        new[0] = '\0';
    }

    *dest = new;

    return( true );
}

#endif

char * WRAPI WRWResIDNameToStr( WResIDName *name )
{
    char        *string;

    string = NULL;

    if( name != NULL ) {
        /* alloc space for the string and a \0 char at the end */
        string = MemAlloc( name->NumChars + 1 );
        if( string != NULL ) {
            /* copy the string */
            memcpy( string, name->Name, name->NumChars );
            string[name->NumChars] = '\0';
        }
    }

    return( string );
}

size_t WRAPI WRStrlen32( const char *str )
{
    size_t      len;
    uint_16     *word;

    len = 0;
    if( str != NULL ) {
        for( word = (uint_16 *)str; *word != 0; ++word ) {
            len += 2;
        }
    }

    return( len );
}

size_t WRAPI WRStrlen( const char *str, bool is32bit )
{
    size_t      len;

    if( is32bit ) {
        len = WRStrlen32( str );
    } else {
        len = 0;
        if( str != NULL ) {
            len = strlen( str );
        }
    }

    return( len );
}

size_t WRAPI WRFindFnOffset( const char *_name )
{
    const unsigned char *cp;
    const unsigned char *last;
    const unsigned char *name = (const unsigned char *)_name;

    if( name == NULL ) {
        return( -1 );
    }

    cp = name;
    last = name;
    while( *cp != 0 ) {
        if( !_mbislead( *cp ) && (*cp == ':' || *cp == '\\') ) {
            last = cp + 1;
        }
        cp = _mbsinc( cp );
    }

    if( *last == '\0' ) {
        return( -1 );
    }

    return( (size_t)( last - name ) );
}
