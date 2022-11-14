CXXFLAGS += \
	-I .. \
	-Wframe-larger-than=386 \
	-fno-threadsafe-statics \
	-std=c++23 \
	-fno-exceptions \
	-fno-rtti \
	-msoft-float \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-pie \
	-fno-pic \
	-fmodules-ts
#	-fexec-charset=IBM-1047
#	-fwide-exec-charset=IBM-1047
#	-fsanitize=undefined

LDFLAGS += \
	-nostdlib \
	-static-libgcc

PROGRAMS := hello.exe calc.exe mplay.exe run.exe tour.exe system.exe
DEPS := $(PROGRAMS:.exe=.d)

all: build

build: $(PROGRAMS)

clean:
	$(RM) *.elf *.o *.d *.exe

.PHONY: clean build all

ksyms.ld: ../kernel/kernel.elf ../tools/elf2ld
	echo "ENTRY(_Z11UDOS_32MainPDi);" >$@
	../tools/elf2ld $< >>$@
	echo "SECTIONS {" >>$@
	echo ". = 16M;" >>$@
	echo "	.text : ALIGN(4K) { *(.text.startup); *(.text .text.*); }" >>$@
	echo "	.rodata : ALIGN(4K) { *(.rodata .rodata.*); }" >>$@
	echo "	.data : ALIGN(4K) { *(.data .data.*); }" >>$@
	echo "	.bss : ALIGN(4K) { *(COMMON); *(.bss .bss.*); }" >>$@
	echo "}" >>$@

%.elf: %.cxx ksyms.ld
	$(CXX) $(CXXFLAGS) -MMD $< -o $@ -Tksyms.ld

%.elf: %.S ksyms.ld
	$(AS) $(ASFLAGS) -MMD $< -o $@ -Tksyms.ld

%.exe: %.elf
	$(OBJCOPY) -O binary $< $@

-include $(DEPS)