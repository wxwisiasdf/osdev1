add-symbol-file kernel/kernel.elf
add-symbol-file apps/calc.elf
target remote localhost:1234
layout asm
br Kernel_Main()
br Int0Ah_AsmStub
br Int0Dh_AsmStub
br *0x1000000
c