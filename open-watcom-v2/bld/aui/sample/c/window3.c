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
* Description:  WHEN YOU FIGURE OUT WHAT THIS MODULE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/



#include "app.h"


static gui_menu_struct W3PopUp[] = {
    { "Default Popup",      MENU_W3_POPUP,  GUI_STYLE_MENU_ENABLED },
    { "Show Lisa The Bug",  MENU_W3_BUG,    GUI_STYLE_MENU_ENABLED },
};

static char *Stuff[] =
{
#if 0
"With a Pentium compiler writer's guide in one hand, and a simulator",
"listing in the other, we set out to make WATCOM C/C++ 32 into a Pentium",
"aware compiler.  It looked easy at first glance, but as we got into it,",
"some interesting challenges emerged.  These are a few of things that we",
"learned.",
"",
"Instruction Scheduling",
"======================",
"",
"Because of the Pentium's multiple execution pipes, instruction",
"scheduling, or reordering, is by far the most important optimization",
"performed by the compiler.  A three phase approach to scheduling emerged",
"as the strategy of choice given the framework of the code genererator.",
"",
"First, 'RISC-ification'.  That is, reduction of integer instructions to",
"use the RISC subset of the instruction set.  For example, we turn an add",
"from memory into a load followed by an add from register.  The two",
"instructions are a bit larger, but no slower, and they give you more",
"freedom to choose an optimal instruction ordering.",
"",
"Second, 'scheduling'.  Move data dependant instructions so that they are",
"not adjacent.  Ideally, a result shouldn't be used until the instruction",
"has had enough time to complete all pipeline stages.  This allows your",
"code to take full advantage of the superscalar architecture, and leads",
"to considerable 486 execution speed improvement.",
"",
"Finally, 're-CISC-ification'.  Build instructions back up to take",
"advantage of complex instructions.  Often, data dependencies do not",
"allow scheduling in certain portions of the code.  You might turn  the",
"load/add sequence created in first phase back into an add from memory,",
"if there was no scheduling benefit.",
"",
"Quirks",
"=======",
"",
"There are some interesting architectural anomalies which can materially affect Pentium",
"performance."
"In order to reference 16 bit items, an operand size prefix has to",
"be used.  We do our best to eliminate these prefix bytes if",
"possible, but it's still a good idea to stay away from short integers.",
"",
"Floating-point instructions must also be scheduled to take full",
"advantage of the float pipe.  The FXCH instruction was designed to make",
"this easier.  It can execute simultaneously with other floating-point",
"instructions.  Since one operand of a floating-point instruction must be",
"ST(0), a free FXCH instruction allows floating-point sequences to be",
"scheduled for better throughput.  Two data independant sequences are",
"interleaved, and FXCH is inserted when an intermediate result needs to",
"be brought to the top of the stack.",
"",
"It sounds great, but before you re-write your floating point library to",
"take full advantage of the free FXCH instruction, you need to read the",
"fine print.  There are conditions that must be met before instruction",
"pairing occurs.",
"",
"First, the FXCH must be followed by another floating-point instruction.",
"Secondly, it must be preceded by a floating-point instruction which does",
"not pop the stack."
"If these",
"conditions are not met, your code may actually run slower, since the",
"FXCH instructions each take one clock cycle.",
"",
"Finally, here's something you might run across in the processor",
"documentation.  'Performing a floating-point operation on a memory",
"operand instead of on a stack register adds no cycles'.  True, assuming",
"a cache hit.  However, there is a clock penalty for cache misses."
"Don't be fooled by the clock cycles.  Keep integer and",
"floating-point values in registers wherever possible.",
"",
"386/486 considerations",
"==================",
"",
"The 486 only has one integer pipe, and one float pipe, but it is still a",
"pipelined architecture.  An optimal Pentium ordering is usually optimal,",
"or nearly so, on the 486 as well.  The scheduling avoids pipeline",
"stalls, greatly increasing 486 performance.",
"",
"There is one major compromise which must be made if code is to be",
"optimized for both the Pentium and the 386/486.  In this case, you don't",
"use the FXCH instruction to achieve parallelism.  It may speed up",
"Pentium code considerably, but will not be free on a 386/486.",
"",
"16 bit code",
"===========",
"",
"Remember to optimize your 16-bit code for the Pentium as well.  All the",
"same rules apply, except that 32-bit operands now require a prefix byte.",
"Since WATCOM C uses a common code generator to produce both 16 and",
"32-bit code, WATCOM C/C++ 16 became Pentium aware for 'free'.  We",
"re-compiled a suite of small benchmarks programs, and were amazed when",
"we saw a 40% average speed increase, with improvements ranging up to a",
"factor of two in execution speed.",
"",
"Conclusion",
"==========",
"",
"The superscalar Pentium aware compiler described here is available",
"today.  With it you can create a singled .EXE (16 or 32-bit) that",
"delivers significant performance benefits on the 486, and is ready to",
"really fly on the Pentium processor.",
#else
"        title = �R�}���h�E�v�����v�g",
"        help = DOS �R�}���h�E�v�����v�g���N����, �R�}���h���͏�Ԃɂ��܂��B^m^m�R�}���h�E���C������DOS �V�F���ɖ߂邽�߂ɂ�:^m^m1. exit �Ɠ��͂�, ^m2. Enter(���s)�L�[�������Ă��������B^m^m�֘A����"
"        pause = disabled",
"    }",
"    special = default",
"    program = ",
"    {",
"        command = SWITCH",
"        title = �p�ꃂ�[�h�֐؂�ւ�",
"        help = ���{�ꃂ�[�h����p�ꃂ�[�h�֐؂�ւ��܂��B�^�X�N�؂�ւ����g�p���ɂ͎��s���Ȃ��ł��������B",
"        pause = disabled",
"    }",
"    program = ",
"    {",
"        command = E %1",
"        title = PC DOS E �G�f�B�^�[",
"        help = �e�L�X�g�E�t�@�C����ҏW���� PC DOS E �G�f�B�^�[���N�����܂��B���̃G�f�B�^�[��I�ԂƕҏW����t�@�C��������͂���p�l�����\������܂��B^m^m�֘A����^m   "
"        pause = disabled",
"        dialog = ",
"        {",
"            title = �ҏW�t�@�C�����̓���",
"            info = �t�@�C��������͂��ĉ������B�t�@�C�����I�[�v�������ɃG�f�B�^�[���N������ꍇ�́AEnter�L�[�������ĉ������B",
"            prompt = �ҏW�t�@�C����?",
"            parameter = %1",
"        }",
"    }",
"    program = ",
"    {",
"        help = �X�P�W���[���[���g�p����Ύw�肵�������Ɏw�肵���v���O���������s���邱�Ƃ��ł��܂��BDOS�v�����v�g����N���ł���v���O�����ł���΂ǂ�ȃv���O�����ł��w�肷�邱�Ƃ��ł��܂��B�������ADOSSHELL ���N������O�� CPSCHED.EXE(TSR) ���풓�����Ă����K�v������܂��B",
"        screenmode = text",
"        alttab = enabled",
"        altesc = enabled",
"        ctrlesc = enabled",
"        prevent = disabled",
"        command = SCHEDULE",
"        title = Central Point�X�P�W���[���[",
"        pause = disabled",
"    }",
"    program = ",
"    {",
"        help = �t���E�X�N���[���� UNDELETE �v���O�������N�����A�ȑO�ɍ폜�����t�@�C������уf�B���N�g���[�𕜌����܂��B"
"        screenmode = text",
"        alttab = enabled",
"        altesc = enabled",
"        ctrlesc = enabled",
"        prevent = disabled",
"        command = UNDELETE",
"        title = Central Point�A���f���[�g",
"        pause = disabled",
"    }",
"    group = ",
"    {",
"        title = �f�B�X�N�E���[�e�B���e�B�[",
"        help = �f�B�X�N�Ǘ��̂��߂̃��[�e�B���e�B�[�̃v���O�����E�A�C�e����\�����܂��B�܂�, ���C���E�O���[�v��ǉ������O���[�v���I�[�v�����邱�Ƃ��ł��܂��B",
"        program = ",
"        {",
"            command = diskcopy %1",
"            title = �f�B�X�P�b�g�̕���",
"            pause = enabled",
"            dialog = ",
"            {",
"                title = �f�B�X�P�b�g�̕���",
"                info = ���ʌ�����ѕ��ʐ�h���C�u����͂��Ă��������B",
"                prompt = �p�����[�^�[ . . .",
"                default = a: b:",
"                parameter = %1",
"            }",
"        }",
"        program = ",
"        {",
"            command = format %1 /q",
"            title = �N�C�b�N�E�t�H�[�}�b�g",
"            pause = enabled",
"            dialog = ",
"            {",
"                title = �N�C�b�N�E�t�H�[�}�b�g",
"                info = �N�C�b�N�E�t�H�[�}�b�g����h���C�u����͂��Ă��������B",
"                prompt = �p�����[�^�[ . . .",
"                default = a:",
"                parameter = %1",
"            }",
"            screenmode = text",
"            alttab = enabled",
"            altesc = enabled",
"            ctrlesc = enabled",
"            prevent = enabled",
"        }",
"        program = ",
"        {",
"            command = format %1",
"            title = �t�H�[�}�b�g",
"            pause = enabled",
"            dialog = ",
"            {",
"                title = �t�H�[�}�b�g",
"                info = �t�H�[�}�b�g����h���C�u����͂��Ă��������B",
"                prompt = �p�����[�^�[ . . .",
"                default = a:",
"                parameter = %1",
"            }",
"        }",
"        program = ",
"        {",
"            command = undelete %1",
"            title = �t�@�C���̕���",
"            pause = enabled",
"            dialog = ",
"            {",
"                title = �t�@�C���̕���",
"                info = ���ӁI���̃R�}���h�͍폜�����t�@�C���𕜌��ł��Ȃ����Ƃ�����܂��B�ڂ�����, F1�L�[�������Ă��������B",
"                prompt = �p�����[�^�[ . . .",
"                default = /LIST",
"                parameter = %1",
"            }",
"            screenmode = text",
#endif
};

static wnd_row TheSize = ArraySize( Stuff );

static void W3MenuItem( a_window wnd, gui_ctl_id id, wnd_row row, wnd_piece piece )
{
    row=row;piece=piece;
    switch( id ) {
    case MENU_W3_POPUP:
        Say2( "Default Popup", WndPopItem( wnd ) );
        break;
    case MENU_W3_BUG:
        TheSize = TheSize == ArraySize( Stuff ) ? 10 : ArraySize( Stuff );
        WndSetRepaint( wnd );
        break;
    }
}

static void W3Modify( a_window wnd, wnd_row row, wnd_piece piece )
{
    W3MenuItem( wnd, 0, row, piece );
}


static wnd_row W3NumRows( a_window wnd )
{
    wnd=wnd;
    return( TheSize );
}

static bool W3GetLine( a_window wnd, wnd_row row, wnd_piece piece, wnd_line_piece *line )
{
    wnd=wnd;
    if( row >= TheSize )
        return( false );
    if( piece != 0 )
        return( false );
    line->text = Stuff[row];
    line->tabstop = true;
//  line->extent = WND_MAX_EXTEND;
    return( true );
}


static void W3Refresh( a_window wnd )
{
    WndSetRepaint( wnd );
}

static wnd_info W3Info = {
    NoWndEventProc,
    W3Refresh,
    W3GetLine,
    W3MenuItem,
    NoVScroll,
    NoBegPaint,
    NoEndPaint,
    W3Modify,
    W3NumRows,
    NoNextRow,
    NoNotify,
    NoChkUpdate,
    PopUp( W3PopUp )
};

a_window W3Open( void )
{
    a_window    wnd;
    wnd_create_struct   info;

    WndInitCreateStruct( &info );
    info.info = &W3Info;
    wnd = WndCreateWithStruct( &info );
    if( wnd != NULL ) {
        WndSetSwitches( wnd, WSW_LBUTTON_SELECTS | WSW_CHAR_CURSOR );
        WndClrSwitches( wnd, WSW_HIGHLIGHT_CURRENT );
    }
    return( wnd );
}
