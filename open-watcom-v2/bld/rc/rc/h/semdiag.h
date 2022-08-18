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
* Description:  Semantic actions for processing dialog resources.
*
****************************************************************************/


#ifndef SEMDIAG_INCLUDED
#define SEMDIAG_INCLUDED

typedef struct DlgHeader32 {
    DialogBoxHeader32           Head;
    DialogBoxExHeader32short    ExHead;
} DlgHeader32;

typedef struct FullDialogBoxHeader {
    bool                Win32;
    union {
        DialogBoxHeader     Head;
        DlgHeader32         Head32;
    } u;
    bool                StyleGiven;
} FullDialogBoxHeader;

typedef struct DlgControl32 {
    uint_32             Style;
    uint_32             ExtendedStyle;
    DialogSizeInfo      SizeInfo;
    uint_32             ID;
    ControlClass        *ClassID;
    ResNameOrOrdinal    *Text;
    uint_16             ExtraBytes;         /* should be 0 */
    uint_32             HelpId;             /* only used for Dialogex */
    bool                HelpIdDefined;      /* only used for Dialogex */
}DlgControl32;

typedef struct FullDialogBoxControl {
    struct FullDialogBoxControl     *next;
    struct FullDialogBoxControl     *prev;
    bool                            Win32;
    union {
        DialogBoxControl            ctrl;
        DlgControl32                ctrl32;
    } u;
    DataElemList                    *dataListHead;
} FullDialogBoxControl;

typedef struct FullDiagCtrlList {
    FullDialogBoxControl            *head;
    FullDialogBoxControl            *tail;
    uint_16                         numctrls; /* Win16 only support upto 255 controls. */
} FullDiagCtrlList;

typedef struct FullDiagCtrlOptions {
    DialogSizeInfo          SizeInfo;
    IntMask                 Style;
    uint_16                 ID;
    ResNameOrOrdinal        *Text;
    uint_32                 ExtendedStyle;
    uint_32                 HelpId;
    bool                    HelpIdDefined;
} FullDiagCtrlOptions;

typedef struct DlgHelpId {
    uint_32     HelpId;
    bool        HelpIdDefined;
} DlgHelpId;

typedef struct PresParamsOS2 {
    struct PresParamsOS2    *next;
    struct PresParamsOS2    *prev;
    ResNameOrOrdinal        *Name;
    DataElemList            *dataList;
    uint_32                 size;
} PresParamsOS2;

typedef struct PresParamListOS2 {
    PresParamsOS2           *head;
    PresParamsOS2           *tail;
    uint_32                 size;
} PresParamListOS2;

typedef struct FullDialogBoxControlOS2 {
    struct FullDialogBoxControlOS2  *next;
    struct FullDialogBoxControlOS2  *prev;
    DialogBoxControl                ctrl;
    DataElemList                    *dataListHead;
    PresParamListOS2                *presParams;
    struct FullDiagCtrlListOS2      *children;
    DialogTemplateItemOS2           *tmpl;
    uint_32                         framectl;
} FullDialogBoxControlOS2;

typedef struct FullDiagCtrlListOS2 {
    FullDialogBoxControlOS2         *head;
    FullDialogBoxControlOS2         *tail;
    uint_8                          numctrls;
} FullDiagCtrlListOS2;

typedef struct FullDiagCtrlOptionsOS2 {
    DialogSizeInfo          SizeInfo;
    IntMask                 Style;
    uint_16                 ID;
    ResNameOrOrdinal        *Text;
} FullDiagCtrlOptionsOS2;

#endif
