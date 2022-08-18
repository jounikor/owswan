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
//  phrase.hpp  Phrase replacement algorithm.
//
#ifndef PHRASE_H
#define PHRASE_H

#include <string.h>
#include "scanning.h"
#include "hlpdir.h" // for class HFSDirectory.


// Forward declarations.
class PTable;
struct P_String;


//
//  HFPhrases   --Class to create the |Phrases file in a windows help file.
//

class HFPhrases : public Dumpable
{
    PTable              *_oldPtable;
    PTable              *_newPtable;

    Buffer<P_String *>  _result;
    Buffer<P_String *>  _hptable;
    unsigned            _resultSize;

    uint_16             _numPhrases;
    uint_32             _phSize;
    uint_32             _size;

    bool                (*_nextf)( InFile * );
    bool                (*_firstf)( InFile * );

    Scanner             *_scanner;
    void                startInput( InFile * );
    char                *nextInput( InFile * );
    void                initHashPTable();

    // Assignment of HFPhrases is not allowed.
    HFPhrases( HFPhrases const & ) : _result( 0 ), _hptable( 0 ) {};
    HFPhrases & operator=( HFPhrases const & ) { return *this; };

    void                removeScanner();

public:
    HFPhrases( HFSDirectory *d_file, bool (*firstf)(InFile *), bool (*nextf)(InFile *) );
    ~HFPhrases();

    // Overrides of the "Dumpable" virtual functions.
    uint_32             size();
    int                 dump( OutFile * dest );

    // Functions of the phrase handler.
    void                readPhrases();
    void                createQueue( char const *path );
    int                 oldTable( char const *path );

    void                replace( char *dst, char const *src, size_t &len );
};

#endif
