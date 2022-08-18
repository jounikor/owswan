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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#ifndef __GTROOTPK_H__
#define __GTROOTPK_H__

#include "gtwin.h"
#include "outline.h"
#include "edmodlst.h"

class OutlineRootElement : public ModuleItem
{
public:
                        OutlineRootElement( OutlineElement * elm );
        virtual         ~OutlineRootElement();

                void    okPressed();

    OutlineElement *    _element;
};

class TreeRootElement : public ModuleItem
{
public:
                        TreeRootElement( TreeRoot * root );
        virtual         ~TreeRootElement();

                void    okPressed();

    TreeRoot *          _root;
};

class TreeRootSelect : public EditModuleList
{
public:
                        TreeRootSelect( WWindow * prt, TreeRootList * roots );
                        ~TreeRootSelect();

protected:
    virtual void        okButton( WWindow * );
    virtual void        helpButton( WWindow* );
            bool        contextHelp( bool );

    virtual void        loadBox();

private:
        TreeRootList *  _roots;
};

class OutlineRootSelect : public EditModuleList
{
public:
                        OutlineRootSelect( WWindow * prt, OutlineElement * root );
                        ~OutlineRootSelect();

protected:
    virtual void        okButton( WWindow * );
    virtual void        helpButton( WWindow* );
            bool        contextHelp( bool );

    virtual void        loadBox();

private:
        OutlineElement *    _root;
};

#endif // __GTROOTPK_H__
