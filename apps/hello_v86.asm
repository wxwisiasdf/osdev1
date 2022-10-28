section .text
start:
    mov ax, seg hello
    mov ds, ax
    mov dx, hello
    mov ah, 09h
    int 21h
    mov ah, 4ch
    xor al, al
    int 21h

section .data
hello db 'Hello, world!', 13, 10, '$'
