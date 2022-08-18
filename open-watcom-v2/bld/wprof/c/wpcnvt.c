/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2017-2017 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Convert sample data to DIF or CSV format.
*
****************************************************************************/


#include <string.h>
#include <stdio.h>

#include "common.h"
#include "aui.h"
#include "wpaui.h"
#include "dip.h"
#include "sampinfo.h"
#include "wpcnvt.h"
#include "dlgcnvt.h"
#include "wpdata.h"


typedef void (DUMPRTNS)( char *, char *, char *, char *, int );

STATIC unsigned convertEntryCount;
STATIC size_t   convertEntrySize[4];

STATIC void dumpModule( image_info *curr_image, mod_info *curr_mod, DUMPRTNS *dump_rtn )
/**************************************************************************************/
{
    file_info       *curr_file;
    rtn_info        *curr_rtn;
    int             file_count;
    int             rtn_count;

    for( file_count = 0; file_count < curr_mod->file_count; ++file_count ) {
        curr_file = curr_mod->mod_file[file_count];
        for( rtn_count = 0; rtn_count < curr_file->rtn_count; ++rtn_count ) {
            curr_rtn = curr_file->routine[rtn_count];
            if( curr_rtn->tick_count != 0
             || (!curr_rtn->gather_routine && !curr_rtn->unknown_routine) ) {
                dump_rtn( curr_image->name, curr_mod->name, curr_file->name,
                          curr_rtn->name, curr_rtn->tick_count );
            }
        }
    }
}



STATIC void dumpImage( image_info *curr_image, DUMPRTNS *dump_rtn )
/*****************************************************************/
{
    mod_info        *curr_mod;
    int             mod_count;

    for( mod_count = 0; mod_count < curr_image->mod_count; ++mod_count ) {
        curr_mod = curr_image->module[mod_count];
        dumpModule( curr_image, curr_mod, dump_rtn );
    }
}



STATIC void dumpSampleImages( sio_data *curr_sio, DUMPRTNS *dump_rtn )
/********************************************************************/
{
    image_info      *curr_image;
    unsigned        image_index;

    for( image_index = 0; image_index < curr_sio->image_count; ++image_index ) {
        curr_image = curr_sio->images[image_index];
        dumpImage( curr_image, dump_rtn );
    }
}



STATIC void countDIFData( char * image, char * module, char * file, char * routine, int count )
/*********************************************************************************************/
{
    /* unused parameters */ (void)count;

    convertEntryCount++;
    if( convertEntrySize[0] < strlen( image ) )
        convertEntrySize[0] = strlen( image );
    if( convertEntrySize[1] < strlen( module ) )
        convertEntrySize[1] = strlen( module );
    if( convertEntrySize[2] < strlen( file ) )
        convertEntrySize[2] = strlen( file );
    if( convertEntrySize[3] < strlen( routine ) ) {
        convertEntrySize[3] = strlen( routine );
    }
}



STATIC void initDIFData( void )
/*****************************/
{
    fprintf( ConvertFile, "TABLE\n0,1\n\"WProf\"\n" );
    fprintf( ConvertFile, "VECTORS\n0,5\n\"\"\n" );
    fprintf( ConvertFile, "TUPLES\n0,%u\n\"\"\n", (unsigned)convertEntryCount++ );
    fprintf( ConvertFile, "LABEL\n1,0\n\"Image\"\n" );
    fprintf( ConvertFile, "SIZE\n1,%u\n\"\"\n", (unsigned)convertEntrySize[0] );
    fprintf( ConvertFile, "LABEL\n2,0\n\"Module\"\n" );
    fprintf( ConvertFile, "SIZE\n2,%u\n\"\"\n", (unsigned)convertEntrySize[1] );
    fprintf( ConvertFile, "LABEL\n3,0\n\"File\"\n" );
    fprintf( ConvertFile, "SIZE\n3,%u\n\"\"\n", (unsigned)convertEntrySize[2] );
    fprintf( ConvertFile, "LABEL\n4,0\n\"Routine\"\n" );
    fprintf( ConvertFile, "SIZE\n4,%u\n\"\"\n", (unsigned)convertEntrySize[3] );
    fprintf( ConvertFile, "LABEL\n5,0\n\"Count\"\n" );
    fprintf( ConvertFile, "SIZE\n5,10\n\"\"\n" );
    fprintf( ConvertFile, "DATA\n0,0\n\"\"\n" );
}



STATIC void finiDIFData( void )
/*****************************/
{
    fprintf( ConvertFile, "-1,0\nEOD\n" );
}



STATIC void dumpDIFData( char *image, char *module, char *file, char *routine, int count )
/****************************************************************************************/
{
    fprintf( ConvertFile, "-1,0\nBOT\n" );
    fprintf( ConvertFile, "1,0\n\"%s\"\n", image );
    fprintf( ConvertFile, "1,0\n\"%s\"\n", module );
    fprintf( ConvertFile, "1,0\n\"%s\"\n", file );
    fprintf( ConvertFile, "1,0\n\"%s\"\n", routine );
    fprintf( ConvertFile, "0,%d\nV\n", count );
}



STATIC void dumpCommaData( char *image, char *module, char *file, char *routine, int count )
/******************************************************************************************/
{
    fprintf( ConvertFile, "\"%s\",\"%s\",\"%s\",\"%s\",\"%d\"\n", image,
             module, file, routine, count );
}



STATIC void doConvert( a_window wnd, DUMPRTNS *dump_rtn, gui_ctl_id id )
/**********************************************************************/
{
    sio_data        *curr_sio;

    curr_sio = WndExtra( wnd );
    if( curr_sio->curr_image == NULL ) {
        curr_sio->curr_image = curr_sio->images[0];
    }
    if( curr_sio->curr_mod == NULL ) {
        curr_sio->curr_mod = curr_sio->curr_image->module[0];
    }
    if( id == MENU_CONVERT_ALL ) {
        dumpSampleImages( curr_sio, dump_rtn );
    } else if( id == MENU_CONVERT_IMAGE ) {
        dumpImage( curr_sio->curr_image, dump_rtn );
    } else if( id == MENU_CONVERT_MODULE ) {
        dumpModule( curr_sio->curr_image, curr_sio->curr_mod, dump_rtn );
    }
}



void WPConvert( a_window wnd, gui_ctl_id id )
/*******************************************/
{
    DlgGetConvert( wnd );
    if( ConvertFile == NULL )
        return;
    if( OptDIFFormat ) {
        convertEntryCount = 0;
        memset( convertEntrySize, 0, sizeof( convertEntrySize ) );
        doConvert( wnd, &countDIFData, id );
        initDIFData();
        doConvert( wnd, &dumpDIFData, id );
    } else {
        doConvert( wnd, &dumpCommaData, id );
    }
    if( OptDIFFormat ) {
        finiDIFData();
    }
    fclose( ConvertFile );
}
