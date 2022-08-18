/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Specifies GetOpt() and associated interfaces
*
****************************************************************************/


#ifndef _INCLUDED_GETOPT_H
#define _INCLUDED_GETOPT_H


extern int GetOpt( int *argc, char *argv[], const char *optstr, const char *usage[] );

/*
 * optstr: contains a list of option characters.  If an option character
 *         is followed by a ':', then the option requires a parameter
 *         If an option character is followed by '::', then the option
 *         has an optional parameter, which must be specified after the
 *         option character.
 *
 *         If the first character in optstr is a '#', then an option
 *         specified as a number is returned in its entirety, and
 *         GetOpt returns a '#'; e.g., if -1234 is typed, OptArg
 *         points to the string "1234", and GetOpt returns '#'.
 *
 * usage:  is an array of strings describing the functionality of the
 *         command, must be NULL terminated.  The first element of
 *         the array must describe the command in brief.
 *
 * When there are no options left, GetOpt returns -1 and argc contains
 * the number of non-option parameters.  Argv is collapsed to contain only
 * the non-option paramters
 */

extern char     *OptArg;
extern int      OptInd;
extern char     *OptEnvVar; /* must be defined, specifies env var to search */
extern char     OptChar;
extern char     AltOptChar;

#endif
