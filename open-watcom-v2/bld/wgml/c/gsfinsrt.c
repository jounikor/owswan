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
* Description:  WGML implement multi letter function &'insert( )
*
****************************************************************************/

#include "wgml.h"

/***************************************************************************/
/*  script string function &'insert(                                       */
/*                                                                         */
/***************************************************************************/

/***************************************************************************/
/*                                                                         */
/* &'insert(new,target<,<n><,<length><,pad>>>):   To  Insert   the  'new'  */
/*    string with  length 'length' into  the 'target' string  after char-  */
/*    acter 'n'.   If 'n' is omitted then  'new' is inserted at the start  */
/*    of 'target'.   The 'pad' character may  be used to extend the 'tar-  */
/*    get' string to length 'n' or the 'new' string to length 'length'.    */
/*      "&'insert(' ','abcdef',3)" ==> "abc def"                           */
/*      "&'insert('123','abc',5,6)" ==> "abc  123   "                      */
/*      "&'insert('123','abc',5,6,'+')" ==> "abc++123+++"                  */
/*      "&'insert('123','abc')" ==> "123abc"                               */
/*      "&'insert('123','abc',5,,'-')" ==> "abc--123"                      */
/*      "&'insert('123','abc',,,'-')" ==> "123abc"                         */
/*                                                                         */
/* ! optional parms LENGTH and PAD are NOT implemented                     */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

condcode    scr_insert( parm parms[MAX_FUN_PARMS], size_t parmcount, char * * result, int32_t ressize )
{
    char            *   pval;
    char            *   pend;
    condcode            cc;
    int                 k;
    int                 n;
    getnum_block        gn;
    char            *   ptarget;
    char            *   ptargetend;

    if( (parmcount < 2) || (parmcount > 3) ) {
        cc = neg;
        return( cc );
    }

    pval = parms[0].start;              // string to insert
    pend = parms[0].stop;

    unquote_if_quoted( &pval, &pend );

    ptarget    = parms[1].start;        // string to be modified
    ptargetend = parms[1].stop;

    unquote_if_quoted( &ptarget, &ptargetend );

    if( pend == pval ) {                // null string insert nothing to do
        **result = '\0';
        return( pos );
    }

    n = 0;                              // default start pos
    gn.ignore_blanks = false;

    if( parmcount > 2 ) {               // evalute startpos
        if( parms[2].stop > parms[2].start ) {
            gn.argstart = parms[2].start;
            gn.argstop  = parms[2].stop;
            cc = getnum( &gn );
            if( cc != pos ) {
                if( !ProcFlags.suppress_msg ) {
                    g_err( err_func_parm, "3 (startpos)" );
                    g_info_inp_pos();
                    err_count++;
                    show_include_stack();
                }
                return( cc );
            }
            n = gn.result;
        }
    }

    k = 0;
    while( (k < n) && (ptarget < ptargetend) && (ressize > 0) ) { // copy up to startpos
        **result = *ptarget++;
        *result += 1;
        k++;
        ressize--;
    }
    if( n > k ) {         // startpos > target length, insert one extra blank
        **result = ' ';
        *result += 1;
        ressize--;
    }

    while( (pval < pend) && (ressize > 0) ) { // insert new string
        **result = *pval++;
        *result += 1;
        ressize--;
    }

    while( (ptarget < ptargetend) && (ressize > 0) ) { // copy rest (if any)
        **result = *ptarget++;
        *result += 1;
        ressize--;
    }

    **result = '\0';

    return( pos );
}

