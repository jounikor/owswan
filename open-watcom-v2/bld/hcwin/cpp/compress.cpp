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
* Description:  WinHelp-style LZ77 compression.
*
****************************************************************************/


#include "compress.h"
#include <string.h>

#define HOLD_SIZE   4096
#define HOLD_BUF    (2 * HOLD_SIZE)
#define WRITE_SIZE  8
#define READ_SIZE   18
#define MIN_READ    3
#define HTABLE_SIZE 65537
#define HTABLE_NIL  -1
#define RARE_BYTE   0xF1


//  LocalHash   --Hash a 3-character string for lookup purposes.

unsigned LocalHash( uint_8 values[] )
{
    unsigned result = 0;
    for( int i = 0; i < MIN_READ; i++ ) {
        result *= 0x10;
        result += values[i];
        result %= HTABLE_SIZE;
    }
    return result;
}


//  CompWriter::CompWriter

CompWriter::CompWriter()
    : _numBytes( 0 ),
      _numTokens( 0 ),
      _bitMask( 0 )
{
    _buffer = new uint_8[2 * WRITE_SIZE];
}


//  CompWriter::~CompWriter

CompWriter::~CompWriter()
{
    delete[] _buffer;
}


//  CompWriter::dump    --For the base class, throw out the buffers.

void CompWriter::dump()
{
    _numBytes = 0;
    _numTokens = 0;
    _bitMask = 0;
}


//  CompWriter::putChr  --Add a raw character to output.

unsigned CompWriter::putChr( uint_8 c )
{
    unsigned result = 1;
    if( _numTokens == 0 ) {
        ++result;
    }
    _numTokens++;
    _buffer[_numBytes++] = c;
    if( _numTokens == WRITE_SIZE ) {
        dump();
    }
    return result;
}


//  CompWriter::putCode --Add a distance/length code to output.

unsigned CompWriter::putCode( unsigned distance, unsigned length )
{
    unsigned result = 2;
    if( _numTokens == 0 ) {
        ++result;
    }

    // Convert the distance/length to a two-byte code.
    // See "compress.doc" for the description of Microsoft-style
    // LZ77 compression.

    _buffer[_numBytes++] = (uint_8)( ( distance - 1 ) % 256 );
    _buffer[_numBytes++] = (uint_8)( ( length - 3 ) * 16 + ( distance - 1 ) / 256 );

    // Update the current bitmask.
    uint_8 mask = 1;
    mask <<= _numTokens++;
    _bitMask |= mask;

    if( _numTokens == WRITE_SIZE ) {
        dump();
    }
    return result;
}


//  CompOutFile::dump   --Send the current set of codes to the output file.

void CompOutFile::dump()
{
    if( _numTokens > 0 ) {
        _dest->write( _bitMask );
        _dest->write( _buffer, _numBytes );
        _numBytes = _numTokens = 0;
    }
    _bitMask = 0;
}


//  CompReader::shuffle --Make room in the buffer for new text.

void CompReader::init()
{
    _last = 0;
    _first = 0;
    _current = 0;

    // Set all of the hash table entries to -1.
    for( size_t i = 0; i < HTABLE_SIZE; ++i ) {
        _htable[i] = HTABLE_NIL;
    }

    *_indices = HTABLE_NIL;
}


//  CompReader::CompReader

CompReader::CompReader( CompWriter *riter )
    : _buffer( HOLD_BUF ),
      _indices( HOLD_BUF ),
      _htable( HTABLE_SIZE ),
      _dest( riter )
{
    init();
}


//  CompReader::flush   --Throw out the buffered text and
//            start compressing from scratch.

void CompReader::flush( bool nodump )
{
    init();

    if( !nodump ) {
        _dest->dump();
    }
    return;
}


//  CompReader::reset   --Switch to a new CompWriter.

void CompReader::reset( CompWriter *riter, bool nodump )
{
    flush( nodump );
    _dest = riter;
}


//  CompReader::shuffle --Make room in the buffer for new text.

void CompReader::shuffle()
{
    size_t  i;
    size_t  len = _last - _first;

    memmove( _buffer, _buffer + _first, len );
    HCTick();

    if( len > static_cast<size_t>( _current ) )
        len = static_cast<size_t>( _current );
    for( i = 0; i < len; ++i ) {
        if( _indices[i + _first] < _first ) {
            _indices[i] = HTABLE_NIL;
        } else {
            _indices[i] = _indices[i + _first] - _first;
        }
    }

    for( i = 0; i < HTABLE_SIZE; ++i ) {
        if( _htable[i] < _first ) {
            _htable[i] = HTABLE_NIL;
        } else {
            _htable[i] -= _first;
        }
    }

    _last -= _first;
    _current -= _first;
    _first = 0;
}


//  CompReader::compress    --Compress a block of text.

unsigned CompReader::compress( char const source[], unsigned amount )
{
    unsigned result = 0;
    if( _last + amount > HOLD_BUF ) {
        shuffle();
    }

    memcpy( _buffer + _last, source, amount );
    _last += amount;

    size_t      hash_value;
    int         key_size, old_key_size;
    int         offset;
    int         best_match = 0;
    int         limit;
    uint_8      *p1, *p2;

    while( _current + MIN_READ <= _last ) {
        // Find the linked list corresponding to the current string.

        hash_value = LocalHash( _buffer + _current );
        old_key_size = MIN_READ - 1;

        offset = _htable[hash_value];
        _indices[static_cast<size_t>( _current )] = offset;
        _htable[hash_value] = _current;

        limit = READ_SIZE;
        if( limit > _last - _current ) {
            limit = _last - _current;
        }

        while( offset >= _first ) {
            // Find the length of the longest common string
            // starting at offset and current.  Optimized
            // for a little extra speed.

            p1 = _buffer + offset;
            p2 = _buffer + _current;

            // Compare four bytes at a time.
            for( key_size = 0; key_size < limit; key_size += 4 ) {
                if( *(uint_32 *)( p1 + key_size ) != *(uint_32 *)( p2 + key_size ) ) {
                    break;
                }
            }
            if( key_size > limit ) {
                key_size = limit;
            } else {
                while( key_size < limit && _buffer[static_cast<size_t>( offset + key_size )] == _buffer[static_cast<size_t>( _current + key_size )] ) {
                    key_size += 1;
                }
            }

            if( key_size > old_key_size ) {
                old_key_size = key_size;
                best_match = offset;
                if( old_key_size == limit ) {
                    break;
                }
            }
            offset = _indices[static_cast<size_t>( offset )];
        }

        // See if we found a match of usable size.

        if( old_key_size < MIN_READ ) {
            result += _dest->putChr( _buffer[static_cast<size_t>( _current )] );
            _current += 1;
        } else {
            result += _dest->putCode( _current - best_match, old_key_size );
            _current += old_key_size;
        }
        if( _current > HOLD_SIZE ) {
            _first = _current - HOLD_SIZE;
        }
    }

    // If there's text left over, dump it to output.

    while( _current < _last ) {
        result += _dest->putChr( _buffer[static_cast<size_t>( _current++ )] );
    }
    if( _current > HOLD_SIZE ) {
        _first = _current - HOLD_SIZE;
    }

    return( result );
}


//  CompReader::add --Add more text to buffer, but
//            pretend it's uncompressible.

unsigned CompReader::add( char const source[], unsigned amount )
{
    unsigned result = 0;
    if( _last + amount > HOLD_BUF ) {
        shuffle();
    }

    // There is a danger of the compressor referring to
    // this part of the buffer later to compress other text
    // (which would be inconsistent with skip().) so we fill
    // the buffer with a "rare" byte.
    memset( _buffer + _current, RARE_BYTE, amount );

    for( unsigned i = 0; i < amount; ++i ) {
        result += _dest->putChr( source[i] );
    }
    _last += amount;
    _current += amount;
    if( _current > HOLD_SIZE ) {
        _first = _current - HOLD_SIZE;
    }
    return( result );
}


//  CompReader::skip    --Pretend to add uncompressible text to output.

unsigned CompReader::skip( unsigned amount )
{
    unsigned result = 0;
    if( _last + amount > HOLD_BUF ) {
        shuffle();
    }

    // There is a danger of the the compressor referring to
    // this part of the buffer later to compress other text.
    // So we fill the buffer with a "rare" byte.
    memset( _buffer + _current, RARE_BYTE, amount );

    for( unsigned i = 0; i < amount; ++i ) {
        result += _dest->putChr( 0 );
    }
    _last += amount;
    _current += amount;
    if( _current > HOLD_SIZE ) {
        _first = _current - HOLD_SIZE;
    }
    return( result );
}
