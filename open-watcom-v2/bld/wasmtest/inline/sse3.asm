 addsubpd xmm1,xmm7
 addsubps xmm1,xmm7
 fisttp dword ptr x
 fisttp dword ptr cs:0aH[bx+di]
 fisttp dword ptr cs:0aH[ebx+eax*2]
 fisttp qword ptr x
 fisttp qword ptr cs:0aH[bx+di]
 fisttp qword ptr cs:0aH[ebx+eax*2]
 fisttp word ptr x
 fisttp word ptr cs:0aH[bx+di]
 fisttp word ptr cs:0aH[ebx+eax*2]
 haddpd xmm1,xmm7
 haddps xmm1,xmm7
 hsubpd xmm1,xmm7
 hsubps xmm1,xmm7
 lddqu xmm1,x
 lddqu xmm1,cs:0aH[bx+di]
 lddqu xmm1,cs:0aH[ebx+eax*2]
 monitor
 movddup xmm1,xmm7
 movddup xmm1,x
 movddup xmm1,cs:0aH[bx+di]
 movddup xmm1,cs:0aH[ebx+eax*2]
 movshdup xmm1,xmm7
 movshdup xmm1,x
 movshdup xmm1,cs:0aH[bx+di]
 movshdup xmm1,cs:0aH[ebx+eax*2]
 movsldup xmm1,xmm7
 movsldup xmm1,x
 movsldup xmm1,cs:0aH[bx+di]
 movsldup xmm1,cs:0aH[ebx+eax*2]
 mwait
