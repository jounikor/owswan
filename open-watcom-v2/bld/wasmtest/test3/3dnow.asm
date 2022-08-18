.586
.k3d
.model small

.data

L$1 dq 0
L$2 dq 0

.code

    femms       
    pavgusb     mm1,mm7
    pavgusb     mm1,DGROUP:L$1
    pavgusb     mm1,cs:0aH[bx+di]
    pavgusb     mm1,cs:0aH[ebx+eax*2]
    pf2id       mm1,mm7
    pf2id       mm1,DGROUP:L$1
    pf2id       mm1,cs:0aH[bx+di]
    pf2id       mm1,cs:0aH[ebx+eax*2]
    pf2iw       mm1,mm7
    pf2iw       mm1,DGROUP:L$1
    pf2iw       mm1,cs:0aH[bx+di]
    pf2iw       mm1,cs:0aH[ebx+eax*2]
    pfacc       mm1,mm7
    pfacc       mm1,DGROUP:L$1
    pfacc       mm1,cs:0aH[bx+di]
    pfacc       mm1,cs:0aH[ebx+eax*2]
    pfadd       mm1,mm7
    pfadd       mm1,DGROUP:L$1
    pfadd       mm1,cs:0aH[bx+di]
    pfadd       mm1,cs:0aH[ebx+eax*2]
    pfcmpeq     mm1,mm7
    pfcmpeq     mm1,DGROUP:L$1
    pfcmpeq     mm1,cs:0aH[bx+di]
    pfcmpeq     mm1,cs:0aH[ebx+eax*2]
    pfcmpge     mm1,mm7
    pfcmpge     mm1,DGROUP:L$1
    pfcmpge     mm1,cs:0aH[bx+di]
    pfcmpge     mm1,cs:0aH[ebx+eax*2]
    pfcmpgt     mm1,mm7
    pfcmpgt     mm1,DGROUP:L$1
    pfcmpgt     mm1,cs:0aH[bx+di]
    pfcmpgt     mm1,cs:0aH[ebx+eax*2]
    pfmax       mm1,mm7
    pfmax       mm1,DGROUP:L$1
    pfmax       mm1,cs:0aH[bx+di]
    pfmax       mm1,cs:0aH[ebx+eax*2]
    pfmin       mm1,mm7
    pfmin       mm1,DGROUP:L$1
    pfmin       mm1,cs:0aH[bx+di]
    pfmin       mm1,cs:0aH[ebx+eax*2]
    pfmul       mm1,mm7
    pfmul       mm1,DGROUP:L$1
    pfmul       mm1,cs:0aH[bx+di]
    pfmul       mm1,cs:0aH[ebx+eax*2]
    pfnacc      mm1,mm7
    pfnacc      mm1,DGROUP:L$1
    pfnacc      mm1,cs:0aH[bx+di]
    pfnacc      mm1,cs:0aH[ebx+eax*2]
    pfpnacc     mm1,mm7
    pfpnacc     mm1,DGROUP:L$1
    pfpnacc     mm1,cs:0aH[bx+di]
    pfpnacc     mm1,cs:0aH[ebx+eax*2]
    pfrcp       mm1,mm7
    pfrcp       mm1,DGROUP:L$1
    pfrcp       mm1,cs:0aH[bx+di]
    pfrcp       mm1,cs:0aH[ebx+eax*2]
    pfrcpit1    mm1,mm7
    pfrcpit1    mm1,DGROUP:L$1
    pfrcpit1    mm1,cs:0aH[bx+di]
    pfrcpit1    mm1,cs:0aH[ebx+eax*2]
    pfrcpit2    mm1,mm7
    pfrcpit2    mm1,DGROUP:L$1
    pfrcpit2    mm1,cs:0aH[bx+di]
    pfrcpit2    mm1,cs:0aH[ebx+eax*2]
    pfrsqit1    mm1,mm7
    pfrsqit1    mm1,DGROUP:L$1
    pfrsqit1    mm1,cs:0aH[bx+di]
    pfrsqit1    mm1,cs:0aH[ebx+eax*2]
    pfrsqrt     mm1,mm7
    pfrsqrt     mm1,DGROUP:L$1
    pfrsqrt     mm1,cs:0aH[bx+di]
    pfrsqrt     mm1,cs:0aH[ebx+eax*2]
    pfsub       mm1,mm7
    pfsub       mm1,DGROUP:L$1
    pfsub       mm1,cs:0aH[bx+di]
    pfsub       mm1,cs:0aH[ebx+eax*2]
    pfsubr      mm1,mm7
    pfsubr      mm1,DGROUP:L$1
    pfsubr      mm1,cs:0aH[bx+di]
    pfsubr      mm1,cs:0aH[ebx+eax*2]
    pi2fd       mm1,mm7
    pi2fd       mm1,DGROUP:L$1
    pi2fd       mm1,cs:0aH[bx+di]
    pi2fd       mm1,cs:0aH[ebx+eax*2]
    pi2fw       mm1,mm7
    pi2fw       mm1,DGROUP:L$1
    pi2fw       mm1,cs:0aH[bx+di]
    pi2fw       mm1,cs:0aH[ebx+eax*2]
    pmulhrw     mm1,mm7
    pmulhrw     mm1,DGROUP:L$1
    pmulhrw     mm1,cs:0aH[bx+di]
    pmulhrw     mm1,cs:0aH[ebx+eax*2]
    prefetch    DGROUP:L$1
    prefetch    cs:0aH[bx+di]
    prefetch    cs:0aH[ebx+eax*2]
    prefetchw   DGROUP:L$2
    pswapd      mm1,mm7
    pswapd      mm1,DGROUP:L$1
    pswapd      mm1,cs:0aH[bx+di]
    pswapd      mm1,cs:0aH[ebx+eax*2]

end
