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
* Description:  Support functions for using strings loaded from resources.
*
****************************************************************************/


#include "commonui.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "cguimem.h"
#include "ldstr.h"

/* No string to be loaded can be more than LDSTR_MAX_STR_LEN bytes long. */
#define LDSTR_MAX_STR_LEN       500

static char         getStringBuffer[LDSTR_MAX_STR_LEN];
static char         tmpBuf[LDSTR_MAX_STR_LEN];
static HINSTANCE    curInst;

/*
 * GetRCString - return a pointer to a string from the resource file
 *             - the pointer is only valid until the next call to
 *               GetString
 */
const char *GetRCString( msg_id msgid )
{
    int         len;

    len = LoadString( curInst, msgid, getStringBuffer, LDSTR_MAX_STR_LEN );
    if( len < 0 )
        len = 0;
    getStringBuffer[len] = '\0';
    return( getStringBuffer );

} /* GetRCString */

/*
 * AllocRCString - return a pointer to a string from the resource file
 *               - the caller must free the memory
 */
char *AllocRCString( msg_id id )
{
    char        *ret;
    int         len;

    len = LoadString( curInst, id, tmpBuf, LDSTR_MAX_STR_LEN );
    if( len < 0 )
        len = 0;
    tmpBuf[len++] = '\0';
    ret = MemAlloc( len );
    if( ret != NULL ) {
        memcpy( ret, tmpBuf, len );
    }
    return( ret );

} /* AllocRCString */

/*
 * CopyRCString - copy a string from the resource file into a buffer
 */
int CopyRCString( msg_id id, char *buf, int bufsize )
{
    int         len;

    len = LoadString( curInst, id, buf, bufsize );
    if( len < 0 )
        len = 0;
    buf[len] = '\0';
    return( len );

} /* CopyRCString */

/*
 * FreeRCString - free the memory allocated by AllocRCString
 */
void FreeRCString( char *str )
{
    MemFree( str );
}

/*
 * SetInstance - set the instance handle used to load resource strings
 */
void SetInstance( HINSTANCE inst )
{
    curInst = inst;

} /* SetInstance */
