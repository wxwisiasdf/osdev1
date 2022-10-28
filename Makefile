export CXX := gcc
export CXXFLAGS := \
	-Werror \
	-Wfatal-errors \
	-O3 \
	-ffreestanding \
	-fstrict-aliasing \
	-fomit-frame-pointer \
	-nostdlib \
	-Wall \
	-Wextra \
	-m32 \
	-march=i686 \
	-flto \
	-ffast-math

export AS := gcc
export ASFLAGS := -m32

export LD := gcc
export LDFLAGS := \
	-Os \
	-m32 \
	-flto \
	-z max-page-size=0x1000

all: build
run: newos.iso build
	objdump -S -C -d isodir/boot/kernel.elf >dump.txt
	objdump -t isodir/boot/kernel.elf | sort >>dump.txt
	qemu-system-x86_64 -s -cdrom newos.iso -smp 2 \
		-d guest_errors,pcall,strace -trace ps2* -no-reboot \
		-vga cirrus -audiodev alsa,id=alsa -device sb16,audiodev=alsa \
		-serial stdio -device usb-ehci \
		-device usb-tablet \
		2>qemu.txt

build: newos.iso
	$(MAKE) -C kernel build
	$(MAKE) -C apps build

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C apps clean
	$(RM) *.iso
	$(RM) -r isodir

.PHONY: clean build all run

newos.iso: isodir/boot/kernel.elf
	mkdir -p grub
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp -r apps/*.png apps/*.o apps/*.raw isodir/
	$(RM) $@
#	grub-file --is-x86-multiboot2 $<
	grub-mkrescue -o $@ isodir

isodir/boot/kernel.elf: kernel/kernel.elf
	mkdir -p isodir/boot/grub
	strip --strip-unneeded $< -o $@

kernel/kernel.elf:
	$(MAKE) -C kernel build
	$(MAKE) -C apps build
