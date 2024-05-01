;450 B
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

    ;mov rax,60
    ;mov rdi,0
    ;syscall
    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r11


    ; return
    ret

message: db "Get Hacked UwU mmrpp nya :3", 10, 0 
message_len equ $ - message