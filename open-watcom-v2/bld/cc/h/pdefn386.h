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


#define NC '\0'

hw_reg_set DefaultVarParms[] = {
    HW_D( HW_EMPTY )
};

#define REG_DEFS \
REG_PICK( ES,  "es",  0  ) \
REG_PICK( DS,  "ds",  1  ) \
REG_PICK( CS,  "cs",  2  ) \
REG_PICK( SS,  "ss",  3  ) \
REG_PICK( AX,  "ax",  4  ) \
REG_PICK( BX,  "bx",  5  ) \
REG_PICK( CX,  "cx",  6  ) \
REG_PICK( DX,  "dx",  7  ) \
REG_PICK( AL,  "al",  8  ) \
REG_PICK( BL,  "bl",  9  ) \
REG_PICK( CL,  "cl",  10 ) \
REG_PICK( DL,  "dl",  11 ) \
REG_PICK( AH,  "ah",  12 ) \
REG_PICK( BH,  "bh",  13 ) \
REG_PICK( CH,  "ch",  14 ) \
REG_PICK( DH,  "dh",  15 ) \
REG_PICK( SI,  "si",  16 ) \
REG_PICK( DI,  "di",  17 ) \
REG_PICK( BP,  "bp",  18 ) \
REG_PICK( SP,  "sp",  19 ) \
REG_PICK( FS,  "fs",  20 ) \
REG_PICK( GS,  "gs",  21 ) \
REG_PICK( EAX, "eax", 22 ) \
REG_PICK( EBX, "ebx", 23 ) \
REG_PICK( ECX, "ecx", 24 ) \
REG_PICK( EDX, "edx", 25 ) \
REG_PICK( ESI, "esi", 26 ) \
REG_PICK( EDI, "edi", 27 ) \
REG_PICK( EBP, "ebp", 28 ) \
REG_PICK( ESP, "esp", 29 )

char Registers[] = {
    #define REG_PICK(c,t,r) t "\0"
    REG_DEFS
    "\0"
    #undef REG_PICK
};

unsigned char RegMap[] = {
    #define REG_PICK(c,t,r) r,
    REG_DEFS
    #undef REG_PICK
};

hw_reg_set RegBits[] = {
    #define REG_PICK(c,t,r) HW_D( HW_##c ),
    REG_DEFS
    #undef REG_PICK
};

/*      INLINE FUNCTIONS */

aux_info        InlineInfo;

/*
  here come the code bursts ...
*/

#include "code386.gh"

static hw_reg_set P_EAX_CL[] = {
    HW_D( HW_EAX ),
    HW_D( HW_CL ),
    HW_D( HW_EMPTY ) };

static hw_reg_set P_EAX_ECX[] = {
    HW_D( HW_EAX ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY ) };

static hw_reg_set P_EDI_ESI[] = {
    HW_D( HW_EDI ),
    HW_D( HW_ESI ),
    HW_D( HW_EMPTY ) };

static hw_reg_set P_EDI_ESI_ECX[] = {
    HW_D( HW_EDI ),
    HW_D( HW_ESI ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_ESI_EDI_ECX[] = {
    HW_D( HW_ESI ),
    HW_D( HW_EDI ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EDI_AX_ECX[] = {
    HW_D( HW_EDI ),
    HW_D( HW_AX ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EDI_EAX_ECX[] = {
    HW_D( HW_EDI ),
    HW_D( HW_EAX ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EDI_AL_ECX[] = {
    HW_D( HW_EDI ),
    HW_D( HW_AL ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_ESEDI_DSESI[] = {
    HW_D_2( HW_ES, HW_EDI ),
    HW_D_2( HW_DS, HW_ESI ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DXEDI_CL[] = {
    HW_D_2( HW_DX, HW_EDI ),
    HW_D( HW_CL ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DXEDI_CXESI_EAX[] = {
    HW_D_2( HW_DX, HW_EDI ),
    HW_D_2( HW_CX, HW_ESI ),
    HW_D( HW_EAX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DXESI_CXEDI_EAX[] = {
    HW_D_2( HW_DX, HW_ESI ),
    HW_D_2( HW_CX, HW_EDI ),
    HW_D( HW_EAX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DXEAX_CXEBX[] = {
    HW_D_2( HW_DX, HW_EAX ),
    HW_D_2( HW_CX, HW_EBX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_ESEDI_DSESI_ECX[] = {
    HW_D_2( HW_ES, HW_EDI ),
    HW_D_2( HW_DS, HW_ESI ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_ESEDI_AL_ECX[] = {
    HW_D_2( HW_ES, HW_EDI ),
    HW_D( HW_AL ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DXEDI_AL_ECX[] = {
    HW_D_2( HW_DX, HW_EDI ),
    HW_D( HW_AL ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_ESEDI_AX_ECX[] = {
    HW_D_2( HW_ES, HW_EDI ),
    HW_D( HW_AX ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_ESEDI_EAX_ECX[] = {
    HW_D_2( HW_ES, HW_EDI ),
    HW_D( HW_EAX ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DXEDI_AX_ECX[] = {
    HW_D_2( HW_DX, HW_EDI ),
    HW_D( HW_AX ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DXEDI_EAX_ECX[] = {
    HW_D_2( HW_DX, HW_EDI ),
    HW_D( HW_EAX ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_ESI_EDI[] = {
    HW_D( HW_ESI ),
    HW_D( HW_EDI ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DSESI_ESEDI[] = {
    HW_D_2( HW_DS, HW_ESI ),
    HW_D_2( HW_ES, HW_EDI ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_SIEAX_CXEDI[] = {
    HW_D_2( HW_SI, HW_EAX ),
    HW_D_2( HW_CX, HW_EDI ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DSESI_ESEDI_ECX[] = {
    HW_D_2( HW_DS, HW_ESI ),
    HW_D_2( HW_ES, HW_EDI ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_DSESI_CL[] = {
    HW_D_2( HW_DS, HW_ESI ),
    HW_D( HW_CL ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_ESI_DL[] = {
    HW_D( HW_ESI ),
    HW_D( HW_DL ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EDI[] = {
    HW_D( HW_EDI ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_ESEDI[] = {
    HW_D_2( HW_ES, HW_EDI ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_CXEDI[] = {
    HW_D_2( HW_CX, HW_EDI ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EAX[] = {
    HW_D( HW_EAX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EDX[] = {
    HW_D( HW_EDX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EDX_AL[] = {
    HW_D( HW_EDX ),
    HW_D( HW_AL ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EDX_EAX[] = {
    HW_D( HW_EDX ),
    HW_D( HW_EAX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EAX_EDX[] = {
    HW_D( HW_EAX ),
    HW_D( HW_EDX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EAX_ESI_EDX_EDI_ECX[] = {
    HW_D( HW_EAX ),
    HW_D( HW_ESI ),
    HW_D( HW_EDX ),
    HW_D( HW_EDI ),
    HW_D( HW_ECX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_EDXEAX[] = {
    HW_D_2( HW_EDX, HW_EAX ),
    HW_D( HW_EMPTY )
};

static hw_reg_set P_8087[] = {
    HW_D( HW_FLTS ),
    HW_D( HW_EMPTY )
};

const alt_inline_funcs FlatAlternates[] = {
    (byte_seq *)&S_memset,    { "memset", (byte_seq *)&FS_memset, FS_memset_parms, FS_memset_ret, FS_memset_saves },
    (byte_seq *)&C_memset,    { "memset", (byte_seq *)&FC_memset, FC_memset_parms, FC_memset_ret, FC_memset_saves },
    (byte_seq *)&S_memcpy,    { "memcpy", (byte_seq *)&FS_memcpy, FS_memcpy_parms, FS_memcpy_ret, FS_memcpy_saves },
    (byte_seq *)&C_memcpy,    { "memcpy", (byte_seq *)&FC_memcpy, FC_memcpy_parms, FC_memcpy_ret, FC_memcpy_saves },
    (byte_seq *)&C_memcmp,    { "memcmp", (byte_seq *)&FC_memcmp, FC_memcmp_parms, FC_memcmp_ret, FC_memcmp_saves },
    (byte_seq *)&C_memchr,    { "memchr", (byte_seq *)&FC_memchr, FC_memchr_parms, FC_memchr_ret, FC_memchr_saves },
    (byte_seq *)&C_strlen,    { "strlen", (byte_seq *)&FC_strlen, FC_strlen_parms, FC_strlen_ret, FC_strlen_saves },
    (byte_seq *)&C_strcat,    { "strcat", (byte_seq *)&FC_strcat, FC_strcat_parms, FC_strcat_ret, FC_strcat_saves },
    NULL,                     { NULL }
};

/* these must also work in FLAT model */
const inline_funcs SInline_Functions[] = {
    { "strcpy", (byte_seq *)&S_strcpy, S_strcpy_parms, S_strcpy_ret, S_strcpy_saves },
    { "strcmp", (byte_seq *)&S_strcmp, S_strcmp_parms, S_strcmp_ret, S_strcmp_saves },
    { "strcat", (byte_seq *)&S_strcat, S_strcat_parms, S_strcat_ret, S_strcat_saves },
    { "strchr", (byte_seq *)&S_strchr, S_strchr_parms, S_strchr_ret, S_strchr_saves },
    { "memset", (byte_seq *)&S_memset, S_memset_parms, S_memset_ret, S_memset_saves },
    { "memcpy", (byte_seq *)&S_memcpy, S_memcpy_parms, S_memcpy_ret, S_memcpy_saves },
    { NULL }
};

/* these must also work in FLAT model */
const inline_funcs Inline_Functions[] = {
    { "strlen", (byte_seq *)&C_strlen, C_strlen_parms, C_strlen_ret, C_strlen_saves },
    { "strcpy", (byte_seq *)&C_strcpy, C_strcpy_parms, C_strcpy_ret, C_strcpy_saves },
    { "strcmp", (byte_seq *)&C_strcmp, C_strcmp_parms, C_strcmp_ret, C_strcmp_saves },
    { "strcat", (byte_seq *)&C_strcat, C_strcat_parms, C_strcat_ret, C_strcat_saves },
    { "strchr", (byte_seq *)&C_strchr, C_strchr_parms, C_strchr_ret, C_strchr_saves },
    { "memcpy", (byte_seq *)&C_memcpy, C_memcpy_parms, C_memcpy_ret, C_memcpy_saves },
    { "memcmp", (byte_seq *)&C_memcmp, C_memcmp_parms, C_memcmp_ret, C_memcmp_saves },
    { "memset", (byte_seq *)&C_memset, C_memset_parms, C_memset_ret, C_memset_saves },
    { "memchr", (byte_seq *)&C_memchr, C_memchr_parms, C_memchr_ret, C_memchr_saves },
    { ".stosd", (byte_seq *)&C_stosd,  C_stosd_parms,  C_stosd_ret,  C_stosd_saves  },
    { ".stosw", (byte_seq *)&C_stosw,  C_stosw_parms,  C_stosw_ret,  C_stosw_saves  },
    { ".stoswb",(byte_seq *)&C_stoswb, C_stoswb_parms, C_stoswb_ret, C_stoswb_saves },
    { NULL }
};

const inline_funcs SBigData_Functions[] = {
    { "strcpy", (byte_seq *)&Z_strcpy, Z_strcpy_parms, Z_strcpy_ret, Z_strcpy_saves },
    { "strcmp", (byte_seq *)&Z_strcmp, Z_strcmp_parms, Z_strcmp_ret, Z_strcmp_saves },
    { "strcat", (byte_seq *)&Z_strcat, Z_strcat_parms, Z_strcat_ret, Z_strcat_saves },
    { "memset", (byte_seq *)&Z_memset, Z_memset_parms, Z_memset_ret, Z_memset_saves },
    { "memcpy", (byte_seq *)&Z_memcpy, Z_memcpy_parms, Z_memcpy_ret, Z_memcpy_saves },
    { NULL }
};

const inline_funcs SBigDataNoDs_Functions[] = {
    { "strcpy", (byte_seq *)&DP_strcpy, DP_strcpy_parms, DP_strcpy_ret, DP_strcpy_saves },
    { "strcmp", (byte_seq *)&DP_strcmp, DP_strcmp_parms, DP_strcmp_ret, DP_strcmp_saves },
    { "strcat", (byte_seq *)&DP_strcat, DP_strcat_parms, DP_strcat_ret, DP_strcat_saves },
    { "memset", (byte_seq *)&DP_memset, DP_memset_parms, DP_memset_ret, DP_memset_saves },
    { "memcpy", (byte_seq *)&DP_memcpy, DP_memcpy_parms, DP_memcpy_ret, DP_memcpy_saves },
    { NULL }
};

const inline_funcs Flat_Functions[] = {
    { "strcmp", (byte_seq *)&F_strcmp, F_strcmp_parms, F_strcmp_ret, F_strcmp_saves },
    { ".stosd", (byte_seq *)&F_stosd,  F_stosd_parms,  F_stosd_ret,  F_stosd_saves  },
    { ".stosw", (byte_seq *)&F_stosw,  F_stosw_parms,  F_stosw_ret,  F_stosw_saves  },
    { ".stoswb",(byte_seq *)&F_stoswb, F_stoswb_parms, F_stoswb_ret, F_stoswb_saves },
    { NULL }
};

const inline_funcs BigData_Functions[] = {
    { "strlen", (byte_seq *)&BD_strlen, BD_strlen_parms, BD_strlen_ret, BD_strlen_saves },
    { "strcpy", (byte_seq *)&BD_strcpy, BD_strcpy_parms, BD_strcpy_ret, BD_strcpy_saves },
    { "strcmp", (byte_seq *)&BD_strcmp, BD_strcmp_parms, BD_strcmp_ret, BD_strcmp_saves },
    { "strcat", (byte_seq *)&BD_strcat, BD_strcat_parms, BD_strcat_ret, BD_strcat_saves },
    { "strchr", (byte_seq *)&BD_strchr, BD_strchr_parms, BD_strchr_ret, BD_strchr_saves },
    { "memcpy", (byte_seq *)&BD_memcpy, BD_memcpy_parms, BD_memcpy_ret, BD_memcpy_saves },
    { "memcmp", (byte_seq *)&BD_memcmp, BD_memcmp_parms, BD_memcmp_ret, BD_memcmp_saves },
    { "memset", (byte_seq *)&BD_memset, BD_memset_parms, BD_memset_ret, BD_memset_saves },
    { "memchr", (byte_seq *)&BD_memchr, BD_memchr_parms, BD_memchr_ret, BD_memchr_saves },
    { ".stosd", (byte_seq *)&BD_stosd,  BD_stosd_parms,  BD_stosd_ret,  BD_stosd_saves  },
    { ".stosw", (byte_seq *)&BD_stosw,  BD_stosw_parms,  BD_stosw_ret,  BD_stosw_saves  },
    { ".stoswb",(byte_seq *)&BD_stoswb, BD_stoswb_parms, BD_stoswb_ret, BD_stoswb_saves },
    { NULL }
 };

const struct  inline_funcs BigDataNoDs_Functions[] = {
    { "strlen", (byte_seq *)&DP_strlen, DP_strlen_parms, DP_strlen_ret, DP_strlen_saves },
    { "strcpy", (byte_seq *)&DP_strcpy, DP_strcpy_parms, DP_strcpy_ret, DP_strcpy_saves },
    { "strcmp", (byte_seq *)&DP_strcmp, DP_strcmp_parms, DP_strcmp_ret, DP_strcmp_saves },
    { "strcat", (byte_seq *)&DP_strcat, DP_strcat_parms, DP_strcat_ret, DP_strcat_saves },
    { "strchr", (byte_seq *)&DP_strchr, DP_strchr_parms, DP_strchr_ret, DP_strchr_saves },
    { "memcpy", (byte_seq *)&DP_memcpy, DP_memcpy_parms, DP_memcpy_ret, DP_memcpy_saves },
    { "memcmp", (byte_seq *)&DP_memcmp, DP_memcmp_parms, DP_memcmp_ret, DP_memcmp_saves },
    { "memset", (byte_seq *)&DP_memset, DP_memset_parms, DP_memset_ret, DP_memset_saves },
    { "memchr", (byte_seq *)&DP_memchr, DP_memchr_parms, DP_memchr_ret, DP_memchr_saves },
    { ".stosd", (byte_seq *)&BD_stosd,  BD_stosd_parms,  BD_stosd_ret,  BD_stosd_saves  },
    { ".stosw", (byte_seq *)&BD_stosw,  BD_stosw_parms,  BD_stosw_ret,  BD_stosw_saves  },
    { ".stoswb",(byte_seq *)&BD_stoswb, BD_stoswb_parms, BD_stoswb_ret, BD_stoswb_saves },
    { NULL }
 };

/* these must also work in FLAT model */
const struct  inline_funcs _8087_Functions[] = {
    { "fabs",  (byte_seq *)&_8087_fabs, C_8087_fabs_parms, C_8087_fabs_ret, C_8087_fabs_saves  },
    { NULL }
};


/* memory model independent functions */

/* these must also work in FLAT model except _fmem & _fstr functions */
const struct  inline_funcs Common_Functions[] = {
  { "abs",     (byte_seq *)&C_abs,     C_abs_parms,     C_abs_ret,     C_abs_saves      },
  { "labs",    (byte_seq *)&C_labs,    C_labs_parms,    C_labs_ret,    C_labs_saves     },
  { "fabs",    (byte_seq *)&C_fabs,    C_fabs_parms,    C_fabs_ret,    C_fabs_saves     },
  { "inp",     (byte_seq *)&C_inp,     C_inp_parms,     C_inp_ret,     C_inp_saves      },
  { "inpw",    (byte_seq *)&C_inpw,    C_inpw_parms,    C_inpw_ret,    C_inpw_saves     },
  { "inpd",    (byte_seq *)&C_inpd,    C_inpd_parms,    C_inpd_ret,    C_inpd_saves     },
  { "outp",    (byte_seq *)&C_outp,    C_outp_parms,    C_outp_ret,    C_outp_saves     },
  { "outpw",   (byte_seq *)&C_outpw,   C_outpw_parms,   C_outpw_ret,   C_outpw_saves    },
  { "outpd",   (byte_seq *)&C_outpd,   C_outpd_parms,   C_outpd_ret,   C_outpd_saves    },
  { "movedata",(byte_seq *)&C_movedata,C_movedata_parms,C_movedata_ret,C_movedata_saves },
  { "_enable", (byte_seq *)&C_enable,  C_enable_parms,  C_enable_ret,  C_enable_saves   },
  { "_disable",(byte_seq *)&C_disable, C_disable_parms, C_disable_ret, C_disable_saves  },
  { "_rotl",   (byte_seq *)&C_rotl,    C_rotl_parms,    C_rotl_ret,    C_rotl_saves     },
  { "_rotr",   (byte_seq *)&C_rotr,    C_rotr_parms,    C_rotr_ret,    C_rotr_saves     },
  { "_lrotl",  (byte_seq *)&C_rotl,    C_rotl_parms,    C_rotl_ret,    C_rotl_saves     },
  { "_lrotr",  (byte_seq *)&C_rotr,    C_rotr_parms,    C_rotr_ret,    C_rotr_saves     },
  { "div",     (byte_seq *)&C_div,     C_div_parms,     C_div_ret,     C_div_saves      },
  { "ldiv",    (byte_seq *)&C_div,     C_div_parms,     C_div_ret,     C_div_saves      },
  { "_fmemcpy",(byte_seq *)&DP_memcpy, DP_memcpy_parms, DP_memcpy_ret, DP_memcpy_saves  },
  { "_fmemcmp",(byte_seq *)&DP_memcmp, DP_memcmp_parms, DP_memcmp_ret, DP_memcmp_saves  },
  { "_fmemset",(byte_seq *)&DP_memset, DP_memset_parms, DP_memset_ret, DP_memset_saves  },
  { "_fmemchr",(byte_seq *)&DP_memchr, DP_memchr_parms, DP_memchr_ret, DP_memchr_saves  },
  { "_fstrcpy",(byte_seq *)&DP_strcpy, DP_strcpy_parms, DP_strcpy_ret, DP_strcpy_saves  },
  { "_fstrcat",(byte_seq *)&DP_strcat, DP_strcat_parms, DP_strcat_ret, DP_strcat_saves  },
  { "_fstrcmp",(byte_seq *)&DP_strcmp, DP_strcmp_parms, DP_strcmp_ret, DP_strcmp_saves  },
  { "_fstrlen",(byte_seq *)&DP_strlen, DP_strlen_parms, DP_strlen_ret, DP_strlen_saves  },
  { ".min",    (byte_seq *)&C_min,     C_min_parms,     C_min_ret,     C_min_saves      },
  { ".max",    (byte_seq *)&C_max,     C_max_parms,     C_max_ret,     C_max_saves      },
  { NULL }
};
