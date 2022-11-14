.set EFLAGS_NT, 1 << 14
.set EFLAGS_VM, 1 << 17

.section .text.dos
.global Kernel_EnterV86
Kernel_EnterV86:
    movl %esp, %ebp
    pushl 4(%ebp)
    pushl 8(%ebp)
    pushfl
    clts #and $(~EFLAGS_NT), (%esp)
    orl $EFLAGS_VM, (%esp)
    pushl 12(%ebp)
    pushl 16(%ebp)
    iretl

.global Kernel_Task16Trampoline
Kernel_Task16Trampoline:
    callw *%ax
    # Halt for now
    cli
    hlt

.global Kernel_Task32Trampoline
Kernel_Task32Trampoline:
    calll *%eax
    # Halt for now
    cli
    hlt

.global Kernel_TaskV86Stub
Kernel_TaskV86Stub:
    pushfl
    clts #and $(~EFLAGS_NT), (%esp)
    orl $EFLAGS_VM, (%esp)
    pushl %eax # CS is on EAX
    andl $0xFFFF, %ebx
    pushl %ebx # EIP is on EBX
    iretl

.align 8
.global Kernel_V86StackBottom
Kernel_V86StackBottom:
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
    .quad 0
.global Kernel_V86StackTop
Kernel_V86StackTop:
