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


#include "mroffset.h"

MergeOffset::MergeOffset()
            : fileIdx( -1 )             // other code assumes < 0
            , offset( 0xa5a5a5a5L )
{
};

MergeOffset::MergeOffset( uint_8 f, uint_32 o )
            : fileIdx( f ), offset( o )
{
};

MergeOffset::MergeOffset( const MergeOffset& o )
            : fileIdx( o.fileIdx )
            , offset( o.offset )
{
};

#if INSTRUMENTS
const char * MergeOffset::getString() const
//-----------------------------------------
{
    static char buffer[512];
    if( fileIdx < 0 ) {
        sprintf( buffer, "<>" );
    } else {
        sprintf( buffer, "<File: %d, Offset: %#lx>", fileIdx, offset );
    }
    return buffer;
}
#endif


