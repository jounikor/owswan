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


//
//  btrees.h    --Classes to support WinHelp-style b-trees, for the Watcom
//                .HLP compiler
//
#ifndef BTREES_H
#define BTREES_H

#include "myfile.h"

struct BtreePage;    // forward declaration.


//
//  BtreeData   --Abstract base class to hold tree data.
//

class BtreeData
{
    BtreeData   *_bnext;
    BtreeData   *_bprev;
    BtreePage   *_child;    // The page below this data.

    // Assignment of BtreeData's (or derived classes) is not permitted.
    BtreeData( BtreeData const & ) {};
    BtreeData & operator=( BtreeData const & ) { return *this; };
protected:
    BtreeData();
public:
    virtual ~BtreeData() {};

    // Assorted access functions.
    BtreeData   *bnext() { return _bnext; };
    BtreeData   *bnext( BtreeData *b ) { _bnext = b; return b; };
    BtreeData   *bprev() { return _bprev; };
    BtreeData   *bprev( BtreeData *b ) { _bprev = b; return b; };
    BtreePage   *child() { return _child; };
    BtreePage   *child( BtreePage *c ) { _child = c; return c; };

    void        insertSelf( BtreePage *dest );
    BtreePage   *seekNext( BtreePage *first );

    // Virtual functions to override in a derived class.
    virtual BtreeData   *myKey() = 0;
    virtual bool        lessThan( BtreeData *other ) = 0;

    virtual uint_32     size() = 0;
    virtual int         dump( OutFile * dest ) = 0;
};


//
//  Btree       --The tree class.
//

#define BTREE_HEADER_SIZE   38

class Btree : public Dumpable
{
    uint_16     _numLevels;     // Number of levels.
    uint_32     _totalEntries;  // Number of entries in leaf nodes.
    uint_16     _numPages;      // Number of pages.
    uint_16     _numSplits;     // Number of page splits.
    uint_32     _size;          // Total size of the structure.
    uint_32     _pageSize;      // Page size.
    char const  *_format;       // The format string for the btree.
    uint_16     _flags;         // The flags for the btree.

    // Some recursive functions to act on the tree.
    void        labelPages( BtreePage *start );
    void        dumpPage( OutFile *dest, BtreePage *start );
    void        killPage( BtreePage *start );

    BtreePage   *_root;

    // Assignment of Btree's is not permitted.
    Btree( Btree const & ) {};
    Btree & operator=( Btree const & ) { return *this; };

public:
    Btree( bool dir, char const *format );
    ~Btree();

    void        insert( BtreeData *newdata );
    BtreeData   *findNode( BtreeData &keyval );

    uint_32     size();                 // Overrides Dumpable::size
    int         dump( OutFile *dest );  // Overrides Dumpable::dump

    friend class BtreeIter;
};


//
//  BtreeIter   --A tree iterator.
//

class BtreeIter
{
    Btree       *_tree;
    BtreePage   *_page;
    BtreeData   *_data;
    void        goPrev();
    void        goNext();
public:
    BtreeIter( Btree &t );
    BtreeData   *data() { return _data; };

    void        init();
    BtreeIter&  operator--();       // prefix
    void        operator--(int);    // postfix
    BtreeIter&  operator++();       // prefix
    void        operator++(int);    // postfix

    // I would LOVE for pages to completely transparent to everything
    // outside of this module, but the (expletive deleted) |KWMAP file
    // needs info about the pages of |KWBTREE.  So...
    uint_16     pageEntries();
    uint_16     thisPage();
    void        nextPage();
};

#endif
