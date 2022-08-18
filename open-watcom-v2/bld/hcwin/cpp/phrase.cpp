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
* Description:  Phrase replacement for compression purposes.
*
****************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include "phrase.h"
#include "compress.h"

#define NEW_USAGE       "Usage: phrase filename"

#define HASH_SIZE       691
#define PTBL_SIZE       1720
#define MAX_DATA_SIZE   0xFFFF
#define STR_BLOCK       80


struct Edge;    // Forward declaration.

//
//  Phrase  --specialized string class for storing
//        candidate strings.
//

struct Phrase
{
    Buffer<char>    _str;
    unsigned        _len;

    int             _numUses;
    Edge            *_firstEdge;

    Phrase          *_next;

    int             _val;

    static Pool     *_pool;
    static void initPool() { _pool = new Pool( sizeof( Phrase ) , PTBL_SIZE ); };
    static void freePool() { delete _pool; };

    void *operator new( size_t ) { return _pool->get(); };
    void operator delete( void *p, size_t ) { _pool->release( p ); };

    Phrase();
    Phrase( Phrase &p );
    ~Phrase() {};

    Phrase &    operator=( Phrase &p );
    int         operator>( Phrase const &p );

    void        chkBufSize( unsigned new_len=0 );
};

Pool *Phrase::_pool = NULL;


// Check line size.

void Phrase::chkBufSize( unsigned new_len )
{
    if( new_len ) {
    } else if( _len < _str.len() ) {
        return;
    } else {
        new_len = 2 * _str.len();
    }
    _str.resize( new_len );
}

//  Phrase::Phrase  --default constructor.

Phrase::Phrase()
    : _str( STR_BLOCK ),    // Arbitrary large value
      _len( 0 )
{
}


//  Phrase::Phrase  --copy constructor.

Phrase::Phrase( Phrase &p )
    : _str( p._len ),
      _len( p._len ),
      _numUses( 1 ),
      _firstEdge( NULL ),
      _next( NULL )
{
    memcpy( _str, p._str, _len );
}


//  Phrase::operator=   --Assignment operator.

Phrase & Phrase::operator=( Phrase &p )
{
    _len = p._len;
    _str.resize( _len );
    memcpy( _str, p._str, _len );
    _numUses = 1;
    _firstEdge = NULL;
    _next = NULL;

    return *this;
}


//  Phrase::operator>   --comparison operator.

int Phrase::operator>( Phrase const &p )
{
    return _val > p._val;
}


//
//  P_String    --Class to represent phrases AFTER the phrase table
//        has been selected.
//

struct P_String
{
    Buffer<char>    _str;
    unsigned        _index;
    P_String        *_next;

    P_String( Phrase &p );
    ~P_String() {};

private:
    // Assignment of P_Strings is not permitted.
    P_String( P_String const & ) : _str( 0 ) {};
    P_String &  operator=( P_String const & ) { return *this; };
};


//  P_String::P_String  --Constructor.

P_String::P_String( Phrase &p )
    : _str( p._len ), _next( NULL )
{
    memcpy( _str, p._str, p._len );
}


//
//  Edge    --Class to represent the notion "Phrase B follows Phrase
//        A".
//

struct Edge
{
    Phrase      *_dest;
    Edge        *_next;
    int         _val;
};


//
//  PTable  --Class to store a working set of Phrases.
//

class PTable
{
    Pool                *_edges;

    Buffer<Phrase *>    _hptable;

    Buffer<Phrase *>    _phrases;
    unsigned            _size;

    unsigned            _iterPos;
    bool                _initialized;

    // Helper function for heapsort-ing.
    void                heapify( unsigned start );

    // Helper function for hashing.
    size_t              getHash( const char *str, size_t len );

    // Assignment of PTable's is not permitted, so it's private.
    PTable( PTable const & ) : _hptable( 0 ), _phrases( 0 ) {};
    PTable &    operator=( PTable const & ) { return *this; };

public:
    PTable();
    ~PTable();

    Phrase              *match( char * &start );
    Phrase              *find( Phrase *other );
    int                 &follows( Phrase *first, Phrase *second );

    void                insert( Phrase *p );

    //  Access function.
    unsigned            size() { return _size; };

    // Iter functions.
    void                start();
    Phrase              *next();

    void                clear();
    void                prune();
};


//  PTable::PTable  --Constructor.

PTable::PTable()
    : _hptable( 0 ),
      _phrases( 0 ),
      _size( 0 ),
      _initialized( false )
{
    _edges = new Pool( sizeof( Edge ) );

    _phrases.resizeNull( PTBL_SIZE );
    _hptable.resizeNull( HASH_SIZE );
}


//  PTable::~PTable --Destructor.

PTable::~PTable()
{
    // Delete any phrases remaining in the Pool.
    for( size_t i = 0; i < _size; i++ ) {
        delete _phrases[i];
    }
    _size = _hptable.len();
    for( size_t i = 0; i < _size; i++ ) {
        delete _hptable[i];
    }
    if( _edges ) {
        delete _edges;
    }
}


#define PH_MIN_LEN  3

size_t PTable::getHash( const char *str, size_t len )
{
    uint_32 h_val = 0;

    if( len > PH_MIN_LEN )
        len = PH_MIN_LEN;
    memcpy( &h_val, str, len );
    return( h_val % HASH_SIZE );
}

//  PTable::match   --Find the best match for a character string.

Phrase *PTable::match( char * &start )
{
    size_t      length;
    size_t      h_val;
    Phrase      *result;

    length = strlen( start );
    if( length == 0 ) {
        return NULL;
    }

    result = NULL;

    if( length >= PH_MIN_LEN ) {
        // First, try to find a match based on the first three characters.
        h_val = getHash( start, length );
        for( result = _hptable[h_val]; result != NULL; result = result->_next ) {
            if( result->_len <= length ) {
                break;
            }
        }
        for( ; result != NULL; result = result->_next ) {
            if( memcmp( result->_str, start, result->_len ) == 0 ) {
                break;
            }
        }
    }
    if( result != NULL ) {
        start += result->_len;
        if( isspace( *start ) ) {
            start++;
        }
    } else {
        // If necessary, check for a shorter match.
        if( length >= PH_MIN_LEN - 1 ) {
            for( length = 1; length < PH_MIN_LEN - 1; ++length ) {
                if( isspace( start[length] ) ) {
                    break;
                }
            }
        }
        h_val = getHash( start, length );
        for( result = _hptable[h_val]; result != NULL; result = result->_next ) {
            if( result->_len <= length ) {
                break;
            }
        }
        for( ; result != NULL; result = result->_next ) {
            if( memcmp( result->_str, start, result->_len ) == 0 ) {
                break;
            }
        }
        if( result != NULL ) {
            start += result->_len;
            if( isspace( *start ) ) {
                start++;
            }
        } else {
            bool found_text = false;
            while( *start != '\0' ) {
                if( found_text && isspace( *start ) ) {
                    start++;
                    break;
                } else if( !found_text && !isspace( *start ) ) {
                    found_text = true;
                }
                start++;
            }
        }
    }
    return result;
}


//  PTable::find    --Find a given Phrase in the table.

Phrase *PTable::find( Phrase *other )
{
    size_t      h_val;
    Phrase      *result;
    unsigned    len = other->_len;
    char        *str = other->_str;

    h_val = getHash( str, len );
    for( result = _hptable[h_val]; result != NULL; result = result->_next ) {
        if( result->_len <= len ) {
            break;
        }
    }
    for( ; result != NULL; result = result->_next ) {
        if( result->_len != len || memcmp( result->_str, str, len ) == 0 ) {
            break;
        }
    }
    if( result != NULL && result->_len != len ) {
        result = NULL;
    }

    return result;
}


//  PTable::follows --Return a reference to the number of times
//            "second" has followed "first".

int &PTable::follows( Phrase *first, Phrase *second )
{
    Edge    *current;

    for( current = first->_firstEdge; current != NULL; current = current->_next ) {
        if( current->_dest == second ) {
            return( current->_val );
        }
    }
    current = (Edge *)_edges->get();
    current->_dest = second;
    current->_val = 0;
    current->_next = first->_firstEdge;
    first->_firstEdge = current;
    return( current->_val );
}


//  PTable::insert  --insert a Phrase in the table.

void PTable::insert( Phrase *p )
{
    Phrase  *current, *temp;
    size_t  h_val;
    size_t  len = p->_len;

    h_val = getHash( p->_str, len );
    temp = NULL;
    for( current = _hptable[h_val]; current != NULL; current = current->_next ) {
        if( current->_len <= len )
            break;
        temp = current;
    }
    p->_next = current;
    if( temp != NULL ) {
        temp->_next = p;
    } else {
        _hptable[h_val] = p;
    }

    if( _size == _phrases.len() ) {
        _phrases.resizeNull( 2 * _size );
    }
    _phrases[_size++] = p;
}


//  PTable::start   --Start iterating through the table.

void PTable::start()
{
    _initialized = true;
    _iterPos = 0;
}


//  PTable::next    --Iterate through the phrase table.

Phrase *PTable::next()
{
    if( _initialized && _iterPos < _size )
        return _phrases[_iterPos++];

    return NULL;
}


//  PTable::clear   --Clear the phrase table.

void PTable::clear()
{
    unsigned    i;
    Edge        *current, *next;

    for( i = 0; i < _size; i++ ) {
        for( current = _phrases[i]->_firstEdge; current != NULL; current = next ) {
            next = current->_next;
            _edges->release( current );
        }
        delete _phrases[i];
    }
    for( i = 0; i < HASH_SIZE; i++ ) {
        _hptable[i] = NULL;
    }
    _initialized = false;
    _size = 0;
}


//  PTable::heapify --Helper function for heapsort.

void PTable::heapify( unsigned start )
{
    unsigned    left, right;
    unsigned    max = start;
    Phrase      *temp;

    for( ;; ) {
        left = 2 * start + 1;
        right = left + 1;

        if( left < _size && (*_phrases[left]) > (*_phrases[start]) ) {
            max = left;
        }
        if( right < _size && (*_phrases[right]) > (*_phrases[max]) ) {
            max = right;
        }

        if( max == start )
            break;

        temp = _phrases[start];
        _phrases[start] = _phrases[max];
        _phrases[max] = temp;

        start = max;
    }
}


//  PTable::prune   --Eliminate all but the 'best' phrases.

void PTable::prune()
{
    unsigned    old_size;
    unsigned    i;
    Phrase      *temp;
    char        *firstc, *startc;
    unsigned    totalsize;

    // Toss out memory we no longer need.
    delete _edges;
    _edges = NULL;

    _hptable.resize( 0 );

    // Heapsort the Phrases to get the top (PTBL_SIZE) Phrases.
    old_size = _size;
    for( i = 0; i < _size; i++ ) {
        // Get rid of leading spaces for efficiency reasons.
        firstc = _phrases[i]->_str;
        startc = _phrases[i]->_str;
        while( firstc - startc < _phrases[i]->_len ) {
            if( isspace( *firstc ) ) {
                firstc++;
            } else {
                break;
            }
        }
        if( firstc > startc ) {
            memmove( startc, firstc, _phrases[i]->_len - (unsigned)( firstc - startc ) );
            _phrases[i]->_len -= (unsigned)( firstc - startc );
        }
        _phrases[i]->_val = ( _phrases[i]->_len - 2 ) * ( _phrases[i]->_numUses - 1 );
    }
    if( _size >= 2 ) {
        for( i = ( _size - 2 ) / 2; i > 0; --i ) {
            heapify( i );
        }
        heapify( i );
    }

    totalsize = 2;  // Size of first 2-byte phrase index.
    for( i = 0; i < PTBL_SIZE; i++ ) {
        if( _size == 0 )
            break;
        totalsize += _phrases[0]->_len + 2; // Phrase length + index length

        if( totalsize > MAX_DATA_SIZE ) {
            break;
        }

        temp = _phrases[_size - 1];
        _phrases[_size - 1] = _phrases[0];
        _phrases[0] = temp;
        _size--;
        heapify(0);
    }

    // Delete the remainder of the phrases.
    for( i = 0; i < _size; i++ ) {
        delete _phrases[i];
    }
    for( ; i < old_size; i++ ) {
        if( _phrases[i]->_val > 4 )
            break;
        delete _phrases[i];
    }
    _size = old_size - i;
    memmove( _phrases, _phrases + i, _size * sizeof( Phrase * ) );
}



//  HFPhrases::HFPhrases()  --Default constructor.

HFPhrases::HFPhrases( HFSDirectory * d_file, bool (*firstf)(InFile *), bool (*nextf)(InFile *) )
    : _oldPtable( NULL ),
      _newPtable( NULL ),
      _result( 0 ),
      _hptable( 0 ),
      _numPhrases( 0 ),
      _size( 0 ),
      _nextf( nextf ),
      _firstf( firstf ),
      _scanner( NULL )
{
    d_file->addFile( this, "|Phrases" );
}



//  HFPhrases::~HFPhrases() --Destructor.

HFPhrases::~HFPhrases()
{
    if( _oldPtable )
        delete _oldPtable;
    if( _newPtable )
        delete _newPtable;
    for( unsigned i = 0; i < _resultSize ; i++ ) {
        delete _result[i];
    }
}


//  HFPhrases::size --Overrides Dumpable::size.

uint_32 HFPhrases::size()
{
    if( _size > 0 ) {
        return _size;
    }

    if( _result.len() == 0 ) {
        createQueue( "phrases.ph" );
    }

    CompWriter  riter;
    CompReader  reader( &riter );
    P_String    *string;
    unsigned    i;

    _size = 10;     // Size of the phrase table header.
    _phSize = 0;

    for( i = 0; i < _numPhrases; i++ ) {
        string = _result[i];
        _phSize += string->_str.len();
        _size += sizeof( uint_16 ) + reader.compress( string->_str, string->_str.len() );
    }

    return _size;
}



//  HFPhrases::dump --Overrides Dumpable::dump.

int HFPhrases::dump( OutFile *dest )
{
    const uint_16 magic = 0x0100;
    unsigned      i;

    dest->write( _numPhrases );
    dest->write( magic );
    dest->write( _phSize );

    uint_16 curr_size = (uint_16)( ( _numPhrases + 1 ) * sizeof( uint_16 ) );
    for( i = 0; i < _numPhrases; i++ ) {
        dest->write( curr_size );
        curr_size = (uint_16)( curr_size + _result[i]->_str.len() );
    }
    dest->write( curr_size );

    CompOutFile riter( dest );
    CompReader  reader( &riter );
    P_String    *string;

    for( i = 0; i < _numPhrases; i++ ) {
        string = _result[i];
        reader.compress( string->_str, string->_str.len() );
    }
    reader.flush();

    return 1;
}


//  removeScanner   --Remove input source scanner and file.

void HFPhrases::removeScanner()
{
    if( _scanner != NULL ) {
        _scanner->file()->close();
        delete _scanner;
        _scanner = NULL;
    }
}



//  HFPhrases::startInput   --Prepare to read the first block
//                of input.

void HFPhrases::startInput( InFile *input )
{
    removeScanner();
    if( !(*_firstf)( input ) )
        return;
    _scanner = new Scanner( input );
}


//  HFPhrases::nextInput    --Get the next block of input.
//

char* HFPhrases::nextInput( InFile *input )
{
    Token   *next;
    char    *result;

    if( _scanner == NULL )
        return NULL;

    for( ;; ) {
        next = _scanner->next();

        if( next->_type == TOK_END ) {
            removeScanner();
            if( !(*_nextf)( input ) ) {
                return NULL;
            }
            _scanner = new Scanner( input );

        } else if( next->_type != TOK_TEXT ) {
            int push_level;

            for( ; next->_type != TOK_END && next->_type != TOK_TEXT; next = _scanner->next() ) {
                if( next->_type == TOK_COMMAND ) {
                    if( strcmp( next->_text, "colortbl" ) == 0
                      || strcmp( next->_text, "fonttbl" ) == 0
                      || strcmp( next->_text, "footnote" ) == 0
                      || strcmp( next->_text, "stylesheet" ) == 0 ) {
                        push_level = 0;
                        do {
                            next = _scanner->next();
                            if( next->_type == TOK_PUSH_STATE ) {
                                push_level++;
                            } else if( next->_type == TOK_POP_STATE ) {
                                push_level--;
                            } else if( next->_type == TOK_END ) {
                                break;
                            }
                        } while( push_level >= 0 );
                    } else if( strcmp( next->_text, "v" ) == 0
                      && (!next->_hasValue || next->_value != 0 ) ) {
                        push_level = 0;
                        do {
                            next = _scanner->next();
                            if( next->_type == TOK_PUSH_STATE ) {
                                push_level++;
                            } else if( next->_type == TOK_POP_STATE ) {
                                push_level--;
                            } else if( next->_type == TOK_COMMAND
                              && strcmp( next->_text, "v" ) == 0
                              && next->_hasValue && next->_value == 0 ) {
                                break;
                            } else if( next->_type == TOK_END ) {
                                break;
                            }
                        } while( push_level >= 0 );
                    }
                }
            }
            if( next->_type == TOK_END ) {
                removeScanner();
                if( !(*_nextf)( input ) ) {
                    return NULL;
                }
                _scanner = new Scanner( input );
            } else {
                result = next->_text;
                break;
            }
        } else {
            result = next->_text;
            break;
        }
    }

    return result;
}


//  HFPhrases::readPhrases  --Fill the phrase table with candidate
//                phrases.

void HFPhrases::readPhrases()
{
    char    *block = NULL;
    char    *end;
    bool    found_text;
    int     count;
    bool    getnext;
    Phrase  phr;
    Phrase  *p_phr, *last, *next, *lookahead;
    PTable  *temp;
    Edge    *current;
    InFile  input;


    Phrase::initPool();

    _oldPtable = new PTable;
    _newPtable = new PTable;

    // Put all of the words in the file in a dictionary.
    HCStartPhrase();
    HCPhraseLoop(1);
    startInput( &input );
    while( (block = nextInput( &input )) != NULL ) {
        last = NULL;
        while( *block != '\0' ) {
            found_text = false;
            phr._len = 0;
            end = block;
            while( *end != '\0' ) {
                if( found_text && isspace( *end ) ) {
                    break;
                } else if( !found_text && !isspace( *end ) ) {
                    found_text = true;
                }
                phr.chkBufSize();
                phr._str[phr._len++] = *end++;
            }

            // Create the phrase.
            p_phr = _newPtable->find( &phr );
            if( p_phr != NULL ) {
                p_phr->_numUses += 1;
            } else {
                p_phr = new Phrase( phr );
                _newPtable->insert( p_phr );
            }

            if( last != NULL ) {
                _newPtable->follows( last, p_phr ) += 1;
            }
            last = p_phr;

            if( *end != '\0' ) {
                block = end+1;
            } else {
                block = end;
            }
        }
    }

    // Build up longer phrases iteratively with extra
    // passes over the file.

    // NOTE THE ARBITRARY CUTOFF.  I have reason to suspect this
    // algorithm is non-terminating in certain cases.
    for( count = 1; count < 10; count++ ) {
        HCPhraseLoop( count+1 );

        temp = _oldPtable;
        _oldPtable = _newPtable;
        _newPtable = temp;

        startInput( &input );
        while( (block = nextInput( &input )) != NULL ) {
            last = NULL;
            next = NULL;
            lookahead = NULL;
            getnext = true;
            while( *block != '\0' ) {
                if( getnext ) {
                    next = _oldPtable->match( block );
                }
                if( *block != '\0' ) {
                    lookahead = _oldPtable->match( block );
                } else {
                    lookahead = NULL;
                }
                if( next == NULL || lookahead == NULL
                  || _oldPtable->follows( next, lookahead ) < 2 ) {
                    if( next != NULL ) {
                        p_phr = _newPtable->find( next );
                        if( p_phr != NULL ) {
                            p_phr->_numUses++;
                        } else {
                            p_phr = new Phrase( *next );
                            _newPtable->insert( p_phr );
                        }
                        if( last != NULL ) {
                            _newPtable->follows( last, p_phr ) += 1;
                        }
                    } else {
                        p_phr = NULL;
                    }
                    next = lookahead;
                    getnext = false;
                } else {
                    // Set phr to (next + lookahead).
                    phr.chkBufSize( next->_len + lookahead->_len + 1 );
                    phr._len = phr._str.len();
                    memcpy( phr._str, next->_str, next->_len );
                    phr._str[next->_len] = ' ';
                    memcpy( phr._str + next->_len + 1, lookahead->_str, lookahead->_len );

                    p_phr = _newPtable->find( &phr );
                    if( p_phr != NULL ) {
                        p_phr->_numUses++;
                    } else {
                        p_phr = new Phrase( phr );
                        _newPtable->insert( p_phr );
                    }
                    if( last != NULL ) {
                        _newPtable->follows( last, p_phr ) += 1;
                    }

                    next = NULL;
                    lookahead = NULL;
                    getnext = true;
                }
                last = p_phr;
            }
            if( next != NULL ) {
                p_phr = _newPtable->find( next );
                if( p_phr != NULL ) {
                    p_phr->_numUses++;
                } else {
                    p_phr = new Phrase( *next );
                    _newPtable->insert( p_phr );
                }
                if( last != NULL ) {
                    _newPtable->follows( last, p_phr ) += 1;
                }
            }
        }

        _oldPtable->clear();

        _newPtable->start();
        while( (p_phr = _newPtable->next()) != NULL ) {
            for( current = p_phr->_firstEdge; current != NULL; current = current->_next ) {
                if( current->_val >= 2 ) {
                    break;
                }
            }
            if( current != NULL ) {
                break;
            }
        }
        if( p_phr == NULL ) {
            break;
        }
    }

    HCDoneTick();

    delete _oldPtable;
    _oldPtable = NULL;
}


//  HFPhrases::initHashPTable    --Initialize the hash table.

void HFPhrases::initHashPTable()
{
    uint_32     hvalue;
    P_String    *curr_str;

    if( _hptable.len() == 0 ) {
        _hptable.resize( HASH_SIZE );
    }
    memset( _hptable, 0, HASH_SIZE * sizeof( P_String * ) );

    for( size_t i = 0; i < _resultSize; i++ ) {
        curr_str = _result[i];
        memcpy( &hvalue, curr_str->_str, PH_MIN_LEN );
        hvalue &= 0xFFFFFF;
        hvalue %= HASH_SIZE;

        curr_str->_next = _hptable[hvalue];
        _hptable[hvalue] = curr_str;
    }
}


//  HFPhrases::createQueue  --Find all candidate Phrases with a high
//                enough _value field, and add them to a
//                priority queue.

void HFPhrases::createQueue( char const *path )
{
    Phrase      *current;
    size_t      i;

    _newPtable->prune();

    _result.resize( _newPtable->size() );
    _resultSize = _result.len();

    _newPtable->start();

    OutFile ph_file( path );
    if( ph_file.bad() ) {
        HCError( FILE_ERR, path );
    }
    for( i = 0; (current = _newPtable->next()) != NULL; i++ ) {
        _result[i] = new P_String( *current );

        ph_file.write( _result[i]->_str, _result[i]->_str.len() );
        ph_file.write( (uint_8)'\r' );
        ph_file.write( (uint_8)'\n' );

        _result[i]->_index = i;
    }
    ph_file.close();

    // We no longer need the dictionary, or the Phrase queue.
    delete _newPtable;
    _newPtable = NULL;

    Phrase::freePool();

    // Initialize the 'hash table'.
    initHashPTable();
}


//  HFPhrases::oldTable --Use a previously created phrase table.

int HFPhrases::oldTable( char const *path )
{
    InFile  ph_file( path );
    if( ph_file.bad() ) {
        return 0;
    }

    Phrase      current;
//    int         done = 0;
    int         c = '\0';
    unsigned    totalsize;  // Size of the phrase data loaded.

    _result.resize( PTBL_SIZE );
    _resultSize = 0;
    current._len = 0;
    totalsize = 2;  // Size of first 2-byte phrase index.
    while( c != EOF ) {
        c = ph_file.nextch();
        if( c == EOF || c == '\n' ) {
            if( current._len != 0 ) {
                totalsize += current._len + 2;  // Phrase size + index size
                if( totalsize > MAX_DATA_SIZE ) {
                    break;
                }

                if( _resultSize == _result.len() ) {
                    _result.resize( 2 * _resultSize );
                }
                _result[_resultSize] = new P_String( current );
                _result[_resultSize]->_index = _resultSize;
                _resultSize += 1;
                current._len = 0;
            }
        } else {
            current.chkBufSize();
            current._str[current._len++] = (char)c;
        }
    }

    // Initialize the 'hash table'.
    initHashPTable();

    return 1;
}



//  HFPhrases::replace  --Go through a block of text and replace
//            common phrases where they appear.

void HFPhrases::replace( char * dst, char const *src, size_t & len )
{
    uint_32     hvalue = 0;
    P_String    *current, *best;
    size_t      read_pos = 0;
    size_t      write_pos = 0;

    if( len > 2 ) {
        while( read_pos < len - 2 ) {
            memcpy( &hvalue, src + read_pos, PH_MIN_LEN );
            hvalue %= HASH_SIZE;

            best = NULL;
            for( current = _hptable[hvalue]; current != NULL; current = current->_next ) {
                if( current->_str.len() <= len - read_pos && memcmp( current->_str, src + read_pos, current->_str.len() ) == 0 ) {
                    if( best == NULL || best->_str.len() < current->_str.len() ) {
                        best = current;
                    }
                }
            }

            if( best == NULL ) {
                dst[write_pos++] = src[read_pos++];
            } else {
                if( best->_index >= _numPhrases ) {
                    if( best->_index > _numPhrases ) {
                        P_String *temp = _result[_numPhrases];
                        _result[_numPhrases] = _result[best->_index];
                        _result[best->_index] = temp;
                        _result[best->_index]->_index = best->_index;
                        best->_index = _numPhrases;
                    }
                    _numPhrases = (uint_16)( _numPhrases + 1 );
                }

                // Convert the index to a WinHelp "phrase code".
                // See "phrases.doc".
                char c = (char)( (best->_index & 0x7f) << 1 );
                dst[write_pos++] = (char)( (( best->_index >> 7 ) & 0xF) + 1 );
                read_pos += best->_str.len();
                if( src[read_pos] == ' ' ) {
                    c |= 0x1;
                    read_pos++;
                }
                dst[write_pos++] = c;
            }
        }
    }
    while( read_pos < len ) {
        dst[write_pos++] = src[read_pos++];
    }

    len = write_pos;
}
