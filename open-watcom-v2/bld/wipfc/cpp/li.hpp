/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2009-2018 The Open Watcom Contributors. All Rights Reserved.
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
* Description: Base for list item specializations
*
*   :li
*   Must be child of :ol, :sl, or :ul
*
****************************************************************************/

#ifndef LI_INCLUDED
#define LI_INCLUDED

#include "tag.hpp"

class Li : public Tag {
public:
    Li( Document* d, Element *p, const std::wstring* f, unsigned int r,
        unsigned int c, unsigned int n, byte l, byte i, bool cmp ) :
        Tag( d, p, f, r, c ), _itemNumber( n ), _nestLevel( l ), _indent( i ),
        _compact( cmp ) { };
    ~Li() { };
private:
    Li( const Li& rhs );                //no copy
    Li& operator=( const Li& rhs );     //no assignment

protected:
    unsigned int        _itemNumber;    //counts from 0
    byte                _nestLevel;     //counts from 0
    byte                _indent;        //in character spaces
    bool                _compact;       //the list is 'compact'
};

#endif //LI_INCLUDED

