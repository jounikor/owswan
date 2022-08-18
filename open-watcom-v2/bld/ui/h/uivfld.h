/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2019 The Open Watcom Contributors. All Rights Reserved.
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


#ifndef _UIVFLD_H_
#define _UIVFLD_H_

typedef struct vfield_edit {
    struct vfield_edit  _FARD *link;    /* next vfield on the vscreen      */
    ORD                 row;            /* position of vfield on vscreen   */
    ORD                 col;            /* ...                             */
    unsigned            length;         /* length of field on vscreen      */
} VFIELD_EDIT;

typedef struct vfieldedit {
    VFIELD_EDIT _FARD *fieldlist;   /* first vfield in linked list of vfields  */
    VFIELD_EDIT _FARD *curfield;    /* current vfield (might be NULL)          */
    VFIELD_EDIT _FARD *prevfield;   /* last vfield (might be NULL)             */
    ATTR        enter_attr;         /* attribute used to echo current field    */
    ATTR        exit_attr;          /* used when cursor leaves current field   */
    char        _FARD *buffer;      /* buffer of characters being editted      */
                                    /*                           SET  RESET    */
    boolbit     oktomodify   :1;    /* user can edit buffer      app   app     */
    boolbit     hidden       :1;    /* echo only cursor on XXX   app   app     */
    boolbit     delpending   :1;    /* rubout key entered field  ui    ui      */
    boolbit     fieldpending :1;    /* a field was just entered  ui    ui      */
    boolbit     dirty        :1;    /* altered buffer contents   ui    app     */
    boolbit     update       :1;    /* new buffer or contents    app   ui      */
    boolbit     cursor       :1;    /* cursor changed            app   ui      */
    boolbit     cancel       :1;    /* cancel field change       app   ui      */
    boolbit     reset        :1;    /* changed field list or     app   ui      */
                                    /* first time through                      */
} VFIELDEDIT;

/*                                                                         */
/* when the application makes any unexpected change to the field editting  */
/* information, it must indicate that it has done so by setting the        */
/* appropriate flag                                                        */
/*                                                                         */
/* when an EV_FIELD_CHANGE event occurs, the application can change:       */
/*                 the screen attributes                                   */
/*                 the oktomodify flag                                     */
/*                 the buffer and/or its contents                          */
/*                                                                         */
/* NOTE: the application cannot change curfield or prevfield directly      */
/*       it must do so by setting the VSCREEN cursor position and          */
/*       setting the cursor flag                                           */
/*                                                                         */

extern ui_event     UIAPI uivfieldedit( VSCREEN *, VFIELDEDIT * );

#endif
