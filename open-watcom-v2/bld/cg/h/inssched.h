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


typedef struct FU_entry {
    unsigned short      good_fu;
    byte                unit_stall;
    byte                opnd_stall;
} FU_entry;

typedef struct data_dag {
    struct data_dag             *prev;
    instruction                 *ins;
    struct dep_list_entry       *deps;
    struct data_dag             *ready;
    unsigned                    stallable       : 8;
    unsigned                    visited         : 1;
    unsigned                    scheduled       : 1;
    unsigned                    height;
    unsigned                    anc_count;
} data_dag;

typedef struct dep_list_entry {
    struct dep_list_entry   *next;
    data_dag                *dep;
} dep_list_entry;


extern data_dag         *DataDag;   /* global for dump routines */

extern const FU_entry   *FUEntry( instruction *ins );
extern void             Schedule( void );
extern bool             SchedFrlFree( void );
extern bool             InsOrderDependant( instruction *ins_i, instruction *ins_j );
