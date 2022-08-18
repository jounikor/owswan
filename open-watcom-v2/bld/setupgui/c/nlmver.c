/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2020 The Open Watcom Contributors. All Rights Reserved.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "wio.h"
#include "watcom.h"
#include "setup.h"
#include "guiutil.h"
#include "setupinf.h"
#include "genvbl.h"
#include "gendlg.h"
#include "utils.h"
#include "nlmver.h"

#include "clibext.h"


static void LenToASCIIZStr( char *tobuf, const unsigned char *frombuf )
{
    memcpy( tobuf, frombuf + 1, *frombuf );
    tobuf[*frombuf] = '\0';
}

static int ReturnNLMVersionInfoFromFile( const VBUF *__pathName, long *majorVersion,
                                  long *minorVersion, long *revision, long *year,
                                  long *month, long *day, char *copyrightString,
                                  char *description )
{
    int             handle, bytes, offset;
    bool            found = false;
    nlm_header_3    *verPtr;
    unsigned char   buffer[READ_SIZE];

    handle = open_vbuf( __pathName, O_BINARY | O_RDONLY );
    if( handle != EFAILURE ) {
        bytes = read( handle, buffer, READ_SIZE );
        close( handle );
        if( bytes == READ_SIZE ) {
            offset = offsetof( nlm_header, descriptionLength );
            if( description != NULL ) {
                LenToASCIIZStr( description, buffer + offset );
            }
            for( ; offset < READ_SIZE; offset++ ) {
                if( !memcmp( VERSION_SIGNATURE, buffer + offset, VERSION_SIGNATURE_LENGTH ) ) {
                    found = true;
                    break;
                }
            }
            if( found ) {
                verPtr = (nlm_header_3 *)( buffer + offset );
                if( majorVersion != NULL ) {
                    *majorVersion = verPtr->majorVersion;
                }
                if( minorVersion != NULL ) {
                    *minorVersion = verPtr->minorVersion;
                }
                if( revision != NULL ) {
                    *revision = verPtr->revision;
                }
                if( year != NULL ) {
                    *year = verPtr->year;
                }
                if( month != NULL ) {
                    *month = verPtr->month;
                }
                if( day != NULL ) {
                    *day = verPtr->day;
                }
                if( copyrightString != NULL ) {
                    found = false;
                    for( ; offset < READ_SIZE; offset++ ) {
                        if( !memcmp( COPYRIGHT_SIGNATURE, buffer + offset, COPYRIGHT_SIGNATURE_LENGTH ) ) {
                            found = true;
                            break;
                        }
                    }
                    if( found ) {
                        LenToASCIIZStr( copyrightString, buffer + offset + COPYRIGHT_SIGNATURE_LENGTH );
                    }
                }
                return( ESUCCESS );
            }
        }
    }
    return( EFAILURE );
}

#define NEWERNLM                    true        // NLM newer than present
#define OLDERNLM                    false       // NLM older than present
#define SAMENLM                     false       // Same versions
#define BAD_PATH_NAME               true        // Version info not found
#define NOT_IN_SYSTEM_DIRECTORY     true        // NLM not found

char sysPath[] = { "SYS:\\SYSTEM\\" };

static bool CheckNewer( const VBUF *newNLM, const VBUF *oldNLM )
{
    int  rc;
    long year, month, day;
    long newyear, newmonth, newday;
    long majorVersion, minorVersion, revision;
    long newmajorVersion, newminorVersion, newrevision;

    /* Get the new NLMs creation date */

    if( (rc = ReturnNLMVersionInfoFromFile( newNLM, &newmajorVersion,
                &newminorVersion, &newrevision, &newyear, &newmonth, &newday,
                NULL, NULL )) != 0 ) {
        return( BAD_PATH_NAME );
    }

    /* Get the old NLMs creation date */

    if( (rc = ReturnNLMVersionInfoFromFile( oldNLM, &majorVersion,
                &minorVersion, &revision, &year, &month, &day,
                NULL, NULL )) != 0 ) {
        return( NOT_IN_SYSTEM_DIRECTORY );
    }

    if( newmajorVersion > majorVersion )
        return( NEWERNLM );
    if( newmajorVersion < majorVersion )
        return( OLDERNLM );

    if( newminorVersion > minorVersion )
        return( NEWERNLM );
    if( newminorVersion < minorVersion )
        return( OLDERNLM );

    if( newrevision > revision )
        return( NEWERNLM );
    if( newrevision < revision )
        return( OLDERNLM );

    if( newyear > year )
        return( NEWERNLM );
    if( newyear < year )
        return( OLDERNLM );

    if( newmonth > month )
        return( NEWERNLM );
    if( newmonth < month )
        return( OLDERNLM );

    if( newday > day )
        return( NEWERNLM );
    if( newday < day )
        return( OLDERNLM );

    /* must be the identical file */
    return( SAMENLM );
}

bool CheckInstallNLM( const VBUF *name, vhandle var_handle )
{
    VBUF        unpacked_as;
    VBUF        temp;
    VBUF        drive;
    VBUF        dir;
    VBUF        fname;
    VBUF        ext;
    bool        cancel;

    VbufInit( &unpacked_as );
    VbufInit( &temp );
    VbufInit( &drive );
    VbufInit( &dir );
    VbufInit( &fname );
    VbufInit( &ext );

    cancel = false;
    VbufSplitpath( name, &drive, &dir, &fname, &ext );
    VbufConcStr( &temp, "_N_" );
    VbufMakepath( &unpacked_as, &drive, &dir, &fname, &temp );
    if( CheckNewer( &unpacked_as, name ) ) {
        VbufSetStr( &dir, sysPath );
        VbufMakepath( &temp, NULL, &dir, &fname, &ext );
        if( CheckNewer( &unpacked_as, &temp ) ) {
            chmod_vbuf( name, PMODE_RWX );
            DoCopyFile( &unpacked_as, name, false );
            VbufSetVbuf( &temp, &fname );
            VbufConcStr( &temp, "_NLM_installed" );
            SetBoolVariableByName_vbuf( &temp, true );
            SetVariableByHandle_vbuf( var_handle, &temp );
        }
    }
    remove_vbuf( &unpacked_as );

    VbufFree( &ext );
    VbufFree( &fname );
    VbufFree( &dir );
    VbufFree( &drive );
    VbufFree( &temp );
    VbufFree( &unpacked_as );
    return( cancel );
}

