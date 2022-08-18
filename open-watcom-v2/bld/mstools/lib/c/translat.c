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
* Description:  Translate Microsoft LIB to Watcom options.
*
****************************************************************************/


#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bool.h"
#include "cmdline.h"
#include "deffile.h"
#include "error.h"
#include "file.h"
#include "fuzzy.h"
#include "lib.h"
#include "message.h"
#include "memory.h"
#include "pathconv.h"
#include "translat.h"
#include "pathgrp2.h"

#include "clibext.h"


#define UNSUPPORTED_STR_SIZE    512

static char *   stristr( const char *str, const char *substr );

static FILE *           exp_file = NULL; /* '.exp' file containing export entries for the linker */

static OPT_STRING *     comment = NULL;
static OPT_STRING *     base = NULL;
static OPT_STRING *     heap = NULL;
static OPT_STRING *     stack = NULL;
static OPT_STRING *     internaldllname = NULL;
static OPT_STRING *     stub = NULL;
static OPT_STRING *     version = NULL;


#define NO_CLEAN_STRING
#include "parseext.c"

/*
 * Add one more unsupported option to optStr.
 */
static void append_unsupported( char *optStr, char *opt )
/*******************************************************/
{
    if( optStr[0] != '\0' ) {
        strcat( optStr, " /" );
    } else {
        strcat( optStr, "/" );
    }
    strcat( optStr, opt );
}


/*
 * Parse unsupported options.
 */
static void unsupported_opts( OPT_STORAGE *cmdOpts )
/**************************************************/
{
    char                opts[UNSUPPORTED_STR_SIZE];

    /*** Build a string listing all unsupported options that were used ***/
    opts[0] = '\0';
    if( cmdOpts->debugtype    ) append_unsupported( opts, "DEBUGTYPE"   );
    if( cmdOpts->import       ) append_unsupported( opts, "IMPORT"      );
    if( cmdOpts->include      ) append_unsupported( opts, "INCLUDE"     );
    if( cmdOpts->mac          ) append_unsupported( opts, "MAC"         );
    if( cmdOpts->nodefaultlib ) append_unsupported( opts, "NODEFAULTLIB");
    if( cmdOpts->subsystem    ) append_unsupported( opts, "SUBSYSTEM"   );
    if( cmdOpts->verbose      ) append_unsupported( opts, "VERBOSE"     );

    /*** If an unsupported option was used, give a warning ***/
    if( opts[0] != '\0' ) {
        UnsupportedOptsMessage( opts );
    }

    if( cmdOpts->machine ) {
        if( ( stricmp( "IX86", cmdOpts->machine_value->data ) != 0 ) &&
             ( stricmp( "I386", cmdOpts->machine_value->data ) != 0 ) &&
             ( stricmp( "APX", cmdOpts->machine_value->data ) != 0 ) &&
             ( stricmp( "ALPHA", cmdOpts->machine_value->data ) != 0 ) ) {
            Warning( "%s target platform is not supported", cmdOpts->machine_value->data );
        }
    }
}


/*
 * Parse a .def file if necessary.
 */
static void def_file_opts( OPT_STORAGE *cmdOpts )
/***********************************************/
{
    DefInfo *           info;
    StringList *        strList;

    if( cmdOpts->def ) {
#ifdef __TARGET_AXP__
        info = ParseDefFile( cmdOpts->def_value->data, !cmdOpts->nofuzzy );
#else
        info = ParseDefFile( cmdOpts->def_value->data );
#endif
        if( info != NULL ) {
            strList = info->exports;
            while( strList != NULL ) {
                cmdOpts->export = true;
                add_string( &cmdOpts->export_value, strList->str, '\0' );
                strList = strList->next;
            }

            strList = info->description;
            while( strList != NULL ) {
                add_string( &comment, strList->str, '\0' );
                strList = strList->next;
            }

            if( info->name != NULL ) {
                add_string( &cmdOpts->name_value, info->name, '\0' );
                cmdOpts->name = true;
            }


            if( info->baseAddr != NULL ) {
                add_string( &base, info->baseAddr, '\0' );
            }

            if( info->heapsize != NULL ) {
                add_string( &heap, info->heapsize, '\0' );
            }

            if( info->stacksize != NULL ) {
                add_string( &stack, info->stacksize, '\0' );
            }

            if( info->internalDllName != NULL ) {
                add_string( &internaldllname, info->internalDllName, '\0' );
            }

            if( info->stub != NULL ) {
                add_string( &stub, info->stub, '\0' );
            }

            if( info->version != NULL ) {
                add_string( &version, info->version, '\0' );
            }

            FreeDefInfo(info);
        } else {
            FatalError( "Error parsing %s -- aborting",
                        cmdOpts->def_value->data );
        }
    }
}


/*
 * Get first library or object file from the list. WLIB will use its name as its main library.
 */
static void get_library(OPT_STORAGE *cmdOpts, CmdLine *cmdLine )
/**************************************************************/
{
    char *              filename;
    char *              filecopy;
    int                 fileType;
    char *              newfilename;
    char *              tempfilename;
    bool                quotes_found = false;

    filename = GetNextFile( &fileType, TYPE_LIB_FILE, TYPE_OBJ_FILE, TYPE_INVALID_FILE );
    if( filename != NULL ) {
        if( fileType == TYPE_OBJ_FILE ) {
            if( (cmdOpts->list) || (cmdOpts->extract) ) {
                for( ;; ) {
                    if( stristr( filename, ".obj" ) == NULL ) {
                        AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "%s", filename );
                        FreeMem(filename);
                        break;
                    } else {
                        FreeMem(filename);
                        filename = GetNextFile( &fileType, TYPE_LIB_FILE, TYPE_OBJ_FILE, TYPE_INVALID_FILE );
                    }
                    if( filename == NULL ) {
                        FatalError("no library or object files specified!");
                    }
                }
            } else {
                /*** Strip quotes from filename  and the extension ***/
                filecopy = DupStrMem( filename );
                newfilename = ReallocMem( filename, strlen( filename ) + 4 );
                if( *newfilename == '"' ) {
                    tempfilename = newfilename + 1;                     /* skip leading " */
                    quotes_found = true;
                } else {
                    tempfilename = newfilename;
                }
                if( strchr( tempfilename, '.' ) == NULL )
                    Zoinks();
                *strchr( tempfilename, '.' ) = '\0';
                /*** Append '.lib' at the end of filename and add quotes if needed ***/
                if( quotes_found ) {
                    tempfilename = DupQuoteStrMem( strcat( tempfilename, ".lib" ), '"' );
                } else {
                    tempfilename = DupStrMem( strcat( tempfilename, ".lib" ) );
                }
                FreeMem( newfilename );
                AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "%s", tempfilename );
                FreeMem( tempfilename );
                AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "-+%s", filecopy );
            }
        } else {
            AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "%s", filename );
            FreeMem(filename);
        }
    } else {
        FatalError("no library or object files specified!");
    }

}


/*
 * Get the rest of library files and object files from the list.
 */
static void get_files(CmdLine *cmdLine )
/**************************************/
{
    char *              filename;
    int                 fileType;

    for( ;; ) {
        filename = GetNextFile( &fileType, TYPE_LIB_FILE, TYPE_OBJ_FILE, TYPE_INVALID_FILE );
        if( filename == NULL )
            break;
        AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "-+%s", filename );
        FreeMem( filename );
    }

    /*** Ignore all '.rbj', '.rs', and '.res' files ***/
    for( ;; ) {
        filename = GetNextFile( &fileType, TYPE_RES_FILE, TYPE_RBJ_FILE, TYPE_RS_FILE, TYPE_INVALID_FILE );
        if( filename == NULL )
            break;
        Warning( "Ignoring resource file '%s'", filename );
        FreeMem( filename );
    }
}


/*
 * Appends '.' to a filename without extension ie. 'my_file' becomes 'my_file.'
 */
static char *VerifyDot( char *filename )
/************************************/
{
    char *              newfilename;
    char *              tempfilename;
    char *              filecopy;
    bool                quotes_found = false;

    filecopy = DupStrMem(filename);
    if( strchr( filename, '.' ) == NULL ) {
        /*** Strip quotes from filename ***/
        newfilename = ReallocMem( filecopy, strlen( filecopy ) + 2 );
        if( *newfilename == '"' ) {
            tempfilename = newfilename + 1;                     /* skip leading " */
            tempfilename[strlen( tempfilename ) - 1] = '\0';    /* smite trailing " */
            quotes_found = true;
        } else {
            tempfilename = newfilename;
        }
        /*** Append '.' at the end of filename and add quotes if needed ***/
        if( quotes_found ) {
            filecopy = DupQuoteStrMem( strcat( tempfilename, "." ), '"' );
        } else {
            filecopy = DupStrMem( strcat( tempfilename, "." ) );
        }
        FreeMem( newfilename );
    }
    return filecopy;
}


/*
 * Called by InitFuzzy when an error occurs.
 */
static int fuzzy_init_callback( const char *filename )
/****************************************************/
{
    Warning( "Cannot extract external symbols from '%s' -- fuzzy name matching may not work",
             filename );
    return( 1 );
}


/*
 * Initialize fuzzy linking.
 */
static void init_fuzzy( void )
/****************************/
{
    unsigned            count;
    char *              filename;
    char *              newstr;
    pgroup2             pg;
    char **             objsvector;

    /*** Get the object file names into an array ***/
    count = 0;
    objsvector = AllocMem( (count+1) * sizeof( char * ) );
    for( ;; ) {
        filename = GetNextFile( NULL, TYPE_OBJ_FILE, TYPE_INVALID_FILE );
        if( filename == NULL )
            break;
        newstr = PathConvert( filename, '\'' );

        /*** Skip .res files ***/
        _splitpath2( newstr, pg.buffer, NULL, NULL, NULL, &pg.ext );
        if( CMPFEXT( pg.ext, "res" ) ) {
            FreeMem( newstr );
            continue;
        }

        objsvector[count] = newstr;
        count++;
        objsvector = ReallocMem( objsvector, ( count + 1 ) * sizeof( char * ) );
    }
    objsvector[count] = NULL;

    /*** Ok, now tell the fuzzy module to initialize itself ***/
    InitFuzzy( (const char **)objsvector, NULL, NULL, fuzzy_init_callback );
    FreeMem( objsvector );
}


/*
 * Destroy an OPT_STRING.
 */
static void del_string( OPT_STRING **p )
/**************************************/
{
    OPT_STRING *        s;

    while( *p != NULL ) {
        s = *p;
        *p = s->next;
        FreeMem( s );
    }
}


/*
 * Create the '.exp' file
 */
static void CreateExp( OPT_STORAGE *cmdOpts, char * name )
/********************************************************/
{
    char *              newstr;
    OPT_STRING *        optStr;
    char *              p;
    char *              tmp;
    char                expname[_MAX_PATH];
    pgroup2             pg;

    /*** Replace the '.lib' extension with '.exp' ***/
    _splitpath2( name, pg.buffer, &pg.drive, &pg.dir, &pg.fname, NULL );
    _makepath( expname, pg.drive, pg.dir, pg.fname, "exp" );

    exp_file = fopen( expname, "w" );
    if( exp_file == NULL ) {
        FatalError( "Cannot create file: %s ", expname );
    }

    optStr = comment;
    while( optStr != NULL ) {
        fprintf( exp_file, "OPTION DESCRIPTION %s\n", optStr->data );
        optStr = optStr->next;
    }
    del_string(&comment); // don't need it any more

    if( cmdOpts->name ) {
        if( *cmdOpts->name_value->data == '\'' ) {
            tmp = cmdOpts->name_value->data + 1;    /* skip leading ' */
            tmp[strlen( tmp ) - 1] = '\0';          /* smite trailing ' */
        } else {
            tmp = cmdOpts->name_value->data;
        }
        newstr = PathConvert( tmp, '\'' );
        fprintf( exp_file, "NAME %s\n", newstr );
        FreeMem( newstr );
    }

    if( base ) {
        fprintf( exp_file, "OPTION OFFSET=%s\n", base->data );
    }
    del_string(&base); // don't need it any more

    if( heap ) {
        p = strchr( heap->data, ',' );
        if( p == NULL ) {               /* /HEAP:reserve */
            fprintf( exp_file, "OPTION HEAPSIZE=%s\n", heap->data );
        } else {                        /* /HEAP:reserve,commit */
            *p = '\0';
            fprintf( exp_file, "OPTION HEAPSIZE=%s\n", heap->data );
            p++;
            fprintf( exp_file, "COMMIT HEAP=%s\n", p );
        }
    }
    del_string(&heap); // don't need it any more

    if( stack ) {
        p = strchr( stack->data, ',' );
        if( p == NULL ) {               /* /STACK:reserve */
            fprintf( exp_file, "OPTION STACK=%s\n", stack->data );
        } else {                        /* /STACK:reserve,commit */
            *p = '\0';
            fprintf( exp_file, "OPTION STACK=%s\n", stack->data );
            p++;
            fprintf( exp_file, "COMMIT STACK=%s\n", p );
        }
    }
    del_string(&stack); // don't need it any more

    if( internaldllname ) {
        newstr = DupQuoteStrMem( internaldllname->data, '\'' );
        fprintf( exp_file, "OPTION MODNAME=%s\n",newstr );
        FreeMem( newstr );
    }
    del_string(&internaldllname); // don't need it any more

    if( stub ) {
        if( *stub->data == '\'' ) {
            tmp = stub->data + 1;             /* skip leading ' */
            tmp[strlen( tmp ) - 1] = '\0';    /* smite trailing ' */
        } else {
            tmp = stub->data;
        }
        newstr = PathConvert( tmp, '\'' );
        fprintf( exp_file, "OPTION STUB=%s\n", newstr );
        FreeMem( newstr );
    }
    del_string(&stub); // don't need it any more

    if( version ) {
        fprintf( exp_file, "OPTION VERSION=%s\n", version->data );
    }
    del_string(&version); // don't need it any more
}


/*
 * Creates import library entry from export statement
 */
static char *ImportEntry( char *export, char *dll_name )
/*******************************************************/
{
    char *              internal_name = NULL;
    char *              ordinal = NULL;
    char *              the_rest = NULL;
    char *              entry_name = NULL;
    char *              export_copy;
    char *              p = NULL;
    size_t              len = 9; /* 7 is for all the '.' and '+' required +1 */

    len += strlen( dll_name );
    export_copy = DupStrMem( export );
    p = strchr( export_copy + 1, '\'' );
    if( export_copy[0] != '\'' || p == NULL )
        Zoinks();       // the entry_name must be in quotes
    p++;
    if( *p != '\0' ) { /* there is something after the entry_name */
        entry_name = p; // char after the entry_name
        p=strchr(p,'=');
        if( p != NULL ) { /* internal_name found */
            *p='\0';
            internal_name=(p+1);
            p=strchr(internal_name,' ');
            if( p != NULL ) { /* there is something after the internal_name */
                *p='\0';
                the_rest=(p+1);
            }
        }

        p=strchr(entry_name,'.');
        if( p != NULL ) { /* ordinal found */
            *p='\0';
            ordinal=(p+1);
            if( internal_name==NULL ) {
                p=strchr(ordinal,' ');
                if( p!=NULL ) { /* there is something after the ordinal */
                    *p='\0';
                    the_rest=(p+1);
                }
            }
            len += strlen( ordinal );
        }

        if( (internal_name==NULL) && (ordinal==NULL) ) {
            p=strchr(entry_name,' ');
            if( p!=NULL ) { /* there is something after the entry_name */
                *p='\0';
                the_rest=(p+1);
            }
        }

        *entry_name='\0'; /* separate the entry name from the rest of the export directive */
    }
    entry_name = export_copy;

    if( internal_name == NULL ) {
        internal_name = MatchFuzzy( entry_name );
    } else {
        internal_name = MatchFuzzy( internal_name );
    }

    if( the_rest ) {
        strupr( the_rest);
    }

    /***  do the '.exp'  file entry  ***/
    if( internal_name != NULL ) {
        fprintf( exp_file, "EXPORT %s", entry_name );
        if( ordinal ) {
            fprintf( exp_file, ".%s", ordinal );
        }
        fprintf( exp_file, "=%s", internal_name );
        if( the_rest ) {
            fprintf( exp_file, " %s", the_rest );
        }
        fprintf( exp_file, "\n");
    } else {
        fprintf( exp_file, "EXPORT %s\n", export );
    }


    /***  do the import library entry ***/

    if( the_rest && strstr( the_rest, "PRIVATE" ) ) {
        FreeMem( export_copy );
        return NULL;
    }

    len += strlen( entry_name );

    if( internal_name != NULL ) {
        len += strlen( internal_name );
    }

    p = AllocMem( len );
    *p = '\0';
    strcat( p, "++" );
    if( internal_name != NULL ) {
        strcat( p, internal_name );
    } else {
        strcat( p, entry_name );
    }
    strcat( p, ".'" );
    strcat( p, dll_name );
    strcat( p, "'" );
    if( internal_name!=NULL ) {
        strcat( p, ".." );
        strcat( p, entry_name );
    }
    if( ordinal!=NULL ) {
        strcat( p, "." );
        strcat( p, ordinal );
    }
    FreeMem( export_copy );
    return p;
}




/*
 * Parse librarian options.
 */
static void lib_opts( OPT_STORAGE *cmdOpts, CmdLine *cmdLine )
/************************************************************/
{
    char *              newstr=NULL;
    char *              p;
    OPT_STRING *        optStr;
    char                dllfilename[_MAX_PATH];
    pgroup2             pg;

    if( cmdOpts->def ) {
        if( cmdOpts->out ) {
            newstr = VerifyDot(cmdOpts->out_value->data);
        } else {
            newstr = VerifyDot(cmdOpts->def_value->data);
            newstr = ReallocMem( newstr, strlen( newstr ) + 4 );
            if( strchr( newstr, '.' ) == NULL )
                Zoinks();
            strcpy( strchr( newstr, '.' ), ".lib" );
        }
        AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "%s", newstr );
        CreateExp( cmdOpts, newstr );
        _splitpath2( newstr, pg.buffer, &pg.drive, &pg.dir, &pg.fname, NULL );  /* .dll extension */
        _makepath( dllfilename, pg.drive, pg.dir, pg.fname, "dll" );
        if( cmdOpts->export ) {
            init_fuzzy();
            optStr = cmdOpts->export_value;
            while( optStr != NULL ) {
                p = ImportEntry( optStr->data, dllfilename );
                if( p ) {
                    AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "%s", p );
                    FreeMem( p );
                }
                optStr = optStr->next;
            }

            if( exp_file )
                fclose( exp_file );
            FiniFuzzy();
        } else {
            FatalError( "/EXPORT option not specified!" );
        }
    } else if( cmdOpts->list ) {
        if( cmdOpts->list_value != NULL ) {
            newstr = VerifyDot(cmdOpts->list_value->data);
            AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "/l=%s", newstr );
        }
        get_library(cmdOpts, cmdLine);
    } else if( cmdOpts->extract ) {
        get_library(cmdOpts, cmdLine);
        if( cmdOpts->out ) {
            Warning( "Ignoring unsupported option /OUT following /EXTRACT." );
            Warning( "'%s' will be used as output name.",cmdOpts->extract_value->data );
            cmdOpts->out=0;
        }
        newstr = VerifyDot(cmdOpts->extract_value->data);
        AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "*%s", newstr );

    } else {
        if( cmdOpts->out ) {
            newstr = VerifyDot(cmdOpts->out_value->data);
            AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "%s", newstr );
        } else {
            get_library(cmdOpts, cmdLine);
        }
        get_files(cmdLine);
        optStr = cmdOpts->remove_value;
        while( optStr != NULL ) {
            newstr = VerifyDot(optStr->data);
            AppendFmtCmdLine( cmdLine, LIB_OPTS_SECTION, "-%s", newstr );
            optStr = optStr->next;
        }

    }
    if( newstr != NULL ) {
        FreeMem( newstr );
    }
}


/*
 * Activate options which are always to be turned on.
 */
static void default_opts( OPT_STORAGE *cmdOpts, CmdLine *cmdLine )
/****************************************************************/
{
    if( !cmdOpts->list && !cmdOpts->nowopts ) {
        AppendCmdLine( cmdLine, LIB_OPTS_SECTION, "/q/b" );
    }
}


/*
 * Add any options meant for the Watcom tools.
 */
static void watcom_opts( OPT_STORAGE *cmdOpts, CmdLine *cmdLine )
/***************************************************************/
{
    OPT_STRING *        curr;

    if( cmdOpts->passwopts ) {
        for( curr=cmdOpts->passwopts_value; curr!=NULL; curr=curr->next ) {
            AppendCmdLine( cmdLine, LIB_OPTS_SECTION, curr->data );
        }
    }
}


/*
 * Case-insensitive version of strstr.
 */
static char *stristr( const char *str, const char *substr )
/*********************************************************/
{
    size_t              strLen;
    size_t              substrLen;
    size_t              i, maxi;
    char                ch;

    substrLen = strlen( substr );
    if( substrLen == 0 )
        return( (char *)str );

    strLen = strlen( str );
    maxi = strLen - substrLen + 1;
    ch = substr[0];

    for( i = 0; i < maxi; i++ ) {
        if( str[i] == ch ) {
            if( !strnicmp( str + i, substr, substrLen ) ) {
                return( (char *)( str + i ) );
            }
        }
    }

    return( NULL );
}


/*
 * Translate scanned MS options to Watcom options.
 */
void OptionsTranslate( OPT_STORAGE *cmdOpts, CmdLine *cmdLine )
/*************************************************************/
{
    /*** Parse the /nologo switch now so we can print the banner ***/
    if( cmdOpts->nologo ) {
        QuietModeMessage();
    } else {
        BannerMessage();
    }

    /*** Parse everything ***/
    default_opts( cmdOpts, cmdLine );
    def_file_opts( cmdOpts );
    unsupported_opts( cmdOpts );
    lib_opts( cmdOpts, cmdLine );
    watcom_opts( cmdOpts, cmdLine );
}
