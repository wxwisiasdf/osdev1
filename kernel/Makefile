CXXFLAGS += \
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
	-Tlinker.ld \
	-nostdlib \
	-static-libgcc

# Low-level bootstrap devices
KERNEL_CXX_SRCS := \
	locale.cxx \
	pic.cxx \
	pit.cxx \
	uart.cxx \
	ps2.cxx \
	pci.cxx \
	dma.cxx \
	usb.cxx \
	ubsan.cxx

# Middle-end devices
KERNEL_CXX_SRCS += \
	rtl81xx.cxx \
	sb16.cxx

KERNEL_CXX_SRCS += \
	main.cxx \
	gdt.cxx \
	vga.cxx \
	tty.cxx \
	video.cxx \
	ui.cxx \
	task.cxx \
	load.cxx \
	alloc.cxx \
	floppy.cxx \
	adlib.cxx \
	atapi.cxx \
	iso9660.cxx \
	string.cxx \
	audio.cxx \
	vfs.cxx

KERNEL_ASM_SRCS := \
	crt0.S \
	handlers.S \
	dosv86.S

KERNEL_OBJS := $(KERNEL_CXX_SRCS:.cxx=.o) $(KERNEL_C_SRCS:.c=.o) $(KERNEL_ASM_SRCS:.S=.o)
DEPS := $(KERNEL_CXX_SRCS:.cxx=.d)

all: build

build: kernel.elf

clean:
	$(RM) *.elf
	$(RM) *.o *.d
	$(RM) -r gcm.cache

.PHONY: clean build all

kernel.elf: $(KERNEL_OBJS)
	$(LD) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -MMD -c $< -o $@

-include $(DEPS)
