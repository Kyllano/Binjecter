BITS 64

SECTION .text
global main

main:
    ; save context
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r11

    mov rax,1
    mov rdi,1
    lea rsi,[rel message]
    mov rdx, message_len
    syscall

    ; load context
    pop r11
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax

    mov rax, 0x4022e0
    jmp rax
message: db "Get Hacked UwU mmrpp nya :3", 10, 0 
message_len equ $ - message