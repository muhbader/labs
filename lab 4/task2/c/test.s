section .text
global _start

section .bss
    buffer: resb 8192
section .data
    infect: dd "Hello, Infected File",0
    infect_len: equ $ -infect
    error: dd "couldn't open",10,0
    error_len: equ $ -error
    open: dd 5
    write: dd 4
    append: dd 0x0441
    permission: dd 441
    file: db "file.txt",0
    stdout: dd 1
    fb: dd 0
    close: dd 6
    seek: db 19
    tes: db "test.o",0
    read: dd 3

_start:
    mov ebx, file    
    mov eax, [open]
    mov ecx, 441h
    mov edx, 441
    int 0x80
    mov dword[fb],eax

    cmp eax,-1
    jng err

    ; mov eax,[seek]
    ; mov ebx,[fb]
    ; mov ecx,0
    ; mov edx, 0
    ; int 0x80

    ; cmp eax,-1
    ; jng err


    ; mov eax,[read]
    ; mov ebx,[fb]
    ; mov ecx, buffer
    ; mov edx, 10
    ; int 0x80

    ; cmp eax,-1
    jng err

    
    ; ;;write to file
    mov ebx,[fb] ;;file descriptor
    mov eax,[write]
    mov ecx,infect
    mov edx,infect_len
    int 0x80

    mov eax,dword[close]
    mov ebx,[fb]
    int 0x80
end:
    mov     ebx,0
    mov     eax,1 ;;exit codess
    int     0x80
    nop

err:   
    mov eax, [write]
    mov ebx,[stdout]
    mov ecx,error
    mov edx,error_len
    int 0x80
    jmp end                                                                                                                                                                                                                                                                                                                                                  