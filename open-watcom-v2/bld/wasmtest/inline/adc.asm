 adc al,0fH
 adc al,7fH
 adc al,0ffH
 adc ax,0fH
 adc ax,7fH
 adc ax,0ffH
 adc ax,7fffH
 adc ax,0ffffH
 adc eax,0fH
 adc eax,7fH
 adc eax,0ffH
 adc eax,7fffH
 adc eax,0ffffH
 adc eax,7fffffffH
 adc eax,0ffffffffH
 adc dl,0fH
 adc dl,7fH
 adc dl,0ffH
 adc dx,0fH
 adc dx,7fH
 adc dx,0ffH
 adc dx,7fffH
 adc dx,0ffffH
 adc edx,0fH
 adc edx,7fH
 adc edx,0ffH
 adc edx,7fffH
 adc edx,0ffffH
 adc edx,7fffffffH
 adc edx,0ffffffffH
 adc byte ptr x,0fH
 adc byte ptr x,7fH
 adc byte ptr x,0ffH
 adc word ptr x,0fH
 adc word ptr x,7fH
 adc word ptr x,0ffH
 adc word ptr x,7fffH
 adc word ptr x,0ffffH
 adc dword ptr x,0fH
 adc dword ptr x,7fH
 adc dword ptr x,0ffH
 adc dword ptr x,7fffH
 adc dword ptr x,0ffffH
 adc dword ptr x,7fffffffH
 adc dword ptr x,0ffffffffH
 adc byte ptr x,dl
 adc word ptr x,dx
 adc dword ptr x,edx
 adc dl,byte ptr x
 adc dx,word ptr x
 adc edx,dword ptr x
 adc dl,bl
 adc dx,bx
 adc edx,ebx
 adc al,bl
 adc ax,bx
 adc eax,ebx
 adc dl,al
 adc dx,ax
 adc edx,eax
 adc edx,ecx
 adc edx,edx
 adc edx,esi
 adc edx,edi
 adc edx,esp
 adc edx,ebp
 adc dx,cx
 adc dx,dx
 adc dx,si
 adc dx,di
 adc dx,sp
 adc dx,bp
 adc dl,cl
 adc dl,dl
 adc dl,ah
 adc dl,bh
 adc dl,ch
 adc dl,dh
 adc eax,dword ptr [eax]
 adc eax,dword ptr [edx]
 adc eax,dword ptr [ebx]
 adc eax,dword ptr [ecx]
 adc eax,dword ptr [esi]
 adc eax,dword ptr [edi]
 adc eax,dword ptr [esp]
 adc eax,dword ptr [ebp]
 adc eax,dword ptr 0fH[edx]
 adc eax,dword ptr 7fH[edx]
 adc eax,dword ptr 7fffH[edx]
 adc eax,dword ptr 0ffffH[edx]
 adc eax,dword ptr 7fffffffH[edx]
 adc eax,dword ptr -1[edx]
 adc edx,dword ptr [eax]
 adc edx,dword ptr [edx]
 adc edx,dword ptr [ebx]
 adc edx,dword ptr [ecx]
 adc edx,dword ptr [esi]
 adc edx,dword ptr [edi]
 adc edx,dword ptr [esp]
 adc edx,dword ptr [ebp]
 adc edx,dword ptr 0fH[edx]
 adc edx,dword ptr 7fH[edx]
 adc edx,dword ptr 7fffH[edx]
 adc edx,dword ptr 0ffffH[edx]
 adc edx,dword ptr 7fffffffH[edx]
 adc edx,dword ptr -1[edx]
 adc edx,dword ptr [eax+esi]
 adc edx,dword ptr [edx+esi]
 adc edx,dword ptr [ebx+esi]
 adc edx,dword ptr [ecx+esi]
 adc edx,dword ptr [esi+esi]
 adc edx,dword ptr [edi+esi]
 adc edx,dword ptr [esp+esi]
 adc edx,dword ptr [ebp+esi]
 adc edx,dword ptr 0fH[edx+esi]
 adc edx,dword ptr 7fH[edx+esi]
 adc edx,dword ptr 7fffH[edx+esi]
 adc edx,dword ptr 0ffffH[edx+esi]
 adc edx,dword ptr 7fffffffH[edx+esi]
 adc edx,dword ptr -1[edx+esi]
 adc edx,dword ptr -0fH[edx+esi*2]
 adc edx,dword ptr -7fffH[edx+esi*4]
 adc edx,dword ptr [edx+esi*8]
