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

#include "clibext.h"


#define isEOL(x)        ((x == CR) || (x == LF) || (x == CTRLZ))

/*
 * SrcOpen - open a file
 */
vi_rc SrcOpen( sfile *curr, files *fi, const char *data, vars_list *vl )
{
    int         i;
    char        name[MAX_SRC_LINE], id[MAX_SRC_LINE], type[MAX_SRC_LINE], t;
    ftype       ft;

    /*
     * validate open statement:
     * OPEN name,id,type
     */
    data = GetNextWord1( data, name );
    if( *name == '\0' ) {
        return( ERR_SRC_INVALID_OPEN );
    }
    data = GetNextWord1( data, id );
    if( *id == '\0' ) {
        return( ERR_SRC_INVALID_OPEN );
    }
    data = GetNextWord1( data, type );
    if( *type == '\0' ) {
        return( ERR_SRC_INVALID_OPEN );
    }
    if( curr->hasvar ) {
        Expand( name, name, vl );
        Expand( id, id, vl );
        Expand( type, type, vl );
    }
    if( id[1] != '\0' || (id[0] < '1' || id[0] > '9') ) {
        return( ERR_SRC_INVALID_OPEN );
    }
    i = id[0] - '1';
    t = type[0];
    if( type[1] != '\0' || (t != 'x' && t != 'r' && t != 'a' && t != 'w') ) {
        return( ERR_SRC_INVALID_OPEN );
    }

    if( fi->ft[i] != SRCFILE_NONE ) {
        return( ERR_SRC_INVALID_OPEN );
    }
    if( t == 'x' ) {
        type[0] = 'r';
    }
    if( name[0] == '@' ) {
        info    *cinfo;

        if( stricmp( &name[1], "." ) == 0 ) {
            cinfo = CurrentInfo;
        } else {
            for( cinfo = InfoHead; cinfo != NULL; cinfo = cinfo->next ) {
                if( strcmp( cinfo->CurrentFile->name, &name[1] ) == 0 ) {
                    break;
                }
            }
            if( cinfo == NULL ) {
                return( ERR_FILE_NOT_FOUND );
            }
        }
        ft = SRCFILE_BUFF;
        fi->u.buffer[i].cinfo = cinfo;
        fi->u.buffer[i].line = 1L;
    } else {
        ft = SRCFILE_FILE;
        fi->u.f[i] = fopen( name, type );
        if( fi->u.f[i] == NULL ) {
            return( ERR_FILE_NOT_FOUND );
        }
        if( t == 'x' ) {
            fclose( fi->u.f[i] );
        }
    }
    if( t == 'x' ) {
        ft = SRCFILE_NONE;
    }

    fi->ft[i] = ft;
    return( ERR_NO_ERR );

} /* SrcOpen */

/*
 * SrcRead - read file
 */
vi_rc SrcRead( sfile *curr, files *fi, const char *data, vars_list *vl )
{
    int         i;
    size_t      j;
    char        id[MAX_SRC_LINE], v1[MAX_SRC_LINE];
    char        tmp[MAX_SRC_LINE];

    /*
     * validate read statement:
     * READ id,variable
     */
    data = GetNextWord1( data, tmp );
    if( *tmp == '\0' ) {
        return( ERR_SRC_INVALID_READ );
    }
    if( curr->hasvar ) {
        Expand( id, tmp, vl );
    } else {
        strcpy( id, tmp );
    }
    data = GetNextWord1( data, tmp );
    if( *tmp == '\0' ) {
        return( ERR_SRC_INVALID_READ );
    }
    if( id[1] != '\0' || (id[0] < '1' || id[0] > '9') ) {
        return( ERR_SRC_INVALID_READ );
    }
    i = id[0] - '1';
    if( !VarName( v1, tmp, vl ) ) {
        return( ERR_SRC_INVALID_READ );
    }
    if( fi->ft[i] == SRCFILE_NONE ) {
        return( ERR_SRC_FILE_NOT_OPEN );
    }
    if( fi->ft[i] == SRCFILE_FILE ) {
        if( fgets( id, sizeof( id ), fi->u.f[i] ) != NULL ) {
            for( j = strlen( id ); j && isEOL( id[j - 1] ); --j ) {
                id[j - 1] = '\0';
            }
            VarAddStr( v1, id, vl );
        } else {
            fclose( fi->u.f[i] );
            fi->ft[i] = SRCFILE_NONE;
            return( END_OF_FILE );
        }
    } else {
        fcb     *cfcb;
        line    *cline;
        vi_rc   rc;

        rc = GimmeLinePtr( fi->u.buffer[i].line, fi->u.buffer[i].cinfo->CurrentFile,
                           &cfcb, &cline );
        if( rc != ERR_NO_ERR ) {
            fi->ft[i] = SRCFILE_NONE;
            return( END_OF_FILE );
        }
        fi->u.buffer[i].line++;
        VarAddStr( v1, cline->data, vl );
    }
    return( ERR_NO_ERR );

} /* SrcRead */

/*
 * SrcWrite - write file
 */
vi_rc SrcWrite( sfile *curr, files *fi, const char *data, vars_list *vl )
{
    int         i;
    char        id[MAX_SRC_LINE], v1[MAX_SRC_LINE];

    /*
     * validate write statement:
     * WRITE id "string"
     */
    data = GetNextWord1( data, id );
    if( *id == '\0' ) {
        return( ERR_SRC_INVALID_WRITE );
    }
    if( GetNextWordOrString( &data, v1 ) != ERR_NO_ERR ) {
        return( ERR_SRC_INVALID_WRITE );
    }
    if( curr->hasvar ) {
        Expand( id, id, vl );
        Expand( v1, v1, vl );
    }
    if( id[1] != '\0' || (id[0] < '1' || id[0] > '9') ) {
        return( ERR_SRC_INVALID_WRITE );
    }
    i = id[0] - '1';

    switch( fi->ft[i] ) {
    case SRCFILE_NONE:
        return( ERR_SRC_FILE_NOT_OPEN );
    case SRCFILE_FILE:
        MyFprintf( fi->u.f[i], "%s\n", v1 );
        break;
    }
    return( ERR_NO_ERR );

} /* SrcWrite */

/*
 * SrcClose - close a work file
 */
vi_rc SrcClose( sfile *curr, files *fi, const char *data, vars_list *vl )
{
    int         i;
    char        id[MAX_SRC_LINE];

    /*
     * validate close statement:
     * CLOSE id
     */
    data = GetNextWord1( data, id );
    if( *id == '\0' ) {
        return( ERR_SRC_INVALID_CLOSE );
    }
    if( curr->hasvar ) {
        Expand( id, id, vl );
    }
    if( id[1] != '\0' || (id[0] < '1' || id[0] > '9') ) {
        return( ERR_SRC_INVALID_CLOSE );
    }
    i = id[0] - '1';

    if( fi->ft[i] == SRCFILE_FILE ) {
        fclose( fi->u.f[i] );
    }
    fi->ft[i] = SRCFILE_NONE;
    return( ERR_NO_ERR );

} /* SrcClose */
