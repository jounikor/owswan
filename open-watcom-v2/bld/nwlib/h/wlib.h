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
* Description:  Master include for librarian.
*
****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include "orl.h"
#include "lib.h"
#include "demangle.h"

#include "wlibio.h"
#include "types.h"
#include "optdef.h"
#include "ops.h"
#include "memfuncs.h"
#include "objfile.h"
#include "inlib.h"

#include "exeelf.h"
#include "wlibutil.h"
#include "libwalk.h"
#include "liblist.h"
#include "cmdline.h"
#include "orlrtns.h"
#include "error.h"
#include "wlibmsg.rh"
#include "ext.h"
#include "proclib.h"
#include "filetab.h"
#include "implib.h"
#include "symlist.h"
#include "writelib.h"
#include "pcobj.h"
#include "omfutil.h"
#include "omfproc.h"
#include "exedos.h"
#include "exeos2.h"
#include "exeflat.h"
#include "exepe.h"
#include "exenov.h"

#define Round(x,s)      (((x) + (s) - 1) & ~((s) - 1))
#define Round2(x)       Round((x),2)
#define Round2var(x)    if((x) & 1) ++(x)

#define FILE_TEMPLATE_MASK  "00000000"
#define FILE_TEMPLATE_FMT   "%8.8d"
