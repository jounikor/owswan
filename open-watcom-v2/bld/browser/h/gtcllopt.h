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


#ifndef __GTCLLOPT_H__
#define __GTCLLOPT_H__

#include "dgcllopt.gh"
#include "gtlnopt.h"
#include "wbrwin.h"

class WDefPushButton;
class WPushButton;
class GTFunctionOpts;

class GTCallOption : public CallOptDlg, public GTLineEditor, public WDialog
{
public:
                                GTCallOption( const GTFunctionOpts & );
                                ~GTCallOption();

        virtual void            initialize();

                void            okButton( WWindow * );
                void            cancelButton( WWindow * );
                void            modifyButton( WWindow * );
                void            helpButton( WWindow * );
                bool            contextHelp( bool );

                int             inRect( int x, int y );

        virtual void            setInfo( PaintInfo * );
        virtual void            endEdit();

        virtual bool            paint();
        virtual bool            leftBttnDn( int, int, WMouseKeyFlags );
        virtual bool            leftBttnDbl( int, int, WMouseKeyFlags );
        virtual bool            keyDown( WKeyCode, WKeyState );
private:
                GTFunctionOpts *    _options;
                WRect *             _rects;
                int                 _hasFocus;

                WDefPushButton *    _okButton;
                WPushButton *       _cancelButton;
                WPushButton *       _modifyButton;
                WPushButton *       _helpButton;
};

#endif // __GTCLLOPT_H__
