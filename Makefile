export CXX := g++
export CXXFLAGS := \
	-Wfatal-errors \
	-O2 \
	-g \
	-ffreestanding \
	-nostdlib \
	-Wall \
	-Wextra \
	-m32 \
	-march=i686

#	-ffast-math \
#	-fstrict-aliasing \
#	-fomit-frame-pointer

export AS := gcc
export ASFLAGS := -m32

export LD := gcc
export LDFLAGS := \
	-O3 \
	-m32 \
	-z max-page-size=0x1000

export STRIP := strip
export OBJCOPY := objcopy

all: build
run: newos.iso build
	objdump -S -C -d isodir/boot/kernel.elf >dump.txt
	objdump -t isodir/boot/kernel.elf | sort >>dump.txt
	qemu-system-x86_64 -s -cdrom newos.iso -d guest_errors,pcall,strace \
		-serial stdio 2>qemu.txt

build: newos.iso
	$(MAKE) -C tools build
	$(MAKE) -C kernel build
	$(MAKE) -C apps build

clean:
	$(MAKE) -C tools clean
	$(MAKE) -C kernel clean
	$(MAKE) -C apps clean
	$(RM) *.iso
	$(RM) -r isodir

.PHONY: clean build all run

newos.iso: isodir/boot/kernel.elf
	mkdir -p grub
	cp grub.cfg isodir/boot/grub/grub.cfg
	cp -r apps/* isodir/
	$(RM) $@
#	grub-file --is-x86-multiboot2 $<
	grub-mkrescue -o $@ isodir

isodir/boot/kernel.elf: kernel/kernel.elf
	mkdir -p isodir/boot/grub
	strip --strip-unneeded $< -o $@

kernel/kernel.elf:
	$(MAKE) -C tools build
	$(MAKE) -C kernel build
	$(MAKE) -C apps build
