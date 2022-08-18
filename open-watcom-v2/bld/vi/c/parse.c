/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
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


#include "vi.h"


/*
 * isIgnorable - test if a character is ignorable
 */
static bool isIgnorable( char c, const char *ign )
{
    char    ci;

    while( (ci = *ign++) != '\0' ) {
        if( ci == c || ci == ' ' && isspace( c ) ) {
            return( true );
        }
    }

    return( false );

} /* isIgnorable */

/*
 * TranslateTabs
 */
void TranslateTabs( char *buff )
{
    int     k;
    int     j;

    for( k = 0; buff[k] != '\0'; ++k ) {
        if( buff[k] == '\\' && buff[k + 1] == 't') {
            buff[k] = '\t';
            for( j = k + 1; buff[j] != 0; j++ ) {
                buff[j] = buff[j + 1];
            }
        }
    }

} /* TranslateTabs */

/*
 * GetNextWordOrString
 */
vi_rc GetNextWordOrString( const char **pbuff, char *st )
{
    const char  *buff = *pbuff;

    SKIP_SPACES( buff );
    if( *buff == '/' ) {
        buff = GetNextWord( buff, st, SingleSlash );
        if( *buff == '/' ) {
            SKIP_CHAR_SPACES( buff );
        }
    } else if( *buff == '"' ) {
        buff = GetNextWord( buff, st, SingleDQuote );
        if( *buff == '"' ) {
            SKIP_CHAR_SPACES( buff );
        }
    } else {
        buff = GetNextWord1( buff, st );
    }
    *pbuff = buff;
    if( *st == '\0' ) {
        return( ERR_NO_STRING );
    }
    return( ERR_NO_ERR );

} /* GetNextWordOrString */

/*
 * GetNextWord1 - get next space delimited word in buff
 */
const char *GetNextWord1( const char *buff, char *res )
{
    char    c;

    SKIP_SPACES( buff );
    /*
     * get word
     */
    for( ; (c = *buff) != '\0'; ++buff ) {
        if( isspace( c ) ) {
            SKIP_CHAR_SPACES( buff );
            break;
        }
        *res++ = c;
    }
    *res = '\0';
    return( (char *)buff );

} /* GetNextWord1 */

/*
 * GetNextWord2 - get next space or alternate character delimited word in buff
 */
const char *GetNextWord2( const char *buff, char *res, char alt_delim )
{
    char    c;

    SKIP_SPACES( buff );
    /*
     * get word
     */
    for( ; (c = *buff) != '\0'; ++buff ) {
        if( isspace( c ) ) {
            SKIP_CHAR_SPACES( buff );
            c = *buff;
            break;
        }
        if( c == alt_delim )
            break;
        *res++ = c;
    }
    *res = '\0';
    if( c == alt_delim ) {
        SKIP_CHAR_SPACES( buff );
    }
    return( (char *)buff );

} /* GetNextWord2 */

/*
 * GetNextWord - get next word in buff
 */
const char *GetNextWord( const char *buff, char *res, const char *ign )
{
    size_t      ign_len;
    char        c;

    /*
     * past any leading ignorable chars (if ignore list has single
     * character, then only skip past FIRST ignorable char)
     */
    ign_len = strlen( ign );
    if( ign_len == 1 ) {
        if( isIgnorable( *buff, ign ) ) {
            ++buff;
        }
    } else {
        while( isIgnorable( *buff, ign ) ) {
            ++buff;
        }
    }
    /*
     * get word
     */
    for( ; (c = *buff) != '\0'; ++buff ) {
        /*
         * look for escaped delimiters
         */
        if( c == '\\' && ign_len == 1 ) {
            if( buff[1] == ign[0] ) {
                ++buff;
                *res++ = *buff;
                continue;
            }
            if( buff[1] == '\\' ) {
                ++buff;
                *res++ = '\\';
                *res++ = '\\';
                continue;
            }
        }
        if( isIgnorable( c, ign ) ) {
            break;
        }
        *res++ = c;
    }
    *res = '\0';
    return( (char *)buff );

} /* GetNextWord */

#if defined( __WATCOMC__ ) && defined( _M_IX86 )
extern char toUpper( char );
#pragma aux toUpper = \
        "cmp  al,61h"   \
        "jl short L1"   \
        "cmp  al,7ah"   \
        "jg short L1"   \
        "sub  al,20h"   \
    "L1:"               \
    __parm      [__al] \
    __value     [__al] \
    __modify    []

extern char toLower( char );
#pragma aux toLower = \
        "cmp  al,41h"   \
        "jl short L1"   \
        "cmp  al,5ah"   \
        "jg short L1"   \
        "add  al,20h"   \
    "L1:"               \
    __parm      [__al] \
    __value     [__al] \
    __modify    []
#else
#define toUpper( x )    toupper( x )
#define toLower( x )    tolower( x )
#endif

#define BOTX(s) ( s[0] == '!' && s[1] != '\0' )

#define EOT(c)  ( c == '\0' || c == ' ' )
#define EOTX(s) ( s[0] == exclm && EOT( s[1] ) )

/*
 * Tokenize - convert character to a token
 */
int Tokenize( const char *Tokens, const char *token, bool entireflag )
{
    int         i = 0;
    const char  *t, *tkn;
    char        c, tc, exclm;

    if( Tokens == NULL || *token == '\0' ) {
        return( TOK_INVALID );
    }
    for( t = Tokens; *t != '\0'; ++t ) {
        tkn = token;
        exclm = '\0';
        if( BOTX( t ) ) {
            exclm = '!';
            ++t;
            ++i;
        }
        for( ;; ) {
            c = *t;
            tc = *tkn;
            if( c == 0 ) {
                if( EOT( tc ) ) {
                    return( i );
                }
                if( EOTX( tkn ) ) {
                    return( i - 1 );
                }
                break;
            }
            if( isupper( c ) ) {
                if( c != toUpper( tc ) ) {
                    break;
                }
            } else if( entireflag ) {
                if( toUpper( c ) != toUpper( tc ) ) {
                    break;
                }
            } else {
                if( EOT( tc ) ) {
                    return( i );
                }
                if( EOTX( tkn ) ) {
                    return( i - 1 );
                }
                if( isupper( tc ) ) {
                    if( c != (char)toLower( tc ) ) {
                        break;
                    }
                } else  {
                    if( c != tc ) {
                        break;
                    }
                }
            }
            t++;
            tkn++;
        }
        SKIP_TOEND( t );
        i++;
    }
    return( TOK_INVALID );

} /* Tokenize */

/*
 * GetNumberOfTokens - return number of tokens in a token string
 */
int GetNumberOfTokens( const char *list )
{
    int         i;

    for( i = 0; *list != '\0'; ++i ) {
        if( BOTX( list ) ) {
            ++i;
        }
        list += strlen( list ) + 1;
    }
    return( i );

} /* GetNumberOfTokens */

/*
 * GetLongestTokenLength - return length of longest token in token string
 */
size_t GetLongestTokenLength( const char *list )
{
    size_t      max_len = 0, len;
    const char  *t;

    for( t = list; *t != '\0'; t += len + 1 ) {
        len = strlen( t );
        if( max_len < len ) {
            max_len = len;
        }
    }
    return( max_len );

} /* GetLongestTokenLength */

#if 0
/*
 * BuildTokenList - build an array of tokens
 */
char **BuildTokenList( int num, char *list )
{
    char        **arr, *data, *t;
    int         k, i = 0, off = 0;

    arr = _MemAllocList( num );
    for( ;; ) {

        t = &list[off];
        if( *t == 0 ) {
            break;
        }
        k = strlen( t );
        data = MemAlloc( k + 1 );
        memcpy( data, t, k + 1 );
        arr[i] = data;
        off += k + 1;
        i++;

    }
    return( arr );

} /* BuildTokenList */
#endif

/*
 * GetTokenString - return token string
 */
const char *GetTokenString( const char *list, int num )
{
    int         i = 0;
    const char  *t;

    for( t = list; *t != '\0'; ) {
        if( i == num ) {
            return( (char *)t );
        }
        if( BOTX( t ) ) {
            ++i;
            if( i == num ) {
                return( (char *)( t + 1 ) );
            }
        }
        t += strlen( t ) + 1;
        i++;
    }
    return( NULL );

} /* GetTokenString */

/*
 * GetTokenStringCVT - return token string
 */
char *GetTokenStringCVT( const char *list, int num, char *dst, bool lowercase )
{
    char        *new = dst;
    const char  *src;
    int         f;

    if( dst == NULL ) {
        return( NULL );
    }
    src = GetTokenString( list, num );
    if( src == NULL ) {
        *dst = '\0';
        return( NULL );
    }
    f = ( *src == '!' );
    if( f ) {
        src++;
    }
    if( lowercase ) {
        while( (*dst = (char)tolower( *(unsigned char *)src )) != '\0' ) {
            ++src;
            ++dst;
        }
    } else {
        while( (*dst = *src) != '\0' ) {
            ++src;
            ++dst;
        }
    }
    if( f ) {
        *dst++ = '!';
        *dst = '\0';
    }
    return( new );

} /* GetTokenStringCVT */

/*
 * ReplaceSubString - replace a sub-string with a different one
 */
int ReplaceSubString( char *data, int len, int s, int e, char *rep, int replen )
{
    int i, ln, delta, slen;

    slen = e - s + 1;
    delta = slen - replen;

    /*
     * make room
     */
    ln = len;
    len -= delta;
    if( delta < 0 ) {
        delta *= -1;
        for( i = ln; i > e; i-- ) {
            data[i + delta] = data[i];
        }
    } else if( delta > 0 ) {
        for(i = e + 1; i <= ln; i++ ) {
            data[i - delta] = data[i];
        }
    }

    /*
     * copy in new string
     */
    for( i = 0; i < replen; i++ ) {
        data[s + i] = rep[i];
    }
    return( len );

} /* ReplaceSubString */
