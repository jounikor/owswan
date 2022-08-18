/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2004-2013 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  WGML implement multi letter function &'min( )
*
****************************************************************************/

#include "wgml.h"

/***************************************************************************/
/*  script string function &'min(                                          */
/*                                                                         */
/***************************************************************************/

/***************************************************************************/
/*                                                                         */
/* &'min(number<,number ...>):  The Minimum function returns the smallest  */
/*    in a list of numbers.                                                */
/*      &'min(5,6,7,8) ==> 5                                               */
/*      &'min(-5,(5*5),0) ==> -5                                           */
/*      &'min('5','6','7') ==> 5                                           */
/*      &'min(1,&'min(2,&'min(3,4))) ==> 1                                 */
/*                                                                         */
/*  ! Maximum number count is 6                                            */
/*                                                                         */
/***************************************************************************/


condcode    scr_min( parm parms[MAX_FUN_PARMS], size_t parmcount, char * * result, int32_t ressize )
{
    char            *   pval;
    char            *   pend;
    condcode            cc;
    int                 k;
    getnum_block        gn;
    long                minimum;

    /* unused parameters */ (void)ressize;

    if( (parmcount < 2) || (parmcount > 6) ) {
        cc = neg;
        return( cc );
    }

    minimum = LONG_MAX;

    gn.ignore_blanks = false;

    for( k = 0; k < parmcount; k++ ) {
        pval = parms[k].start;
        pend = parms[k].stop;

        unquote_if_quoted( &pval, &pend );

        if( pend == pval ) {            // null string nothing to do
            continue;                   // skip empty value
        }
        gn.argstart = pval;
        gn.argstop  = pend;
        cc = getnum( &gn );
        if( !(cc == pos  || cc == neg) ) {
            if( !ProcFlags.suppress_msg ) {
                g_err( err_func_parm, "" );
                g_info_inp_pos();
                err_count++;
                show_include_stack();
            }
            return( cc );
        }
        if( gn.result < minimum ) {
            minimum = gn.result;        // new minimum
        }
    }

    *result += sprintf( *result, "%ld", minimum );

    return( pos );
}
