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


#include "plusplus.h"
#include <stdarg.h>
#include "srcfile.h"
#include "vbuf.h"
#include "fmttype.h"
#include "fmtsym.h"
#include "fmtmsg.h"
#include "template.h"
#ifndef NDEBUG
    #include "dbg.h"
#endif


static void leading( VBUF *pbuf, char lead, int len )
/***************************************************/
{
    while( len-- > 0 ) {
        VbufConcChr( pbuf, lead );
    }
}

static bool formatClassForSym( SYMBOL sym, VBUF *buf )
/****************************************************/
{
    CLASSINFO *info;            // - class information for symbol
    bool ok;                    // - return: true ==> is class member
    NAME name;                  // - class name

    ok = false;
    info = SymClassInfo( sym );
    if( info != NULL ) {
        name = info->name;
        if( name != NULL ) {
            VbufConcStr( buf, NameStr( name ) );
            ok = true;
        }
    }
    return( ok );
}

SYMBOL FormatMsg( VBUF *pbuf, char *fmt, va_list args )
/******************************************************
 * this function assumes that pbuf is initialized
 * all information is concatenated to the end of pbuf
 */
{
    VBUF    prefix, suffix;
    char    cfmt;
    char    local_buf[1 + sizeof( int ) * 2 + 1];
    unsigned len;
    SYMBOL  retn_symbol;

    retn_symbol = NULL;
    cfmt = *fmt;
    while( cfmt ) {
        if( cfmt == '%' ) {
            fmt++;
            cfmt = *fmt;
            switch( cfmt ) {
            case '1':   /* %01d */
            case '2':   /* %02d */
            case '3':   /* %03d */
            case '4':   /* %04d */
            case '5':   /* %05d */
            case '6':   /* %06d */
            case '7':   /* %07d */
            case '8':   /* %08d */
            case '9':   /* %09d */
                len = stxicpy( local_buf, va_arg( args, int ) ) - local_buf;
                leading( pbuf, '0', ( cfmt - '0' ) - len );
                VbufConcStr( pbuf, local_buf );
                break;
            case 'c':   /* %c */
                VbufConcChr( pbuf, va_arg( args, int ) );
                break;
            case 's':   /* %s */
                VbufConcStr( pbuf, va_arg( args, char * ) );
                break;
            case 'u':   /* %u */
                VbufConcDecimal( pbuf, va_arg( args, unsigned int ) );
                break;
            case 'd':   /* %d */
                VbufConcInteger( pbuf, va_arg( args, int ) );
                break;
            case 'L':   /* token location */
            {   TOKEN_LOCN *locn;
                locn = va_arg( args, TOKEN_LOCN * );
                if( locn == NULL ) {
                    VbufConcStr( pbuf, "by compiler" );
                } else {
                    char *src_file = SrcFileName( locn->src_file );
                    if( src_file == NULL ) {
                        VbufConcStr( pbuf, "on the command line" );
                    } else {
                        if( ( CompFlags.ew_switch_used )
                          &&( locn->src_file == SrcFileTraceBackFile() ) ) {
                            VbufConcStr( pbuf, "at: " );
                        } else {
                            VbufConcStr( pbuf, "in: " );
                            VbufConcStr( pbuf, SrcFileName( locn->src_file ) );
                        }
                        VbufConcChr( pbuf, '(' );
                        VbufConcInteger( pbuf, locn->line );
                        if( locn->column ) {
                            if( CompFlags.ew_switch_used ) {
                                VbufConcChr( pbuf, ',' );
                                VbufConcInteger( pbuf, locn->column );
                            } else {
                                VbufConcStr( pbuf, ") (col " );
                                VbufConcInteger( pbuf, locn->column );
                            }
                        }
                        VbufConcChr( pbuf, ')' );
                    }
                }
            }   break;
            case 'N':   /* name */
                FormatName( va_arg( args, NAME ), &prefix );
                VbufConcVbuf( pbuf, &prefix );
                VbufFree( &prefix );
                break;
            case 'F':   /* symbol name (decorated) */
            {   SYMBOL      sym;
                sym = va_arg( args, SYMBOL );
                FormatSym( sym, &prefix );
                VbufConcVbuf( pbuf, &prefix );
                VbufFree( &prefix );
            }   break;
            case 'S':   /* symbol name (abbreviated) */
            {   SYMBOL      sym;
                SYMBOL_NAME sn;
                NAME        name;
                sym = va_arg( args, SYMBOL );
                if( sym == NULL ) {
                    VbufConcStr( pbuf, "module data" );
                } else {
                    if( formatClassForSym( sym, pbuf ) ) {
                        VbufConcStr( pbuf, "::" );
                    }
                    if( SymIsCtor( sym ) ) {
                        formatClassForSym( sym, pbuf );
                    } else if( SymIsDtor( sym ) ) {
                        VbufConcChr( pbuf, '~' );
                        formatClassForSym( sym, pbuf );
                    } else {
                        sn = sym->name;
#ifndef NDEBUG
                        if( sn == NULL ) {
                            CFatal( "FormatMsg -- %S symbol has NULL SYMBOL_NAME" );
                        }
#endif
                        name = sn->name;
#ifndef NDEBUG
                        if( name == NULL ) {
                            CFatal( "FormatMsg -- %S SYMBOL_NAME has NULL name" );
                        }
#endif
                        if( name == CppConversionName() ) {
                            VbufConcStr( pbuf, "operator " );
                            FormatType( SymFuncReturnType( sym )
                                      , &prefix
                                      , &suffix );
                            VbufFree( &suffix );
                        } else {
                            FormatName( name, &prefix );
                        }
                        VbufConcVbuf( pbuf, &prefix );
                        VbufFree( &prefix );
                    }
                    if( sym->flag2 & SYMF2_TOKEN_LOCN ) {
                        DbgVerify( retn_symbol == NULL, "too many symbols" );
                        retn_symbol = sym;
                    }
                }
            }   break;
            case 'T':   /* type name */
            {   TYPE type = va_arg( args, TYPE );
                TYPE refed = TypeReference( type );
                if( NULL != refed ) {
                    type = refed;
                }
                FormatType( type, &prefix, &suffix );
                VbufConcVbuf( pbuf, &prefix );
                VbufConcVbuf( pbuf, &suffix );
                VbufFree( &prefix );
                VbufFree( &suffix );
                VbufTruncWhite( pbuf );
                if( NULL != refed ) {
                    VbufConcStr( pbuf, " (lvalue)" );
                }
            }   break;
            case 'P':   /* PTREE list */
            {   const PTREE p = va_arg( args, PTREE );

                FormatPTreeList( p, &prefix );
                VbufConcVbuf( pbuf, &prefix );
                VbufFree( &prefix );
            }   break;
            case 'I':   /* PTREE id */
            {   const PTREE p = va_arg( args, PTREE );

                FormatPTreeId( p, &prefix );
                VbufConcVbuf( pbuf, &prefix );
                VbufFree( &prefix );
            }   break;
            case 'M':   /* template info */
            {   TEMPLATE_INFO * const tinfo = va_arg( args, TEMPLATE_INFO * );
                const SYMBOL sym = tinfo->sym;

                FormatTemplateInfo( tinfo, &prefix );
                VbufConcVbuf( pbuf, &prefix );
                VbufFree( &prefix );
                if( sym->flag2 & SYMF2_TOKEN_LOCN ) {
                    DbgVerify( retn_symbol == NULL, "too many symbols" );
                    retn_symbol = sym;
                }
            }   break;
            case 'C':   /* template specialization */
            {   TEMPLATE_SPECIALIZATION * const tspec =
                    va_arg( args, TEMPLATE_SPECIALIZATION * );

                FormatTemplateSpecialization( tspec, &prefix );
                VbufConcVbuf( pbuf, &prefix );
                VbufFree( &prefix );
            }   break;
            default:
                VbufConcChr( pbuf, cfmt );
            }
        } else {
            VbufConcChr( pbuf, cfmt );
        }
        fmt++;
        cfmt = *fmt;
    }
    return( retn_symbol );
}
