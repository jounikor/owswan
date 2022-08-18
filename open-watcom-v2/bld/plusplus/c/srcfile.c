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
* Description:  Source file management.
*
****************************************************************************/


#include "plusplus.h"
#include <errno.h>
#include "wio.h"
#include "memmgr.h"
#include "fname.h"
#include "ring.h"
#include "preproc.h"
#include "scan.h"
#include "stats.h"
#include "carve.h"
#include "sysdep.h"
#include "name.h"
#include "cmdline.h"
#include "pcheader.h"
#include "initdefs.h"
#include "iosupp.h"
#include "pathlist.h"
#ifndef NDEBUG
    #include "pragdefn.h"
    #include "togglesd.h"
#endif
#include "brinfo.h"
#include "cscanbuf.h"

#include "clibext.h"


//
// A guarded file is one which contains:
//
//  (1) White space and comments
//  (2) Some form of #if (#if, #ifdef, etc )
//  (3) A body with no form of #else for the #if in (2)
//  (4) A #endif for the #if in (2)
//  (5) White space and comments
//
// On the second and subsequent reads of a guarded file, the file can be
// skipped when not preprocessing and the condition for the #if in (2)
// indicates that the body need not be processed.
//
#define GUARD_DEFS \
    GDEF( GUARD_INCLUDE )   /* - always include */ \
    GDEF( GUARD_IF )        /* - file is guarded (#if expr, #ifdef) */ \
    GDEF( GUARD_IFNDEF )    /* - file is guarded (#ifndef) */ \
                            /* following valid while file being processed */ \
    GDEF( GUARD_TOP )       /* - processing white-space at top */ \
    GDEF( GUARD_MID )       /* - processing body (#if to #endif) */ \
    GDEF( GUARD_BOT )       /* - processing white-space at bottom */ \
                            /* used only when !defined(NDEBUG) */ \
    GDEF( GUARD_NULL )      /* - to force printing */ \

enum {                          // GUARD STATE
    #define GDEF( id )  id,
    GUARD_DEFS
    #undef GDEF
};

#ifndef NDEBUG
static const char *guardStateNames[] = {    // - names of guard states
    #define GDEF( id )  #id ,
    GUARD_DEFS
    #undef GDEF
};
#endif

struct dir_list {               // DIR_LIST -- directory entry
    DIR_LIST    *next;          // - next in ring
    char const  *name;          // - directory name
};

union freed_open_file {
    OPEN_FILE   fb;                     // - freed open file
    OPEN_FILE   **next;                 // - next in free chain
};

// 'srcFile' and 'openFile' must always be kept in synch!
// use set_SrcFile to set 'srcFile' just to be sure
static SRCFILE srcFile;                     // current source file
static OPEN_FILE *openFile;                 // actual open file

#define BLOCK_SRCFILE   16
#define BLOCK_OPEN_FILE 8
#define BLOCK_RO_DIRS   4
static carve_t carveSrcFile;                // source files
static carve_t carveOpenFile;               // open files
static carve_t alternateCarveSrcFile;       // loaded pchdr source files
static carve_t carveRoDir;                  // read-only directories
static union freed_open_file *freeFiles;    // freed open files
static SRCFILE srcFilesUnique;              // list of unique files
static SRCFILE traced_back;                 // last file traced back for msg.s
static SRCFILE primarySrcFile;              // primary source file
static DIR_LIST* roDirs;                    // read-only directories
static unsigned totalSrcFiles;              // running total of SRCFILE's

static const unsigned char  notFilled[2] = { '\n', '\0' };  // default buffer to force srcReadBuffer

static int lastChar;                    // unknown char to return in GetNextChar

static unsigned numBlanks1;
static unsigned numTABs;
static unsigned numBlanks2;
static unsigned tabWidth;

int (*NextChar)( void ) = GetNextChar;

// the following table is used to map three character sequences
// beginning with ?? followed by 'tri' into the single character 'new'
struct tri_graph {
    unsigned char   tri;
    unsigned char   new;
};

static const struct tri_graph triGraphs[] = {
    { '=', '#' },
    { '(', '[' },
    { '/', '\\'},
    { ')', ']' },
    { '\'','^' },
    { '<', '{' },
    { '!', '|' },
    { '>', '}' },
    { '-', '~' },
    { '\0','\0'}
};

#define _FIND_ACTUAL( curr ) for( ; curr->alias; curr = curr->sister )

#ifndef NDEBUG
static OPEN_FILE *activeSrc( void )
{
    SRCFILE src;

    src = srcFile;
    if( src != NULL ) {
        if( src->active != openFile ) {
            CFatal( "openFile is out of synch with srcFile" );
        }
    } else {
        if( openFile != NULL ) {
            CFatal( "openFile is out of synch with srcFile" );
        }
    }
    return( openFile );
}
#else
#define activeSrc()     (openFile)
#endif

static void set_srcFile( SRCFILE src )
{
    srcFile = src;
    if( src != NULL ) {
        openFile = src->active;
    } else {
        openFile = NULL;
    }
}


static void cleanupOpenFiles(   // CLEAN UP ON MEMORY FAILURE
    void )
{
    union freed_open_file *fr;

    RingIterBegSafe( freeFiles, fr ) {
        CMemFreePtr( &fr->fb.buff );
    } RingIterEndSafe( fr )
}


static void srcFileInit(        // INITIALIZE SOURCE FILING
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    traced_back = NULL;
    freeFiles = NULL;
    srcFilesUnique = NULL;
    primarySrcFile = NULL;
    roDirs = NULL;
    alternateCarveSrcFile = NULL;
    totalSrcFiles = 0;
    carveSrcFile = CarveCreate( sizeof( *srcFile ), BLOCK_SRCFILE );
    carveOpenFile = CarveCreate( sizeof( *freeFiles ), BLOCK_OPEN_FILE );
    carveRoDir = CarveCreate( sizeof( DIR_LIST ), BLOCK_RO_DIRS );
    CMemRegisterCleanup( cleanupOpenFiles );
}


static void srcFileFini(        // COMPLETE SOURCE FILING
    INITFINI* defn )            // - definition
{
    /* unused parameters */ (void)defn;

    if( srcFile != NULL ) {
        if( activeSrc() != NULL ) {
            while( NULL != srcFile ) {
                SrcFileClose( true );
            }
        } else {
            set_srcFile( NULL );
        }
    }
    cleanupOpenFiles();
    CarveDestroy( carveSrcFile );
    CarveDestroy( carveOpenFile );
    CarveDestroy( alternateCarveSrcFile );
    CarveDestroy( carveRoDir );
    // cleanupOpenFiles is registered as a memory callback so once
    // we destroy all OPEN_FILEs, we can't reference them anymore
    freeFiles = NULL;
}

INITDEFN( srcfile, srcFileInit, srcFileFini )




static unsigned getGuardState(  // GET GUARD STATE FOR CURRENT FILE
    void )
{
    SRCFILE src_file = srcFile;

    _FIND_ACTUAL( src_file );
    return( src_file->guard_state );
}


static void setGuardState(      // SET GUARD STATE FOR CURRENT FILE
    unsigned new_state )        // - new state
{
    SRCFILE actual;             // - non-aliased SRCFILE

    actual = srcFile;
    _FIND_ACTUAL( actual );
#ifndef NDEBUG
    if( TOGGLEDBG( dump_tokens ) ) {
        if( actual->guard_state != new_state ) {
            printf( "New Guard State[%s]: %s\n"
                  , actual->name
                  , guardStateNames[new_state] );
        }
    }
#endif
    actual->guard_state = new_state;
}


static SRCFILE srcFileGetUnique(// FIND SOURCE FILE IN UNIQUE LIST
    char const *name )          // - name of source file
{
    SRCFILE srch;               // - searched source file

/* Do a string sensitive compare -- it's safer. */
    for( srch = srcFilesUnique; srch != NULL; srch = srch->unique ) {
       if( strcmp( srch->name, name ) == 0 ) {
           break;
       }
    }
    return( srch );
}


static SRCFILE srcFileAddUnique(// ADD NEW SOURCE FILE TO UNIQUE LIST
    SRCFILE curr )              // - current source file
{
    SRCFILE srch;               // - searched source file

    srch = srcFileGetUnique( curr->name );
    if( srch == NULL ) {
        curr->unique = srcFilesUnique;
        srcFilesUnique = curr;
    }
    return( srch );
}


static SRCFILE srcFileAlloc(    // ALLOCATE A SRCFILE
    void *fp,                   // - system file control
    char *name,                 // - file name
    time_t ftime )
{
    SRCFILE new_src;            // - new source file
    OPEN_FILE *old_act;         // - open-file information (old)
    OPEN_FILE *new_act;         // - open-file information (new)

    new_src = CarveAlloc( carveSrcFile );
    new_src->name = name;
    new_src->full_name = NULL;
    new_src->parent = srcFile;
    new_src->unique = NULL;
    new_src->pch_child = NULL;
    new_src->primary = false;
    new_src->lib_inc = false;
    new_src->alias = false;
    new_src->cmdline = false;
    new_src->read_only = false;
    if( fp == NULL && name == NULL ) {
        new_src->cmdline = true;
        new_src->read_only = true;
    }
    new_src->cmdlneol = false;
    new_src->cmdlneof = false;
    new_src->uncached = false;
    new_src->free = false;
    new_src->pch_create = false;
    new_src->pch_kludge = false;
    new_src->assume_file = true;
    new_src->found_eof = false;
    new_src->once_only = false;
    new_src->ignore_swend = false;
    new_src->index = totalSrcFiles++;
    new_src->active = NULL;
    new_src->ifndef_name = NULL;
    new_src->ifndef_len = 0;
    new_src->sister = new_src;
    new_src->guard_state = GUARD_NULL;
    new_src->time_stamp = ftime;
    new_src->force_include = false;
    MacroStateGet( &(new_src->macro_state) );
    if( srcFile == NULL ) {
        new_src->parent_locn = 0;
    } else {
        if( srcFile->pch_create ) {
            srcFile->pch_child = new_src;
            new_src->pch_kludge = true;
        }
        old_act = activeSrc();
        if( CurrChar == '\n' && old_act->line > 0 ) {
            // if we have scanned a \n already then getCharCheck
            // will have incremented the current line too soon
            new_src->parent_locn = old_act->line - 1;
        } else {
            new_src->parent_locn = old_act->line;
        }
        old_act->currc = CurrChar;
    }
    new_act = RingPop( &freeFiles );
    if( new_act == NULL ) {
        new_act = CarveAlloc( carveOpenFile );
        new_act->buff = NULL;
    }
    new_src->active = new_act;
    new_act->line = 0;
    new_act->column = 0;
    new_act->nextc = &notFilled[0];
    new_act->lastc = &notFilled[1];
    new_act->fp = fp;
    return( new_src );
}

static void setJustOpenedGuardState( void )
{
    if( CompFlags.cpp_mode ) {
        setGuardState( GUARD_INCLUDE );
    } else {
        setGuardState( GUARD_TOP );
    }
}

static void setReOpenedGuardState( unsigned guard_state )
{
    if( CompFlags.cpp_mode ) {
        guard_state = GUARD_INCLUDE;
    } else if( guard_state == GUARD_IFNDEF ) {
        // seen this #include before with #ifndef MACRO but MACRO is not defined
        guard_state = GUARD_TOP;
    }
    setGuardState( guard_state );
}

void SrcFileSetCreatePCHeader(  // MARK SRCFILE TO CREATE PCHDR WHEN #include FINISHES
    void )
{
    srcFile->pch_create = true;
}

void SrcFileNotAFile(           // LABEL SRCFILE AS A DEVICE
    SRCFILE sf )                // - the device source file
{
    if( sf != NULL ) {
        sf->assume_file = false;
    }
}

SRCFILE SrcFileOpen(            // OPEN NEW SOURCE FILE
    void *fp,                   // - system file control
    const char *name,           // - file name
    time_t ftime )
{
    SRCFILE     new_src;        // - new source file
    OPEN_FILE   *new_act;       // - open-file information (new)

    new_src = srcFileAlloc( fp, FNameAdd( name ), ftime );
    if( CompFlags.cpp_output ) {
        CppEmitPoundLine( 1, new_src->name, EL_NULL );
    }
    set_srcFile( new_src );
    new_act = activeSrc();
    if( ( new_act->buff == NULL ) && ( fp != NULL ) ) {
        // need room for '\0' to signal end of buffer
        // may need room for three ';' in PCH source files
        new_act->buff = CMemAlloc( PRODUCTION_BUFFER_SIZE + 1 + 3 );
    }
    if( fp == NULL ) {
        setGuardState( GUARD_IFNDEF );
    } else {
        SRCFILE old = srcFileAddUnique( new_src );
        if( old == NULL ) {
            setJustOpenedGuardState();
        } else {
            new_src->time_stamp = old->time_stamp;
            _FIND_ACTUAL( old );
            setReOpenedGuardState( old->guard_state );
        }
    }
    CurrChar = '\n';
    return( new_src );
}


void SrcFileAlias(              // SET UP ALIAS FOR SOURCE FILE
    char *name,                 // - alias name
    LINE_NO line,               // - line no. (used in #line)
    int adjust )                // - amount to adjust line no. before setting
{
    SRCFILE curr;               // - current entry
    SRCFILE alias;              // - alias entry

    name = FNameAdd( name );
    if( CompFlags.cpp_output ) {
        CppEmitPoundLine( line, name, EL_NULL );
    }
    line += adjust;
    alias = NULL;
    RingIterBeg( srcFile->sister, curr ) {
        if( name == curr->name ) {
            alias = curr;
            break;
        }
    } RingIterEnd( curr )
    if( alias == NULL ) {
        alias = CarveAlloc( carveSrcFile );
        *alias = *srcFile;
        alias->name = name;
        alias->parent = srcFile->parent;
        alias->sister = srcFile->sister;
        alias->alias = true;
        srcFile->sister = alias;
    }
    set_srcFile( alias );
    activeSrc()->line = line;
}

static void popSrcFile( SRCFILE srcfile, OPEN_FILE *act )
{
    RingPush( &freeFiles, act );
    srcfile->active = NULL;
    set_srcFile( srcfile->parent );
}

bool SrcFileClose(              // CLOSE A SOURCE FILE
    bool shutdown )             // - shutdown in progress
{
    SRCFILE tmp_src;            // - extra SRCFILE
    SRCFILE old_src;            // - SRCFILE being closed
    OPEN_FILE *act;             // - open-file information
    LINE_NO lines_read;         // - number of lines read from file
    bool ok;                    // - return: true ==> not EOF
    bool browsed;               // - true ==> file was browsed

    if( CompFlags.scanning_c_comment ) {
        SrcFileCurrentLocation();
        CErr1( ERR_INCOMPLETE_COMMENT );
    }
    old_src = srcFile;
    browsed = false;
    act = old_src->active;
    if( NULL != act->fp ) {
        SrcFileFClose( act->fp );
        act->fp = NULL;
        browsed = true;
    }
    {
        SRCFILE actual;

        actual = old_src;
        _FIND_ACTUAL( actual );
        switch( actual->guard_state ) {
        case GUARD_BOT:
            if( actual->ifndef_name != NULL ) {
                actual->guard_state = GUARD_IFNDEF;
            } else {
                actual->guard_state = GUARD_IF;
            }
            break;
        case GUARD_IF:
        case GUARD_INCLUDE:
            break;
        default:
            actual->guard_state = GUARD_INCLUDE;
        }
    }
    if( old_src->cmdline ) {
        popSrcFile( old_src, act );
        CurrChar = LCHR_EOF;
        ok = false;
    } else {
        lines_read = act->line - 1;
        if( old_src->parent == NULL ) {
            if( shutdown ) {
                popSrcFile( old_src, act );
                browsed = false;
            }
            SrcLineCount += lines_read;
            CurrChar = LCHR_EOF;
            ok = false;
        } else {
            popSrcFile( old_src, act );
            act = activeSrc();
            IncLineCount += lines_read;
            CurrChar = act->currc;
            if( CompFlags.cpp_output ) {
                if( IsSrcFilePrimary( srcFile )
                 && act->line == 0
                 && act->column == 0 ) {
                    // just completed -fi inclusion file
                    PpStartFile();
                    act->line = 0;
                }
                CppEmitPoundLine( old_src->parent_locn, srcFile->name, EL_NEW_LINE );
            }
            ok = true;
        }
    }
    if( browsed ) {
        BrinfCloseSource( old_src );
    }
    if( srcFile != NULL && srcFile->pch_create ) {
        // turn off before pchdr is created!
        tmp_src = srcFile->pch_child;
        srcFile->pch_child = NULL;
        srcFile->pch_create = false;
        SrcFileCurrentLocation();
        PCHeaderCreate( tmp_src->name );
    }
    return( ok );
}


char *SrcFileName(              // GET NAME OF SOURCE FILE
    SRCFILE sf )                // - source file
{
    char *name;                 // - name

    if( sf == NULL ) {
        if( srcFile == NULL ) {
            name = NULL;
        } else {
            name = srcFile->name;
        }
    } else {
        name = sf->name;
    }
    return( name );
}

char *SrcFileFullName(          // GET FULL PATH NAME OF SOURCE FILE
    SRCFILE sf )                // - source file
{
    char *p;
    char buff[_MAX_PATH];

    if( sf->full_name != NULL ) {
        return( sf->full_name );
    }
    p = IoSuppFullPath( SrcFileName( sf ), buff, _MAX_PATH );
    sf->full_name = FNameAdd( p );
    return( sf->full_name );
}


char *SrcFileNameCurrent(       // GET NAME OF CURRENT SOURCE FILE
    void )
{
    return( SrcFileName( srcFile ) );
}


LINE_NO SrcFileLine(            // GET CURRENT SOURCE LINE
    void )
{
    return( activeSrc()->line );
}


void SrcFileSetErrLoc(          // SET TEMPORARY ERROR LOCATION TO CURRENT POS
    void )
{
    OPEN_FILE *act;
    TOKEN_LOCN tmp_locn;

    act = activeSrc();
    tmp_locn.src_file = srcFile;
    tmp_locn.line = act->line;
    tmp_locn.column = act->column;
    SetErrLoc( &tmp_locn );
}


void SrcFileCurrentLocation(    // SET LOCATION FOR CURRENT SOURCE FILE
    void )
{
    OPEN_FILE *act;

    act = activeSrc();
    TokenLine = act->line;
    TokenColumn = act->column;
}


static bool srcReadBuffer(      // READ NEXT BUFFER
    bool close_top_file )       // - true ==> close top file
{
    OPEN_FILE *act;             // - open file
    SRCFILE src_file;           // - unaliased source file
    int amt_read;               // - amount read

    act = activeSrc();
    for(;;) {
        src_file = srcFile;
        _FIND_ACTUAL( src_file );
        if( src_file->uncached ) {
            act->fp = SrcFileFOpen( src_file->name, SFO_SOURCE_FILE );
            src_file->uncached = false;
            if( act->fp == NULL ) {
                CErr( ERR_IO_ERR, src_file->name, strerror( errno ) );
            } else {
                SysSeek( fileno( act->fp ), act->pos );
            }
        }
        if( act->fp != NULL ) {
            if( close_top_file ) {
                close_top_file = false;
            } else {
                if( src_file->force_include && act->nextc == notFilled ) {
                    InitialMacroFlags = MFLAG_NONE;
                }
                act->lastc = act->nextc = act->buff;
                if( src_file->found_eof ) {
                    src_file->found_eof = false;
                    amt_read = 0;
                    DbgAssert( !( SysRead( fileno( act->fp )
                                        , act->buff
                                        , PRODUCTION_BUFFER_SIZE ) > 0 ) );
                } else {
                    amt_read = SysRead( fileno( act->fp )
                                        , act->buff
                                        , PRODUCTION_BUFFER_SIZE );
                }
                if( amt_read > 0 ) {
                    if( amt_read < PRODUCTION_BUFFER_SIZE ) {
                        if( src_file->assume_file ) {
                            src_file->found_eof = true;
                        }
                    }
                    // mark end of buffer
                    act->buff[amt_read] = '\0';
                    act->lastc += amt_read;
                    /* CurrChar not set; must read buffer */
                    return( false );
                }
                if( amt_read == 0 ) {
                    // we're at the end of the file
                    if( src_file->pch_kludge ) {
                        // only do this once
                        src_file->pch_kludge = false;
                        if( src_file->assume_file ) {
                            // make sure next time we return EOF
                            src_file->found_eof = true;
                        }
                        // these three ';'s form the end of the pre-compiled
                        // header file so that the parser will be in a good
                        // state.  REWRITE.C caused a problem when a template
                        // was at the end of the header file and it tried to
                        // read one more token to make sure it had all of the
                        // tokens for the template definition.
                        // NYI: we should verify the parser is in a good state
                        // in case people try to pre-compile header files that
                        // split declarations across multiple files (rare)
                        act->buff[0] = ';';
                        act->buff[1] = ';';
                        act->buff[2] = ';';
                        act->buff[3] = '\0';
                        act->lastc += 3;
                        if( CurrChar != '\n' ) {
                            // terminate the last line (if necessary)
                            CurrChar = '\n';
                            return( true );
                        }
                        /* CurrChar not set; must read buffer */
                        return( false );
                    }
                    if( CurrChar != '\n' ) {
                        // file didn't end with a new-line
                        if( src_file->assume_file ) {
                            // make sure next time we return EOF
                            src_file->found_eof = true;
                        }
                        act->buff[0] = '\0';
                        CurrChar = '\n';
                        return( true );
                    }
                } else if( amt_read == -1 ) {
                    CErr( ERR_IO_ERR, src_file->name, strerror( errno ) );
                }
            }
        }
        if( ! SrcFileClose( false ) ) {
            /* CurrChar set to LCHR_EOF */
            return( true );
        }
        act = activeSrc();
        if( act->nextc < act->lastc ) {
            /* CurrChar set to ->lastc */
            return( true );
        }
        if( act->nextc == act->lastc ) {
            /* CurrChar not set; must read buffer ('\0' will be the char) */
            return( false );
        }
    }
}

static int getTestCharFromFile( OPEN_FILE **pact )
{
    OPEN_FILE *act;
    int c;

    DbgAssert( *pact == activeSrc() );  // NYI: is always true we can optimize
    for(;;) {
        act = activeSrc();
        c = *act->nextc++;
        if( c != '\0' ) {
            break;
        }
        // '\0' in the middle of the buffer must be processed as a char
        if( act->nextc != ( act->lastc + 1 ) )
            break;
        if( srcReadBuffer( getGuardState() == GUARD_IFNDEF ) ) {
            c = CurrChar;
            break;
        }
    }
    *pact = activeSrc();
    return( c );
}

static int getSecondMultiByte( void )
{
    int c;
    OPEN_FILE *act;

    act = activeSrc();
    c = getTestCharFromFile( &act );
    // should we do this for a multi-byte char?
    act->column++;
    NextChar = GetNextChar;
    CurrChar = c;
    return( c );
}

static int getCharCheck( OPEN_FILE *act, int c );

static int getCharAfterOneQuestion( void )
{
    int c;
    OPEN_FILE *act;

    // column for 'lastChar' has not been set yet
    act = activeSrc();
    NextChar = GetNextChar;
    c = lastChar;
    if( c == '?' ) {
        act->column++;
        CurrChar = c;
        return( c );
    }
    return( getCharCheck( act, c ) );
}

static int doFlushWS( void )
{
    char c;

    // columns for whitespace have not been set yet
    c = '\0';
    if( numBlanks1 != 0 ) {
        --numBlanks1;
        c = ' ';
    } else if( numTABs != 0 ) {
        --numTABs;
        c = '\t';
    } else if( numBlanks2 != 0 ) {
        --numBlanks2;
        c = ' ';
    }
    return( c );
}

static int flushWSgetCharAfterOneQuestion( void )
{
    int c;
    OPEN_FILE *act;

    c = doFlushWS();
    if( c != '\0' ) {
        act = activeSrc();
        return( getCharCheck( act, c ) );
    }
    return( getCharAfterOneQuestion() );
}

static int getCharAfterTwoQuestion( void )
{
    OPEN_FILE *act;

    act = activeSrc();
    act->column++;
    CurrChar = '?';
    NextChar = getCharAfterOneQuestion;
    return( CurrChar );
}

static void outputTrigraphWarning( char c )
{
    if( !CompFlags.extensions_enabled ) {
        // probably know about trigraphs if they are using -za
        return;
    }
    if( SkipLevel != NestLevel ) {
        // we are in an excluded #if region
        return;
    }
    if( CompFlags.scanning_c_comment ) {
        // do one cares about this in comments
        return;
    }
    if( CompFlags.scanning_cpp_comment ) {
        // do one cares about this in comments
        return;
    }
    SrcFileCurrentLocation();
    CErr2( WARN_EXPANDED_TRIGRAPH, c );
}

static int translateTriGraph( int c )
{
    const struct tri_graph *p;

    for( p = triGraphs; p->tri != '\0'; ++p ) {
        if( c == p->tri ) {
            outputTrigraphWarning( p->new );
            return( p->new );
        }
    }
    return( c );
}

static int tryTrigraphStart( void )
{
    int c;
    OPEN_FILE *act;

    act = activeSrc();
    c = doFlushWS();
    if( c != '\0' ) {
        return( getCharCheck( act, c ) );
    }
    NextChar = GetNextChar;
    return( getCharCheck( act, '?' ) );
}

static int tryBackSlashNewLine( OPEN_FILE *act )
{
    bool skipped_ws = false;
    int nc;

    // CurrChar is '\\' and act->column is up to date
    numBlanks1 = 0;
    numTABs = 0;
    numBlanks2 = 0;
    nc = getTestCharFromFile( &act );
    if( CompFlags.extensions_enabled ) {
        while( nc == ' ' ) {
            ++numBlanks1;
            skipped_ws = true;
            nc = getTestCharFromFile( &act );
        }
        while( nc == '\t' ) {
            skipped_ws = true;
            ++numTABs;
            nc = getTestCharFromFile( &act );
        }
        while( nc == ' ' ) {
            skipped_ws = true;
            ++numBlanks2;
            nc = getTestCharFromFile( &act );
        }
    }
    if( nc == '\r' ) {
        nc = getTestCharFromFile( &act );
    }
    switch( nc ) {
    case '\n':
        if( skipped_ws ) {
            SrcFileCurrentLocation();
            CErr1( WARN_WHITE_AFTER_SPLICE );
        }
        if( CompFlags.scanning_cpp_comment ) {
            if( SkipLevel == NestLevel ) {
                SrcFileCurrentLocation();
                CErr1( WARN_SPLICE_IN_CPP_COMMENT );
            }
        }
        if( CompFlags.cpp_output ) {
            if( CompFlags.in_pragma ) {
                PrtChar( '\\' );
                PrtChar( '\n' );
            } else if( CompFlags.cpp_line_wanted ) {
                PrtChar( '\n' );
            }
        }
        act->line++;
        act->column = 0;
        return( GetNextChar() );
    case '?':
        NextChar = tryTrigraphStart;
        break;
    default:
        lastChar = nc;
        NextChar = flushWSgetCharAfterOneQuestion;
    }
    return( '\\' );
}

static int tryTrigraphAgain( void )
{
    OPEN_FILE *act;
    int c;
    int xc;

    act = activeSrc();
    act->column++;
    c = getTestCharFromFile( &act );
    if( c != '?' ) {
        xc = translateTriGraph( c );
        if( c != xc ) {
            act->column += 2;
            CurrChar = xc;
            NextChar = GetNextChar;
            if( xc == '\\' ) {
                return( tryBackSlashNewLine( act ) );
            }
            return( xc );
        }
        lastChar = c;
        CurrChar = '?';
        NextChar = getCharAfterTwoQuestion;
        return( '?' );
    }
    CurrChar = c;
    /* leave NextChar set here because it could still be a trigraph */
    return( c );
}

static int getCharCheck( OPEN_FILE *act, int c )
{
    int xc;
    int nc;
    int nnc;

    // column has not being changed for 'c' yet
    if( c != '?' ) {
        switch( c ) {
        case '\0':
            // '\0' in the middle of the buffer must be processed as a char
            if( act->nextc == ( act->lastc + 1 ) ) {
                if( !srcReadBuffer( getGuardState() == GUARD_IFNDEF ) ) {
                    return( GetNextChar() );
                }
                return( CurrChar );
            }
            CurrChar = c;
            return( CurrChar );
        case '\n':
            act->line++;
            act->column = 0;
            break;
        case '\t':
            act->column = ( act->column + tabWidth ) & - tabWidth;
            break;
        case '\\':
            act->column++;
            CurrChar = c;
            return( tryBackSlashNewLine( act ) );
        case '\r':
            break;
        default:
            act->column++;
            if( c > 0x7f && CharSet[c] & C_DB ) {
                // we should not process the second byte through
                // normal channels since its value is meaningless
                // out of context
                NextChar = getSecondMultiByte;
            }
        }
        CurrChar = c;
        return( c );
    }
    /* we have one '?' */
    act->column++;
    nc = getTestCharFromFile( &act );
    if( nc != '?' ) {
        lastChar = nc;
        CurrChar = c;
        NextChar = getCharAfterOneQuestion;
        return( c );
    }
    /* we have two '?'s */
    nnc = getTestCharFromFile( &act );
    xc = translateTriGraph( nnc );
    if( nnc != xc ) {
        act->column += 2;
        CurrChar = xc;
        if( xc == '\\' ) {
            return( tryBackSlashNewLine( act ) );
        }
        return( xc );
    }
    lastChar = nnc;
    CurrChar = c;
    if( nnc == '?' ) {
        /* the next char after this may be a valid trigraph! */
        NextChar = tryTrigraphAgain;
        return( c );
    }
    NextChar = getCharAfterTwoQuestion;
    return( c );
}

static int regetUngotCharAfterOneQuestion( void )
{
    int c;

    // column for 'lastChar' has been set
    NextChar = GetNextChar;
    c = lastChar;
    CurrChar = c;
    return( c );
}

static int regetUngotCharAfterTwoQuestion( void )
{
    int c;

    NextChar = regetUngotCharAfterOneQuestion;
    c = '?';
    CurrChar = c;
    return( c );
}

static int restartDetectOneQuestion( void )
{
    int c;

    NextChar = regetUngotCharAfterOneQuestion;
    c = '?';
    CurrChar = c;
    return( c );
}

static int restartDetectTwoQuestion( void )
{
    int c;

    NextChar = regetUngotCharAfterTwoQuestion;
    c = '?';
    CurrChar = c;
    return( c );
}

static int restartBackSlashWhiteSpace( void )
{
    int c;

    NextChar = flushWSgetCharAfterOneQuestion;
    c = '\\';
    CurrChar = c;
    return( c );
}

static int restartBackSlashWhiteSpaceQuestion( void )
{
    int c;

    NextChar = tryTrigraphStart;
    c = '\\';
    CurrChar = c;
    return( c );
}

void GetNextCharUndo( int c )
{
    if( NextChar == GetNextChar ) {
        // will return 'lastChar' and reset back to 'GetNextChar'
        NextChar = regetUngotCharAfterOneQuestion;
        lastChar = c;
        return;
    }
    if( NextChar == getCharAfterOneQuestion ) {
        // lastChar already has character saved in it
        // c should be '?'
        DbgAssert( c == '?' );
        NextChar = restartDetectOneQuestion;
        return;
    }
    if( NextChar == getCharAfterTwoQuestion ) {
        // lastChar already has character saved in it
        // c should be '?'
        DbgAssert( c == '?' );
        NextChar = restartDetectTwoQuestion;
        return;
    }
    if( NextChar == flushWSgetCharAfterOneQuestion ) {
        // lastChar already has character saved in it
        // c should be '\\'
        DbgAssert( c == '\\' );
        NextChar = restartBackSlashWhiteSpace;
        return;
    }
    if( NextChar == tryTrigraphStart ) {
        // c should be '\\'
        // no need to set lastChar
        // tryTrigraphStart will eventually return '?'
        DbgAssert( c == '\\' );
        NextChar = restartBackSlashWhiteSpaceQuestion;
        return;
    }
    DbgStmt( CFatal( "more than one GetNextCharUndo executed" ) );
}

int GetNextChar( void )
{
    OPEN_FILE *act;
    int c;

    act = activeSrc();
    c = *act->nextc++;
    if(( CharSet[c] & C_EX ) == 0 ) {
        act->column++;
        CurrChar = c;
        return( c );
    }
    return( getCharCheck( act, c ) );
}

int SrcFileScanName( int c )    // CALLED FROM CSCAN TO SCAN AN IDENTIFIER
{
    const unsigned char *p;
    OPEN_FILE           *act;

    if( NextChar == GetNextChar ) {
        while( CharSet[c] & (C_AL | C_DI) ) {
            // codegen can't do this optimization so we have to
            act = activeSrc();
            p = act->nextc;
            while( CharSet[c] & (C_AL | C_DI) ) {
                WriteBufferChar( c );
                c = *p++;
            }
            act->column += p - act->nextc;
            act->nextc = p;
            if(( CharSet[c] & C_EX ) == 0 )
                break;
            // act->column is one too many at this point
            --act->column;
            c = getCharCheck( act, c );
        }
        CurrChar = c;
    } else {
        // it should be impossible to get here
        // but we'll just be safe rather than sorry...
        while( CharSet[c] & (C_AL | C_DI) ) {
            WriteBufferChar( c );
            c = NextChar();
        }
    }
    WriteBufferNullChar();
    return( c );
}

void SrcFileScanWhiteSpace( bool expanding )
{
    const unsigned char *p;
    OPEN_FILE           *act;
    int                 c;

    /* unused parameters */ (void)expanding;

    if( NextChar == GetNextChar ) {
        for( ;; ) {
            // codegen can't do this optimization so we have to
            c = '\0';
            act = activeSrc();
            p = act->nextc;
            for( ;; ) {
                c = *p++;
                if( CharSet[c] != C_WS )
                    break;
                c = *p++;
                if( CharSet[c] != C_WS )
                    break;
                c = *p++;
                if( CharSet[c] != C_WS )
                    break;
                c = *p++;
                if( CharSet[c] != C_WS )
                    break;
                c = *p++;
                if( CharSet[c] != C_WS )
                    break;
                c = *p++;
                if( CharSet[c] != C_WS )
                    break;
                c = *p++;
                if( CharSet[c] != C_WS )
                    break;
                c = *p++;
                if( CharSet[c] != C_WS ) {
                    break;
                }
            }
            act->column += p - act->nextc;
            act->nextc = p;
            if(( CharSet[c] & C_EX ) == 0 )
                break;
            if( c == '\n' ) {
                act->line++;
                act->column = 0;
                break;
            }
            // act->column is one too many at this point
            if( c == '\t' ) {
                act->column = (( act->column - 1 ) + tabWidth ) & - tabWidth;
            } else if( c != '\r' ) {
                --act->column;
                c = getCharCheck( act, c );
                if(( CharSet[c] & C_WS ) == 0 ) {
                    break;
                }
            }
        }
        CurrChar = c;
    } else {
        for( ;; ) {
            c = NextChar();
            if(( CharSet[c] & C_WS ) == 0 )
                break;
            c = NextChar();
            if(( CharSet[c] & C_WS ) == 0 ) {
                break;
            }
        }
    }
}


bool IsSrcFilePrimary(          // DETERMINE IF PRIMARY SOURCE FILE
    SRCFILE sf )                // - a source file
{
    return(( sf != NULL ) && sf->primary );
}


void SetSrcFileLibrary(         // MARK CURRENT SOURCE FILE AS A LIBRARY FILE
    void )
{
    srcFile->lib_inc = true;
}


SRCFILE SrcFileEnclosingPrimary(// FIND ENCLOSING PRIMARY SOURCE FILE
    SRCFILE src )               // - a source file
{
    SRCFILE primary;

    DbgAssert( src != NULL );
    primary = src;
    while( src != NULL ) {
        if( src->primary ) {
            primary = src;
        }
        src = src->parent;
    }
    DbgAssert( primary != NULL );
    _FIND_ACTUAL( primary );
    return( primary );
}


void SetSrcFilePrimary(         // MARK CURRENT SOURCE FILE AS THE PRIMARY FILE
    void )
{
    srcFile->primary = true;
    DbgAssert( primarySrcFile == NULL );
    primarySrcFile = srcFile;
}

SRCFILE SrcFileGetPrimary(      // GET PRIMARY SOURCE FILE
    void )
{
    DbgAssert( primarySrcFile != NULL );
    return( primarySrcFile );
}


void SetSrcFileCommand(         // MARK CURRENT SOURCE FILE AS A COMMAND FILE
    void )
{
    srcFile->cmdline   = true;
    srcFile->read_only = true;  // To exclude file from dependency list
}


bool IsSrcFileLibrary(          // DETERMINE IF SOURCE FILE IS #include <file.h>
    SRCFILE sf )                // - a source file
{
    return(( sf != NULL ) && sf->lib_inc );
}


bool IsSrcFileCmdLine(          // DETERMINE IF SOURCE FILE IS FOR CMD-LINE
    SRCFILE sf )                // - a source file
{
    return( sf != NULL && sf->cmdline );
}


bool SrcFilesOpen(              // DETERMINE IF ANY SOURCE FILES OPEN
    void )
{
    return( srcFile != NULL && !srcFile->cmdline );
}


void SrcFileGetTokenLocn(       // FILL IN TOKEN_LOCN FROM CURRENCY
    TOKEN_LOCN *tgt )           // - to be filled in
{
    tgt->src_file = srcFile;
    tgt->line = TokenLine;
    tgt->column = TokenColumn;
}

void SrcFileSetTokenLocn(       // RESET TOKEN_LOCN
    TOKEN_LOCN *tgt )           // - from SrcFileGetTokenLocn
{
    set_srcFile( tgt->src_file );
    TokenLine = tgt->line;
    TokenColumn = tgt->column;
}

bool SrcFileAreTLSameLine(      // CHECK WHETHER TOKEN_LOCNs ARE THE SAME LINE
    TOKEN_LOCN *l1,             // - location one
    TOKEN_LOCN *l2 )            // - location two
{
    if( ! SrcFileSame( l1->src_file, l2->src_file ) ) {
        return( false );
    }
    if( l1->line != l2->line ) {
        return( false );
    }
    return( true );
}


SRCFILE SrcFileCurrent(         // GET CURRENT SRCFILE
    void )
{
    return( srcFile );
}


void SrcFilePoint(              // SET CURRENT SRCFILE
    SRCFILE srcfile )           // - source file to be set as current
{
    set_srcFile( srcfile );
}


time_t SrcFileTimeStamp(       // GET TIME STAMP FOR FILE
    SRCFILE srcfile )           // - source file
{
    return( srcfile->time_stamp );
}


SRCFILE SrcFileWalkInit(        // START WALK OF SOURCE FILES
    void )
{
    return( srcFilesUnique );
}


SRCFILE SrcFileWalkNext(        // NEXT FILE IN WALK OF SOURCE FILES
    SRCFILE curr )              // - previous file
{
    return( curr->unique );
}

SRCFILE SrcFileNotReadOnly(     // GET NEXT NON-READ-ONLY SOURCE FILE
    SRCFILE curr )              // - current source file
{
    DIR_LIST* srch;             // - search R/O entry
    bool read_only;             // - true ==> file is in read-only directory
    char const *file_name;      // - file name of current entry

    for( ; curr != NULL; curr = curr->unique ) {
        if( curr->read_only )
            continue;
        read_only = false;
        file_name = SrcFileFullName( curr );
        RingIterBeg( roDirs, srch ) {
            if( 0 == strnicmp( srch->name, file_name, strlen( srch->name ) ) ) {
                read_only = true;
                break;
            }
        } RingIterEnd( srch );
        if( !read_only ) {
            break;
        }
    }
    return( curr );
}

static void addRoDir( const char *dirname )
{
    DIR_LIST* curr;             // - new R/O dir entry

    curr = RingCarveAlloc( carveRoDir, &roDirs );
    curr->name = FNameAdd( dirname );
}

void SrcFileReadOnlyDir(        // SPECIFY DIRECTORY AS READ-ONLY
    char const *path_list )     // - the directory
{
    char *full;                 // - full path
    char path[_MAX_PATH];       // - used to extract directory
    char buff[_MAX_PATH];       // - expanded path for directory
    DIR_LIST* curr;             // - current R/O entry
    DIR_LIST* srch;             // - search R/O entry

    while( *path_list != '\0' ) {
        char *p = path;
        path_list = GetPathElement( path_list, NULL, &p );
        *p = '\0';
        full = IoSuppFullPath( path, buff, sizeof( buff ) );
        curr = NULL;
        RingIterBeg( roDirs, srch ) {
            if( 0 == stricmp( full, srch->name ) ) {
                curr = srch;
                break;
            }
        } RingIterEnd( srch );
        if( curr == NULL ) {
            addRoDir( full );
        }
    }
}


void SrcFileReadOnlyFile(       // SPECIFY FILE AS READ-ONLY
    char const *file )          // - the file
{
    SRCFILE srcfile;            // - current source file

    if( NULL == file ) {
        srcfile = srcFile;
    } else {
        srcfile = srcFileGetUnique( file );
    }
    if( NULL != srcfile ) {
        srcfile->read_only = true;
    }
}

SRCFILE SrcFileIncluded(        // FILE THAT INCLUDES THIS FILE
    SRCFILE curr,               // - current file
    LINE_NO *line )             // - line that contains #include
{
    SRCFILE parent;

    *line = 0;
    parent = curr->parent;
    if( parent != NULL ) {
        *line = curr->parent_locn;
    }
    return( parent );
}


static void srcFileGuardReject( // SIGNAL REJECTION OF GUARDED FILE
    void )
{
    setGuardState( GUARD_INCLUDE );
}


void SrcFileGuardPpIf(          // #IF DETECTED IN SOURCE FILE
    void )
{
    unsigned guard_state = getGuardState();

    if( ( guard_state != GUARD_INCLUDE )
      &&( guard_state != GUARD_IF )
      &&( 0 == IfDepthInSrcFile() ) ) {
        if( guard_state == GUARD_TOP ) {
            setGuardState( GUARD_MID );
        } else {
            srcFileGuardReject();
        }
    }
}


void SrcFileGuardPpIfndef(      // SUPPLY #IFNDEF NAME
    const char *name,           // - macro name
    size_t len )                // - length of name
{
    SRCFILE src;

    src = srcFile;
    _FIND_ACTUAL( src );
    if( src->ifndef_name == NULL ) {
        /* only need the first #ifndef in the file */
        src->ifndef_len = len;
        src->ifndef_name = CPermAlloc( len + 1 );
        strcpy( src->ifndef_name, name );
    }
}


void SrcFileGuardPpElse(        // #ELSE DETECTED IN SOURCE FILE
    void )
{
    if( IfDepthInSrcFile() <= 1 ) {
        srcFileGuardReject();
    }
}


void SrcFileGuardPpEndif(       // #ENDIF DETECTED IN SOURCE FILE
    void )
{
    if( ( 0 == IfDepthInSrcFile() ) &&( getGuardState() == GUARD_MID ) ) {
        setGuardState( GUARD_BOT );
    }
}


void SrcFileGuardStateSig(      // SIGNAL SIGNIFICANCE (TOKEN, ETC) IN FILE
    void )
{
    SRCFILE src_file;

    /* NYI: needs to be able to handle a NULL srcFile! */
    src_file = srcFile;
    _FIND_ACTUAL( src_file );
    if( src_file->guard_state != GUARD_MID ) {
        src_file->guard_state = GUARD_INCLUDE;
    }
}


bool SrcFileGuardedIf(          // SKIP REST OF GUARDED FILE, IF POSSIBLE
    bool value )                // - <value> in #if <value>
{
    if( getGuardState() == GUARD_IF ) {
        if( !value ) {
            if( !srcReadBuffer( true ) ) {
                GetNextChar();
            }
            return( true );
        }
        setGuardState( GUARD_INCLUDE );
    }
    return( false );
}


bool SrcFileProcessOnce(        // CHECK WHETHER WE HAVE TO OPEN THE FILE
    const char *name )
{
    SRCFILE old;                // - existing SRCFILE

    old = srcFileGetUnique( name );
    if( old != NULL ) {
        if( old->once_only ) {
            return( true );
        }
        if( old->guard_state == GUARD_IFNDEF ) {
            if( MacroExists( old->ifndef_name, old->ifndef_len ) ) {
                return( true );
            }
        }
    }
    return( false );
}


//---------------------------------------------------------------------------
// The following routines permit the standard SrcFile interface to be
// employed while scanning characters from a command-line buffer.
//---------------------------------------------------------------------------


void SrcFileCmdLnDummyOpen(     // OPEN DUMMY FILE FOR COMMAND LINE
    void )
{
    SRCFILE cmd_file;           // - command file

    cmd_file = srcFileAlloc( NULL, NULL, 0 );
    set_srcFile( cmd_file );
    setGuardState( GUARD_IFNDEF );
}


void SrcFileCmdLnDummyClose(    // CLOSE DUMMY FILE FOR COMMAND LINE
    void )
{
    SRCFILE cmd_file;           // - command file

    cmd_file = srcFile;
    SrcFileClose( false );
//  CarveFree( carveSrcFile, cmd_file );
}


int SrcFileCmdLnGetChar(        // GET NEXT CHARACTER FOR CMD-LINE FILE
    void )
{
    int return_eol;             // - true => return end of line
    int next_char;              // - next character
    SRCFILE sf;                 // - source file for command line

    sf = srcFile;
    if( sf->cmdlneof ) {
        next_char = LCHR_EOF;
    } else if( sf->cmdlneol ) {
        next_char = '\n';
        sf->cmdlneof = true;
    } else {
        if( sf->ignore_swend ) {
            return_eol = CmdScanBufferEnd();
        } else {
            return_eol = CmdScanSwEnd();
        }
        if( return_eol ) {
            sf->cmdlneol = true;
            next_char = '\r';
        } else {
            next_char = CmdScanChar();
        }
    }
    CurrChar = next_char;
    return( next_char );
}


void SrcFileTraceBack(          // INDICATE SRCFILE USED IN TRACE-BACK
    SRCFILE sf )                // - source-file in message
{
    traced_back = sf;
}


bool SrcFileTraceBackReqd(      // DETERMINE IF MSG TRACE-BACK REQ'D
    SRCFILE sf )                // - source-file in message
{
    return( ( sf != NULL ) && ( sf != traced_back ) );
}


SRCFILE SrcFileTraceBackFile(   // GET SRCFILE TRACED BACK
    void )
{
    return( traced_back );
}

bool SrcFileSame(               // ARE THESE SRC FILES THE SAME FILE?
    SRCFILE f1,                 // - src-file 1
    SRCFILE f2 )                // - src-file 2
{
    _FIND_ACTUAL( f1 );
    _FIND_ACTUAL( f2 );
    return( f1 == f2 );
}


unsigned SrcFileIndex(          // GET INDEX OF THIS SRCFILE
    SRCFILE sf )                // - the source file
{
    return( sf->index );
}


static bool srcFileCacheClose( bool close_all_ok )
{
    SRCFILE curr;
    SRCFILE last;
    SRCFILE multiple_cached;
    OPEN_FILE *uncache;
    OPEN_FILE *active;

    multiple_cached = NULL;
    last = NULL;
    for( curr = srcFile; curr != NULL; curr = curr->parent ) {
        if( curr->uncached ) {
            continue;
        }
        active = curr->active;
        if( active == NULL ) {
            continue;
        }
        if( active->fp == stdin ) {
            continue;
        }
        multiple_cached = last;
        last = curr;
    }
    if( last == NULL ) {
        return( false );
    }
    if( multiple_cached == NULL && !close_all_ok ) {
        return( false );
    }
    _FIND_ACTUAL( last );
    last->uncached = true;
    uncache = last->active;
    uncache->pos = SysTell( fileno( uncache->fp ) );
    SrcFileFClose( uncache->fp );
    uncache->fp = NULL;
    return( true );
}


static bool recursiveIncludeDetected( const char *name )
{
    SRCFILE curr;

    for( curr = srcFile; curr != NULL; curr = curr->parent ) {
        if( ! MacroStateMatchesCurrent( &(curr->macro_state) ) ) {
            /* any open before this will never match */
            break;
        }
        if( strcmp( name, curr->name ) == 0 ) {
            CErr2p( ERR_RECURSIVE_FILE_INCLUDE, name );
            return( true );
        }
    }
    return( false );
}


FILE *SrcFileFOpen( const char *name, src_file_open kind )
{
    FILE *fp;
    const char *mode;
    static const char *file_kind[] = { "rb", "r", "rb", "w", "wb" };

    mode = file_kind[kind];
    for(;;) {
        fp = fopen( name, mode );
        if( fp != NULL ) {
            if( recursiveIncludeDetected( name ) ) {
                fclose( fp );
                fp = NULL;
            }
            break;
        }
        if( errno != ENOMEM && errno != ENFILE && errno != EMFILE )
            break;
        if( ! srcFileCacheClose( kind == SFO_SOURCE_FILE ) ) {
            break;
        }
    }
    return( fp );
}

int SrcFileFClose( FILE *fp )
{
    if( fp == stdin )
        return( 0 );
    return( fclose( fp ) );
}

static void markFreeSrcFile( void *p )
{
    SRCFILE s = p;

    s->free = true;
}

static void saveSrcFile( void *e, carve_walk_base *d )
{
    SRCFILE sister_save;
    SRCFILE parent_save;
    SRCFILE unique_save;
    OPEN_FILE *active_save;
    char *name_save;
    char *ifndef_save;
    size_t name_len;
    size_t ifndef_len;
    SRCFILE s = e;

    if( s->free ) {
        return;
    }
    sister_save = s->sister;
    s->sister = SrcFileGetIndex( sister_save );
    parent_save = s->parent;
    s->parent = SrcFileGetIndex( parent_save );
    unique_save = s->unique;
    s->unique = SrcFileGetIndex( unique_save );
    active_save = s->active;
    s->active = NULL;
    name_save = s->name;
    ifndef_save = s->ifndef_name;
    name_len = 0;
    if( name_save != NULL ) {
        name_len = strlen( name_save ) + 1;
    }
    s->name = PCHSetUInt( name_len );
    ifndef_len = 0;
    if( ifndef_save != NULL ) {
        ifndef_len = s->ifndef_len + 1;
    }
    s->ifndef_name = PCHSetUInt( ifndef_len );
    PCHWriteCVIndex( d->index );
    PCHWriteVar( *s );
    if( name_len > 0 ) {
        PCHWrite( name_save, name_len );
    }
    if( ifndef_len > 0 ) {
        PCHWrite( ifndef_save, ifndef_len );
    }
    s->sister = sister_save;
    s->parent = parent_save;
    s->unique = unique_save;
    s->active = active_save;
    s->name = name_save;
    s->ifndef_name = ifndef_save;
}

static void writeRoDirs( void )
{
    DIR_LIST *curr;
    unsigned len;

    RingIterBeg( roDirs, curr ) {
        len = strlen( curr->name ) + 1;
        PCHWriteUInt( len );
        PCHWrite( curr->name, len );
    } RingIterEnd( curr );
    PCHWriteUInt( 0 );
}

pch_status PCHWriteSrcFiles( void )
{
    carve_walk_base dsrc;

    SrcFilePCHWrite( srcFilesUnique );
    CarveWalkAllFree( carveSrcFile, markFreeSrcFile );
    CarveWalkAll( carveSrcFile, saveSrcFile, &dsrc );
    PCHWriteCVIndexTerm();
    writeRoDirs();
    return( PCHCB_OK );
}

static void readRoDirs( void )
{
    unsigned len;
    char buff[_MAX_PATH];

    for( ; (len = PCHReadUInt()) != 0; ) {
        PCHRead( buff, len );
        addRoDir( buff );
    }
}

pch_status PCHReadSrcFiles( void )
{
    SRCFILE unique_srcfiles;
    char *buff;
    size_t buff_len;
    SRCFILE s;
    SRCFILE n;
    size_t name_len;
    size_t ifndef_len;
    cvinit_t data;

    // primarySrcFile will already be set properly so it is unaffected by PCH
    unique_srcfiles = SrcFilePCHRead();
    buff_len = 80;
    buff = CMemAlloc( buff_len );
    CarveInitStart( carveSrcFile, &data );
    for( ; (s = PCHReadCVIndexElement( &data )) != NULL; ) {
        PCHReadVar( *s );
        s->sister = SrcFileMapIndex( s->sister );
        s->parent = SrcFileMapIndex( s->parent );
        s->unique = SrcFileMapIndex( s->unique );
        s->active = NULL;
        s->full_name = NULL;
        MacroStateClear( &(s->macro_state) );
        name_len = PCHGetUInt( s->name );
        if( name_len != 0 ) {
            if( name_len > buff_len ) {
                CMemFree( buff );
                buff_len = name_len;
                buff = CMemAlloc( buff_len );
            }
            PCHRead( buff, name_len );
            s->name = FNameAdd( buff );
        } else {
            s->name = NULL;
        }
        ifndef_len = PCHGetUInt( s->ifndef_name );
        if( ifndef_len > 0 ) {
            if( ifndef_len > buff_len ) {
                CMemFree( buff );
                buff_len = ifndef_len;
                buff = CMemAlloc( buff_len );
            }
            PCHRead( buff, ifndef_len );
            s->ifndef_name = strpermsave( buff );
        } else {
            s->ifndef_name = NULL;
        }
    }
    for( s = unique_srcfiles; s != NULL; s = n ) {
        n = s->unique;
        if( ! IsSrcFilePrimary( s ) ) {
            srcFileAddUnique( s );
        }
    }
    CMemFree( buff );
    readRoDirs();
    return( PCHCB_OK );
}

SRCFILE SrcFileGetIndex( SRCFILE e )
{
    return( CarveGetIndex( carveSrcFile, e ) );
}

SRCFILE SrcFileMapIndex( SRCFILE e )
{
    return( CarveMapIndex( carveSrcFile, e ) );
}

pch_status PCHInitSrcFiles( bool writing )
{
    if( writing ) {
        PCHWriteCVIndex( CarveLastValidIndex( carveSrcFile ) );
    } else {
        alternateCarveSrcFile = carveSrcFile;
        carveSrcFile = CarveCreate( sizeof( *srcFile ), BLOCK_SRCFILE );
        CarveMapOptimize( carveSrcFile, PCHReadCVIndex() );
    }
    return( PCHCB_OK );
}

pch_status PCHFiniSrcFiles( bool writing )
{
    if( !writing ) {
        carve_t tmp;

        tmp = carveSrcFile;
        carveSrcFile = alternateCarveSrcFile;
        CarveMapUnoptimize( tmp );
        alternateCarveSrcFile = tmp;
    }
    return( PCHCB_OK );
}

void SrcFileSetTab( unsigned tab )
{
    switch( tab ) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
        tabWidth = tab;
        break;
    }
}

void SrcFileOnceOnly(           // CURRENT SRCFILE CAN BE SKIPPED IF #INCLUDE AGAIN
    void )
{
    SRCFILE srcfile;            // - current source file

    srcfile = srcFile;
    if( srcfile != NULL ) {
        srcfile->once_only = true;
    }
}

void SrcFileSetSwEnd(           // SET CURRENT SRCFILE IGNORE CMDLINE SW END STATUS
    bool val )                  // - value to use to set status
{
    SRCFILE srcfile;            // - current source file

    srcfile = srcFile;
    if( srcfile != NULL && srcfile->cmdline ) {
        if( val ) {
            srcfile->ignore_swend = true;
        } else {
            srcfile->ignore_swend = false;
        }
    }
}
