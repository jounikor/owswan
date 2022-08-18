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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/*
CONTEXT:  Hash-value-to-topic-offset mapping.
*/

#include <string.h>
#include "context.h"
#include "hcmem.h"

#include "clibext.h"


//
//  FutureHash  --A quick structure to record unresolved hotlinks.
//

struct FutureHash
{
    Buffer<char>    _string;
    FutureHash      *_prev;
    FutureHash      *_next;

    FutureHash( size_t len ) : _string( len ), _next( NULL ) {};
};


//
//  ContextKey  --"Key" data to store in the b-tree.
//

class ContextKey : public BtreeData
{
protected:
    uint_32     _hashValue;

    // Redefinitions of the BtreeData virtual functions.
    BtreeData       *myKey();
    bool            lessThan( BtreeData *other );
    virtual uint_32 size() { return sizeof( uint_32 ); };
    virtual int     dump( OutFile * dest );

public:
    ContextKey( uint_32 hval ) : _hashValue( hval ) {};
};


//
//  ContextRec  --"Record" data to store in the b-tree.
//

class ContextRec : public ContextKey
{
    uint_32 _offset;

    // Redefinitions of the BtreeData virtual functions.
    uint_32 size() { return 2*sizeof( uint_32 ); };
    int     dump( OutFile * dest );

public:
    ContextRec( uint_32 hval, uint_32 off )
        : ContextKey( hval ), _offset( off ) {};

    uint_32 offset() { return _offset ; };
};


//  ContextKey::myKey       --Overrides BtreeData::myKey

BtreeData *ContextKey::myKey()
{
    return new ContextKey( _hashValue );
}


//  ContextKey::lessThan    --Overrides BtreeData::lessThan

bool ContextKey::lessThan( BtreeData *other )
{
    ContextKey  *true_other = (ContextKey*) other;

    // WinHelp hash values are signed ints, and are sorted as such.
    return( (int_32)_hashValue < (int_32)true_other->_hashValue );
}


//  ContextKey::dump        --Overrides BtreeData::dump

int ContextKey::dump( OutFile * dest )
{
    dest->write( _hashValue );
    return 1;
}


//  ContextRec::dump        --Overrides BtreeData::dump

int ContextRec::dump( OutFile * dest )
{
    dest->write( _hashValue );
    dest->write( _offset );
    return 1;
}


//  HFContext::HFContext

HFContext::HFContext( HFSDirectory * d_file )
    : _head( NULL ),
      _tail( NULL )
{
    _data = new Btree( false, "L4" );
    d_file->addFile( this, "|CONTEXT" );
}


//  HFContext::~HFContext

HFContext::~HFContext()
{
    delete _data;
    FutureHash  *current;
    FutureHash  *next;
    for( current = _head; current != NULL; current = next ) {
        next = current->_next;
        delete current;
    }
}

const uint_32 HFContext::_badValue = (uint_32)~0L;


//  HFContext::addOffset    --Add a new entry to the btree.

void HFContext::addOffset( uint_32 hval, uint_32 off )
{
    // Check to see if we have a reference to this hash value.
    FutureHash  *current;
    for( current = _head; current != NULL; current = current->_next ) {
        if( Hash( current->_string ) == hval ) {
            break;
        }
    }

    // If a reference exists, delete it.
    if( current != NULL ) {
        if( current->_prev != NULL ) {
            current->_prev->_next = current->_next;
        }
        if( current->_next != NULL ) {
            current->_next->_prev = current->_prev;
        }
        if( _head == current ) {
            _head = current->_next;
        }
        if( _tail == current ) {
            _tail = current->_prev;
        }
        delete current;
    }
    _data->insert( new ContextRec( hval, off ) );

    return;
}


//  HFContext::getOffset    --Lookup an entry from in the b-tree.

uint_32 HFContext::getOffset( uint_32 hval )
{
    uint_32 result = _badValue;
    ContextKey  searchkey( hval );
    ContextRec  *foundrec = (ContextRec*) _data->findNode( searchkey );

    if( foundrec != NULL ) {
        result = foundrec->offset();
    }
    return result;
}


//  HFContext::recordContext    --Record a forward reference to a
//                context string.

void HFContext::recordContext( char const str[] )
{
    uint_32 h_value = Hash( str );
    ContextKey  temp_key(h_value);
    BtreeData   *c_rec = _data->findNode( temp_key );

    // Continue only if the topic is not yet defined.
    if( c_rec != NULL )
        return;

    FutureHash  *current;

    // Check to see if this topic has already been referenced.
    for( current = _head; current != NULL; current = current->_next ) {
        int comparison = stricmp( current->_string, str );
        if( comparison == 0 )   // string was already in list
            return;
        if( comparison > 0 ) {
            break;
        }
    }

    // If this topic has not been referenced or defined before,
    // add it to the list of references.
    FutureHash  *newnode = new FutureHash( strlen( str ) + 1 );
    strcpy( newnode->_string, str );
    if( current == NULL ) {
        newnode->_prev = _tail;
        _tail = newnode;
        if( _head == NULL ) {
            _head = newnode;
        } else {
            newnode->_prev->_next = newnode;
        }
    } else {
        newnode->_next = current;
        newnode->_prev = current->_prev;
        current->_prev = newnode;
        if( newnode->_prev == NULL ) {
            _head = newnode;
        } else {
            newnode->_prev->_next = newnode;
        }
    }
    return;
}


//  HFContext::dump --Overrides Dumpable::dump

int HFContext::dump( OutFile * dest )
{
    FutureHash  *current;
    for( current = _head; current != NULL; current = current->_next ) {
        HCWarning( HLP_NOTOPIC, (const char *)current->_string );
    }
    return _data->dump( dest );
}
