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
* Description:  Parsing of .hpj files.
*
****************************************************************************/


#include <stdlib.h>
#include <ctype.h>
#ifdef __UNIX__
#include <dirent.h>
#include <unistd.h>
#else
#include <direct.h>
#endif
#include "hpjread.h"
#include "hcerrors.h"
#include "parsing.h"
#include "topic.h"
#include "phrase.h"
#include "bmx.h"
#include "pathgrp2.h"

#include "clibext.h"


#define NB_FILES    10
#define LINE_BLOCK  128


// Other string resources.
static char const   SBaggage[]      = "BAGGAGE";
static char const   SOptions[]      = "OPTIONS";
static char const   SConfig[]       = "CONFIG";
static char const   SFiles[]        = "FILES";
static char const   SMap[]          = "MAP";
static char const   SBitmaps[]      = "BITMAPS";
static char const   SWindows[]      = "WINDOWS";
static char const   STitle[]        = "TITLE";
static char const   SCopyright[]    = "COPYRIGHT";
static char const   SCompress[]     = "COMPRESS";
static char const   STrue[]         = "TRUE";
static char const   SHigh[]         = "HIGH";
static char const   SMedium[]       = "MEDIUM";
static char const   SYes[]          = "YES";
static char const   SFalse[]        = "FALSE";
static char const   SLow[]          = "LOW";
static char const   SNo[]           = "NO";
static char const   SOldKeyPhrase[] = "OLDKEYPHRASE";
static char const   SContents[]     = "CONTENTS";
static char const   SIndex[]        = "INDEX";
static char const   SBmRoot[]       = "BMROOT";
static char const   SRoot[]         = "ROOT";
static char const   Sinclude[]      = "#include";
static char const   Sdefine[]       = "#define";
static char const   Smain[]         = "main";
static char const   Ssecondary[]    = "secondary";
static char const   SstartComment[] = "/*";
static char const   SendComment[]   = "*/";



// Check line size.

void HPJScanner::chkLineSize( size_t size )
{
    if( size == _curLine.len() ) {
        _curLine.resize( size + LINE_BLOCK );
    }
}

//  C-tor and D-tor for class HPJScanner

HPJScanner::HPJScanner( InFile *src )
    : _input( src ), _curLine( 0 ), _lineSize( 0 )
{
    _lineNum = 0;
    if( !_input->bad() ) {
        chkLineSize( _lineSize );
    }
}

//  HPJScanner::open    --Initialize the parser on a given filename.

bool HPJScanner::open( char const filename[] )
{
    bool result = _input->open( filename );
    if( !result ) {
        if( _lineSize == 0 ) {
            chkLineSize( _lineSize );
        }
    }
    return result;
}


//  HPJScanner::getLine --Read a single line of text into _curLine.
//                         Returns the line length (0 in case of failure).

size_t HPJScanner::getLine()
{
    int     current;
    bool    has_text;
    size_t  cur_len=0;

    // Loop until we've identified a single line.
    while( cur_len == 0 ) {
        ++_lineNum;
        current = 0;
        cur_len = 0;
        has_text = false;
        for( ;; ) {
            current = _input->nextch();
            if( current == ';' && cur_len == 0 )
                break;
            if( current == EOF || current == '\n' )
                break;
            if( !isspace( current ) ) {
                has_text = true;
            }
            chkLineSize( cur_len );
            _curLine[cur_len++] = (char)current;
        }
        if( current == ';' && cur_len == 0 ) {
            do {
                current = _input->nextch();
            } while( current != EOF && current != '\n' );
            if( cur_len == 0 ) {
                continue;
            }
        }
        if( current != EOF || cur_len > 0 ) {
            chkLineSize( cur_len );
            _curLine[cur_len++] = '\0';
        } else {
            break;
        }
        if( !has_text ) {
            cur_len = 0;
        }
    }

    // Set up the buffer so this line can be tokenized.
    if( cur_len > 0 ) {
        _bufPos = 0;
        _bufChar = _curLine[0];
    }
    return cur_len;
}

//  HPJScanner::getArg  --Read a "= <string>" argument from the .HPJ file.

char *HPJScanner::getArg( size_t start_pos )
{
    char    *arg;

    arg = _curLine + start_pos;
    // Eat whitespace.
    while( isspace( *arg ) )
        arg++;

    // The next character had better be an '='.
    if( *arg++ != '=' ) {
        HCWarning( HPJ_NOARG, _lineNum, name() );
        return NULL;
    }

    // Eat whitespace.
    while( isspace( *arg ) )
        arg++;

    return arg;
}


//  HPJScanner::tokLine --Tokenize a line, like the strtok() function.

char *HPJScanner::tokLine()
{
    size_t i,j;
    _curLine[_bufPos] = _bufChar;

    i = _bufPos;
    // Eat whitespace.
    while( isspace( _curLine[i] ) )
        i++;

    if( _curLine[i] == '\0' )
        return NULL;

    // Find the end of the token.
    for( j = i; _curLine[j] != '\0'; ++j ) {
        if( isspace( _curLine[j] ) ) {
            break;
        }
    }

    _bufPos = j;
    _bufChar = _curLine[j];
    _curLine[j] = '\0';
    return _curLine + i;
}


//  HPJScanner::endTok  --Stop tokenizing and get the rest of the line.

char *HPJScanner::endTok()
{
    _curLine[_bufPos] = _bufChar;
    return _curLine + _bufPos;
}


HPJReader::StrNode  *HPJReader::_topFile    = NULL;
HPJReader::StrNode  *HPJReader::_curFile    = NULL;
HPJReader::StrNode  *HPJReader::_firstDir   = NULL;
char const          *HPJReader::_startDir   = NULL;


//  HPJReader::HPJReader

HPJReader::HPJReader( HFSDirectory * d_file, Pointers *other_files,
              InFile *input )
    : _scanner( input )
{
    _dir = d_file;
    _theFiles = other_files;
    _sysFile = other_files->_sysFile;
    _numBagFiles = 0;
    _rtfFiles = NULL;
    _root = NULL;
    _homeDir = new char[_MAX_PATH];
    if( getcwd( _homeDir, _MAX_PATH ) == NULL ) {
        _bagFiles = NULL;
    } else if( input->bad() ) {
        _bagFiles = NULL;
    } else {
        _bagFiles = new Baggage*[NB_FILES];
    }
    _oldPhrases = false;
}


//  HPJReader::~HPJReader

HPJReader::~HPJReader()
{
    delete[] _homeDir;
    StrNode *current, *next;
    for( current = _root; current != NULL; current = next ) {
        next = current->_next;
        delete[] current->_name;
        delete current;
    }
    for( current = _rtfFiles; current != NULL; current = next ) {
        next = current->_next;
        delete[] current->_name;
        delete current;
    }

    if( _bagFiles != NULL ) {
        for( int i = 0; i < _numBagFiles; i++ ) {
            delete _bagFiles[i];
        }
        delete[] _bagFiles;
    }
}


//  HPJReader::firstFile    --Callback function for the phrase handler.

bool HPJReader::firstFile( InFile *input )
{
    _curFile = _topFile;
    return nextFile( input );
}


//  HPJReader::nextFile --Callback function for the phrase handler.

bool HPJReader::nextFile( InFile *input )
{
    StrNode         *curdir;
    char            *filename;

    while( _curFile != NULL ) {
        filename = _curFile->_name;
        _curFile = _curFile->_next;
        if( _firstDir == NULL ) {
            input->open( filename );
            if( !input->bad() ) {
                return true;
            }
        } else {
            for( curdir = _firstDir; curdir != NULL; curdir = curdir->_next ) {
                if( chdir( curdir->_name ) )
                    continue;
                input->open( filename );
                if( chdir( _startDir ) )
                    HCWarning( HPJ_BADDIR, _startDir );
                if( !input->bad() ) {
                    return true;
                }
            }
        }
    }
    return false;
}


//  HPJReader::parseFile --Parse an .HPJ file, and call .RTF parsers
//                          as appropriate.

void HPJReader::parseFile()
{
    HCStartFile( _scanner.name() );

    size_t  length;
    char    section[15];
    size_t  i;

    length = _scanner.getLine();    // Get the first line.
    while( length != 0 ) {
        // The first line had better be the beginning of a section.
        if( _scanner[0] != '[' ) {
            HCWarning( HPJ_NOTSECTION, _scanner.lineNum(), _scanner.name() );
            length = skipSection();
            continue;
        }

        // Read in the name of the section.
        for( i=1; i < length ; i++ ) {
            if( _scanner[i] == ']' ) break;
            section[i - 1] = (char)toupper( _scanner[i] );
        }

        // If the section name wasn't terminated properly, skip the section.
        if( i == length ) {
            HCWarning( HPJ_BADSECTION, _scanner.lineNum(), _scanner.name() );
            length = skipSection();
            continue;
        }
        section[i - 1] = '\0';

        // Pass control to the appropriate "section handler".
        if( strcmp( section, SBaggage ) == 0 ) {
            length = handleBaggage();
        } else if( strcmp( section, SOptions ) == 0 ) {
            length = handleOptions();
        } else if( strcmp( section, SConfig ) == 0 ) {
            length = handleConfig();
        } else if( strcmp( section, SFiles ) == 0 ) {
            length = handleFiles();
        } else if( strcmp( section, SMap ) == 0 ) {
            length = handleMap();
        } else if( strcmp( section, SBitmaps ) == 0 ) {
            length = handleBitmaps();
        } else if( strcmp( section, SWindows ) == 0 ) {
            length = handleWindows();
        } else {
            HCWarning( HPJ_BADSECTION, _scanner.lineNum(), _scanner.name() );
            length = skipSection();
        }
    }

    if( _rtfFiles == NULL ) {
        HCError( HPJ_NOFILES );
    }

    // Now parse individual RTF files.
    // First, implement phrase replacement if desired.
    if( _theFiles->_sysFile->isCompressed() ) {
        _topFile = _rtfFiles;
        _firstDir = _root;
        _startDir = _homeDir;

        _theFiles->_phrFile = new HFPhrases( _dir, &firstFile, &nextFile );

        char        full_path[_MAX_PATH];
        pgroup2     pg;

        _fullpath( full_path, _scanner.name(), _MAX_PATH );
        _splitpath2( full_path, pg.buffer, &pg.drive, &pg.dir, &pg.fname, NULL );
        _makepath( full_path, pg.drive, pg.dir, pg.fname, PH_EXT );

        if( !_oldPhrases || !_theFiles->_phrFile->oldTable( full_path ) ) {
            _theFiles->_phrFile->readPhrases();
            _theFiles->_phrFile->createQueue( full_path );
        }
    }

    _theFiles->_topFile = new HFTopic( _dir, _theFiles->_phrFile );

    // For each file, search the ROOT path, and create a RTFparser
    // to deal with it.
    for( StrNode *curfile = _rtfFiles; curfile != NULL; curfile = curfile->_next ) {
        InFile  source;
        if( _root == NULL ) {
            source.open( curfile->_name );
        } else {
            for( StrNode *curdir = _root; curdir != NULL; curdir = curdir->_next ) {
                if(chdir( curdir->_name ) )
                    continue;
                source.open( curfile->_name );
                if( chdir( _homeDir ) )
                    HCWarning( HPJ_BADDIR, _homeDir );
                if( !source.bad() ) {
                    break;
                }
            }
        }
        if( source.bad() ) {
            HCWarning( FILE_ERR, curfile->_name );
        } else {
            RTFparser rtfhandler( _theFiles, &source );
            rtfhandler.Go();
            source.close();
        }
    }
}


//  HPJReader::skipSection --Jump to the next section header.

size_t HPJReader::skipSection()
{
    size_t result;
    do {
        result = _scanner.getLine();
    } while( result != 0 && _scanner[0] != '[' );
    return result;
}


//  HPJReader::handleBaggage --Create baggage files.

size_t HPJReader::handleBaggage()
{
    size_t result = 0;

    while( _numBagFiles < NB_FILES ) {
        result = _scanner.getLine();
        if( result == 0 )
            break;
        if( _scanner[0] == '[' )
            break;
        _bagFiles[_numBagFiles++] = new Baggage( _dir, _scanner );
    }
    return result;
}


//  HPJReader::handleOptions --Parse the [OPTIONS] section.

#define MAX_OPTION_LEN  12

size_t HPJReader::handleOptions()
{
    size_t  result;
    char    option[MAX_OPTION_LEN + 1];
    char    *arg;
    int     i;
    for( ;; ) {
        result = _scanner.getLine();
        if( result == 0 || _scanner[0] == '[' )
            break;

        // Read in the name of the option.
        for( i = 0; i < MAX_OPTION_LEN; i++ ) {
            if( isspace( _scanner[i] ) || _scanner[i] == '=' )
                break;
            option[i] = (char)toupper( _scanner[i] );
        }
        option[i] = '\0';

        // At present, I only support a few options.
        // Most of these involve passing information to
        // the HFSystem object "_sysFile".
        if( strcmp( option, STitle ) == 0 ) {
            arg = _scanner.getArg( i );
            if( arg != NULL ) {
                _sysFile->addRecord( new SystemText( HFSystem::SYS_TITLE, arg ) );
            }
        } else if( strcmp( option, SCopyright ) == 0 ) {
            arg = _scanner.getArg( i );
            if( arg != NULL ) {
                _sysFile->addRecord( new SystemText( HFSystem::SYS_COPYRIGHT, arg ) );
            }
        } else if( strcmp( option, SCompress ) == 0 ) {
            arg = _scanner.getArg( i );
            if( arg != NULL ) {
                if( stricmp( arg, STrue ) == 0 ||
                    stricmp( arg, SHigh ) == 0 ||
                    stricmp( arg, SMedium ) == 0 ||
                    stricmp( arg, SYes  ) == 0 ) {
                    _sysFile->setCompress( 1 );
                } else if( stricmp( arg, SFalse ) == 0 ||
                           stricmp( arg, SLow   ) == 0 ||
                       stricmp( arg, SNo    ) == 0 ) {
                    _sysFile->setCompress( 0 );
                }
            }
        } else if( strcmp( option, SOldKeyPhrase ) == 0 ) {
            arg = _scanner.getArg( i );
            if( arg != NULL ) {
                _oldPhrases = ( stricmp( arg, STrue ) == 0 || stricmp( arg, SYes  ) == 0 );
            }
        } else if( strcmp( option, SContents ) == 0 || strcmp( option, SIndex    ) == 0 ) {
            arg = _scanner.getArg( i );
            if( arg != NULL ) {
                _sysFile->setContents( Hash( arg ) );
            }
        } else if( strcmp( option, SBmRoot ) == 0 ) {
            arg = _scanner.getArg( i );
            _theFiles->_bitFiles->addToPath( arg );
        } else if( strcmp( option, SRoot ) == 0 ) {

            // Update the search paths.
            arg = _scanner.getArg( i );
            StrNode *current;
            current = _root;
            if( current != NULL ) {
                while( current->_next != NULL ) {
                    current = current->_next;
                }
            }

            // There may be many directories specified on each line.
            if( arg != NULL ) {
                int j;
                StrNode *temp;
                while( arg[0] != '\0' ) {
                    j = 0;
                    while( arg[j] != '\0' && arg[j] != ',' && arg[j] != ';' ) {
                        ++j;
                    }
                    temp = new StrNode;
                    temp->_name = new char[j + 1];
                    memcpy( temp->_name, arg, j );
                    temp->_name[j] = '\0';
                    temp->_next = NULL;
                    if( chdir( temp->_name ) ) {
                        HCWarning( HPJ_BADDIR, temp->_name );
                        delete[] temp->_name;
                        delete temp;
                    } else if( chdir( _homeDir ) ) {
                        HCWarning( HPJ_BADDIR, _homeDir );
                        delete[] temp->_name;
                        delete temp;
                    } else {
                        if( current == NULL ) {
                            _root = temp;
                            current = _root;
                        } else {
                            current->_next = temp;
                            current = current->_next;
                        }
                    }
                    arg += j;
                    if( arg[0] != '\0' ) {
                        ++arg;
                    }
                }
            }
        }
    }
    return result;
}


//  HPJReader::handleConfig --Parse the [CONFIG] section (where macros
//                             are kept).

size_t HPJReader::handleConfig()
{
    size_t result;
    for( ;; ) {
        result = _scanner.getLine();
        if( result == 0 || _scanner[0] == '[' )
            break;
        _sysFile->addRecord( new SystemText( HFSystem::SYS_MACRO, _scanner ) );
    }
    return result;
}


//  HPJReader::handleFiles --Parse the [FILES] section.

size_t HPJReader::handleFiles()
{
    size_t result;
    size_t i;
    StrNode *current = _rtfFiles;
    StrNode *temp;
    if( current != NULL ) {
        while( current->_next != NULL ) {
            current = current->_next;
        }
    }
    for( ;; ) {
        result = _scanner.getLine();
        if( result == 0 || _scanner[0] == '[' )
            break;
        for( i = 0; _scanner[i] != '\0'; ++i ) {
            if( isspace( _scanner[i] ) ) {
                break;
            }
        }
        _scanner[i] = '\0';
        temp = new StrNode;
        temp->_name = new char[i + 1];
        memcpy( temp->_name, _scanner, i + 1 );
        temp->_next = NULL;
        if( current == NULL ) {
            current = _rtfFiles = temp;
        } else {
            current = current->_next = temp;
        }
    }
    return result;
}


//  HPJReader::handleBitmaps    --Parse the [BITMAPS] section.

size_t HPJReader::handleBitmaps()
{
    size_t result;
    size_t i;
    for( ;; ) {
        result = _scanner.getLine();
        if( result == 0 || _scanner[0] == '[' )
            break;
        i = 0;
        // Eat whitespace.
        while( isspace( _scanner[i] ) )
            i++;
        if( _scanner[i] == '\0' )
            continue;
        try{
            _theFiles->_bitFiles->note( &_scanner[i] );
        } catch( HFBitmaps::ImageNotSupported ) {
            HCWarning( UNKNOWN_IMAGE, &_scanner[0], _scanner.lineNum(), _scanner.name() );
        }
    }
    return result;
}


//  HPJReader::handleWindows    --Parse the [WINDOWS] section.
#define VALID_TYPE      0x0001
#define VALID_NAME      0x0002
#define VALID_CAPTION   0x0004
#define VALID_X         0x0008
#define VALID_Y         0x0010
#define VALID_WIDTH     0x0020
#define VALID_HEIGHT    0x0040
#define VALID_MAX       0x0080
#define VALID_RGB1      0x0100
#define VALID_RGB2      0x0200
#define VALID_ONTOP     0x0400

#define PARAM_MAX       1023

size_t HPJReader::handleWindows()
{
    size_t  result;
    size_t  i, limit;
    char    *arg;
    bool    bad_param;
    int     red, green, blue;
    uint_16 wflags;
    char    name[HLP_SYS_NAME + 1];
    char    caption[HLP_SYS_CAP + 1];
    uint_16 x = 0;
    uint_16 y = 0;
    uint_16 width = 0;
    uint_16 height = 0;
    uint_16 use_max_flag = 0;
    uint_32 rgb_main, rgb_nonscroll;

    for( ; (result = _scanner.getLine()) != 0; ) {
        if( _scanner[0] == '[' )
            break;

        limit = HLP_SYS_NAME;
        if( limit > result - 1 ) {
            limit = result - 1;
        }
        for( i = 0; i < limit; i++ ) {
            if( isspace( _scanner[i] ) || _scanner[i] == '=' )
                break;
            name[i] = _scanner[i];
        }
        if( i == result - 1 ) {
            HCWarning( HPJ_INCOMPLETEWIN, _scanner.lineNum(), _scanner.name() );
            continue;
        } else if( i == HLP_SYS_NAME ) {
            HCWarning( HPJ_LONGWINNAME, _scanner.lineNum(), _scanner.name() );
        }
        name[i] = '\0';
        while( i < result - 1 && !isspace( _scanner[i] ) ) {
            i++;
        }

        arg = _scanner.getArg(i);
        if( arg == NULL || *arg == '\0' ) {
            HCWarning( HPJ_INCOMPLETEWIN, _scanner.lineNum(), _scanner.name() );
            continue;
        }

        wflags = VALID_TYPE | VALID_NAME;
        _winParamBuf = arg;

        arg = nextWinParam();
        if( *arg != '\0' ) {
            i = 0;
            if( *arg == '"' ) {
                arg++;
            }
            while( i < HLP_SYS_CAP && *arg != '\0' && *arg != '"' ) {
                caption[i++] = *arg++;
            }
            caption[i] = '\0';
            wflags |= VALID_CAPTION;
        }

        bad_param = false;
        arg = nextWinParam();
        if( *arg != '\0' ) {
            x = (uint_16)strtol( arg, NULL, 0 );
            if( x > PARAM_MAX ) {
                bad_param = true;
            } else {
                wflags |= VALID_X;
            }
        }
        arg = nextWinParam();
        if( *arg != '\0' ) {
            y = (uint_16)strtol( arg, NULL, 0 );
            if( y > PARAM_MAX ) {
                bad_param = true;
            } else {
                wflags |= VALID_Y;
            }
        }
        arg = nextWinParam();
        if( *arg != '\0' ) {
            width = (uint_16)strtol( arg, NULL, 0 );
            if( width > PARAM_MAX ) {
                bad_param = true;
            } else {
                wflags |= VALID_WIDTH;
            }
        }
        arg = nextWinParam();
        if( *arg != '\0' ) {
            height = (uint_16)strtol( arg, NULL, 0 );
            if( height > PARAM_MAX ) {
                bad_param = true;
            } else {
                wflags |= VALID_HEIGHT;
            }
        }
        if( bad_param ) {
            HCWarning( HPJ_WINBADPARAM, _scanner.lineNum(), _scanner.name() );
            continue;
        }

        arg = nextWinParam();
        if( *arg != '\0' ) {
            use_max_flag = (uint_16)strtol( arg, NULL, 0 );
            wflags |= VALID_MAX;
        }

        red = green = blue = 0;
        bad_param = false;

        arg = nextWinParam();
        if( *arg != '\0' ) {
            red = strtol( arg, NULL, 0 );
            if( red < 0 || red > 255 ) {
                bad_param = true;
            }
            wflags |= VALID_RGB1;
        }
        arg = nextWinParam();
        if( *arg != '\0' ) {
            green = strtol( arg, NULL, 0 );
            if( green < 0 || green > 255 ) {
                bad_param = true;
            }
            wflags |= VALID_RGB1;
        }
        arg = nextWinParam();
        if( *arg != '\0' ) {
            blue = strtol( arg, NULL, 0 );
            if( blue < 0 || blue > 255 ) {
                bad_param = true;
            }
            wflags |= VALID_RGB1;
        }

        if( bad_param ) {
            HCWarning( HPJ_WINBADCOLOR, _scanner.lineNum(), _scanner.name() );
            continue;
        } else {
            rgb_main = (uint_32)(red + ( green << 8 ) + ( blue << 16 ));
        }

        red = green = blue = 0;
        bad_param = false;

        arg = nextWinParam();
        if( *arg != '\0' ) {
            red = strtol( arg, NULL, 0 );
            if( red < 0 || red > 255 ) {
                bad_param = true;
            }
            wflags |= VALID_RGB2;
        }
        arg = nextWinParam();
        if( *arg != '\0' ) {
            green = strtol( arg, NULL, 0 );
            if( green < 0 || green > 255 ) {
                bad_param = true;
            }
            wflags |= VALID_RGB2;
        }
        arg = nextWinParam();
        if( *arg != '\0' ) {
            blue = strtol( arg, NULL, 0 );
            if( blue < 0 || blue > 255 ) {
                bad_param = true;
            }
            wflags |= VALID_RGB2;
        }

        if( bad_param ) {
            HCWarning( HPJ_WINBADCOLOR, _scanner.lineNum(), _scanner.name() );
            continue;
        } else {
            rgb_nonscroll = (uint_32)(red + ( green << 8 ) + ( blue << 16 ));
        }

        arg = nextWinParam();
        if( *arg != 0 || strtol( arg, NULL, 0 ) != 0 ) {
            wflags |= VALID_ONTOP;
        }

        if( strcmp( name, Smain ) == 0 ) {
            _sysFile->addRecord( new SystemWin( wflags, Smain, name, caption,
                            x, y, width, height, use_max_flag,
                        rgb_main, rgb_nonscroll ) );
        } else {
            _sysFile->addRecord( new SystemWin( wflags, Ssecondary, name, caption,
                            x, y, width, height, use_max_flag,
                        rgb_main, rgb_nonscroll ) );
        }
    }
    return result;
}


//  HPJReader::nextWinParam --Helper function for handleWindows.

char *HPJReader::nextWinParam()
{
    char * result = _winParamBuf;
    char * newbuf;

    if( *result != '\0' ) {
        // Eat whitespace.
        while( isspace( *result ) )
            result++;
        newbuf = result;
        if( *newbuf == '"' ) {
            while( *newbuf != ',' && *newbuf != '\0' ) {
                newbuf++;
            }
        } else {
            while( *newbuf != ',' && *newbuf != '\0' ) {
                if( *newbuf == '(' || *newbuf == ')' ) {
                    *newbuf = ' ';
                }
                newbuf++;
            }
            // Eat whitespace.
            while( isspace( *result ) ) {
                result++;
            }
        }
        if( *newbuf == ',' ) {
            *newbuf = '\0';
            newbuf++;
        }
        _winParamBuf = newbuf;
    }
    return result;
}


//  HPJReader::handleMap --Parse the [MAP] section.

size_t HPJReader::handleMap()
{
    size_t  result;
    char    *token;
    uint_32 hash_value;
    int     con_num;
    bool    is_good_string;
    size_t  i;
    for( ;; ) {
        result = _scanner.getLine();
        if( result == 0 || _scanner[0] == '[' )
            break;
        token = _scanner.tokLine();
        if( token == NULL )
            continue;

        // "#include" means go to another file.
        if( stricmp( token, Sinclude ) == 0 ) {
            includeMapFile( _scanner.endTok() );
            continue;
        }

        // "#define" is an optional header, ignore it.
        if( stricmp( token, Sdefine ) == 0 ) {
            token = _scanner.tokLine();
        }
        if( token == NULL )
            continue;

        // verify that the current token at this point is a context string.
        is_good_string = true;
        for( i=0; token[i] != '\0'; ++i ) {
            if( !isalnum( token[i] ) && token[i] != '.' && token[i] != '_' ) {
                is_good_string = false;
            }
        }
        if( !is_good_string ) {
            HCWarning( CON_BAD, token, _scanner.lineNum(), _scanner.name() );
        } else {
            // Associate the context string with a context number.
            hash_value = Hash(token);
            token = _scanner.tokLine();
            if( token == NULL ) {
                HCWarning( CON_NONUM, token, _scanner.lineNum(), _scanner.name() );
            } else {
                con_num = atol( token );
                _theFiles->_mapFile->addMapRec( con_num, hash_value );
            }
        }
    }
    return result;
}


//  HPJReader::includeMapFile   --Parse an #include-d MAP file.

void HPJReader::includeMapFile( char *str )
{
    char seek_char;
    char *name;

    // Eat whitespace.
    while( isspace( *str ) )
        str++;
    // Get the filename.
    switch( *str++ ) {
    case '"':
        seek_char = '"';
        break;
    case '<':
        seek_char = '>';
        break;
    default:
        HCWarning( HPJ_BADINCLUDE, _scanner.lineNum(), _scanner.name() );
        return;
    }

    for( name = str; *str != '\0'; str++ ) {
        if( *str == seek_char ) {
            break;
        }
    }
    if( str == name ) {
        HCWarning( HPJ_BADINCLUDE, _scanner.lineNum(), _scanner.name() );
        return;
    }

    // Now try to find it in the ROOT path and/or current directory.
    *str = '\0';
    StrNode *current;
    InFile  source;
    if( _root == NULL ) {
        source.open( name );
    } else {
        for( current = _root; current != NULL; current = current->_next ) {
            if( chdir( current->_name ) )
                continue;
            source.open( name );
            if( chdir( _homeDir ) )
                HCWarning( HPJ_BADDIR, _homeDir );
            if( !source.bad() ) {
                break;
            }
        }
    }

    if( source.bad() ) {
        HCWarning(INCLUDE_ERR, name, _scanner.lineNum(), _scanner.name() );
        return;
    }

    HCStartFile( name );

    // Now parse the secondary file.
    HPJScanner  input( &source );
    bool    not_done;
    char    *token;
    bool    is_good_str;
    int     con_num;
    uint_32 hash_value;
    for( ;; ) {
        not_done = ( input.getLine() != 0 );
        if( !not_done )
            break;

        token = input.tokLine();

        if( stricmp( token, Sinclude ) == 0 ) {

            // "#include" directives may be nested.
            includeMapFile( input.endTok() );
            continue;
        } else if( stricmp( token, Sdefine ) == 0 ) {

            // "#define" directives specify a context string.
            token = input.tokLine();
            if( token == NULL )
                continue;
            is_good_str = true;
            for( int i = 0; token[i] != '\0'; ++i ) {
                if( !isalnum( token[i] ) && token[i] != '.' && token[i] != '_' ) {
                    is_good_str = false;
                }
            }
            if( !is_good_str ) {
                HCWarning( CON_BAD, token, input.lineNum(), input.name() );
            } else {
                hash_value = Hash( token );
                token = input.tokLine();
                if( token == NULL ) {
                    HCWarning( CON_NONUM, token, input.lineNum(), input.name() );
                } else {
                    con_num = atol( token );
                    _theFiles->_mapFile->addMapRec( con_num, hash_value );
                }
            }
        } else if( strncmp( token, SstartComment, 2 ) == 0 ) {

            // #include-d files may contain comments.
            int startcomment = input.lineNum();
            while( token != NULL && strstr( token, SendComment ) == NULL ) {
                do {
                    token = input.tokLine();
                    if( token != NULL )
                        break;
                    not_done = ( input.getLine() != 0 );
                } while( not_done );
            }

            if( token == NULL ) {
                HCWarning( HPJ_RUNONCOMMENT, startcomment, input.name() );
                break;
            }
        } else {
            HCWarning( HPJ_INC_JUNK, input.lineNum(), input.name() );
            continue;
        }
    }
    HCDoneTick();
    return;
}
