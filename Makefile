# ChocolateOS
#
# macOS: clang + ld.lld + qemu + xorriso
# Linux: the same, or x86_64-elf-gcc if you prefer.

CC      := clang
CXX     := clang++
LD      := ld.lld
TARGET   = --target=x86_64-unknown-none-elf

LIMINE_DIR := third_party/limine
LIMINE_BR  := v9.x-binary
ISO        := chocolateos.iso
KERNEL     := build/kernel.elf

INCLUDES := -Ikernel/include -I$(LIMINE_DIR)

FREESTANDING := $(TARGET) -ffreestanding -fno-stack-protector -fno-stack-check \
	-fno-lto -fno-pic -m64 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
	-mcmodel=kernel -mno-80387 -Wall -Wextra -DLIMINE_API_REVISION=3

CFLAGS   := $(FREESTANDING) -std=c11 $(INCLUDES)
CXXFLAGS := $(FREESTANDING) -std=c++20 -fno-exceptions -fno-rtti $(INCLUDES)
ASFLAGS  := $(TARGET) -c
LDFLAGS  := -nostdlib -static --no-pie -z max-page-size=0x1000 -T kernel/linker.ld

C_SRCS   := $(wildcard kernel/src/*.c)
CXX_SRCS := $(wildcard kernel/src/*.cpp)
AS_SRCS  := $(wildcard kernel/src/*.S)
OBJS     := $(C_SRCS:kernel/src/%.c=build/%.c.o) \
            $(CXX_SRCS:kernel/src/%.cpp=build/%.cpp.o) \
            $(AS_SRCS:kernel/src/%.S=build/%.S.o)

.PHONY: all kernel iso run run-nographic clean distclean

all: iso

$(LIMINE_DIR)/limine.h:
	mkdir -p third_party
	git clone --depth 1 --branch $(LIMINE_BR) https://github.com/limine-bootloader/limine.git $(LIMINE_DIR)
	$(MAKE) -C $(LIMINE_DIR)

build/%.c.o: kernel/src/%.c $(LIMINE_DIR)/limine.h
	mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.cpp.o: kernel/src/%.cpp $(LIMINE_DIR)/limine.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%.S.o: kernel/src/%.S
	mkdir -p build
	$(CC) $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJS) kernel/linker.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

kernel: $(KERNEL)

iso: $(KERNEL) $(LIMINE_DIR)/limine.h
	rm -rf iso_root
	mkdir -p iso_root/boot/limine iso_root/EFI/BOOT
	cp $(KERNEL) iso_root/boot/kernel.elf
	cp boot/limine.conf iso_root/boot/limine/
	cp $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin \
	   $(LIMINE_DIR)/limine-uefi-cd.bin iso_root/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO)
	$(LIMINE_DIR)/limine bios-install $(ISO)

QEMU_FLAGS := -m 256M -serial stdio -no-reboot -no-shutdown \
	-device usb-ehci,id=ehci \
	-device qemu-xhci,id=xhci \
	-device usb-kbd,bus=xhci.0 \
	-device usb-mouse,bus=xhci.0

run: iso
	qemu-system-x86_64 $(QEMU_FLAGS) -cdrom $(ISO)

run-nographic: iso
	qemu-system-x86_64 $(QEMU_FLAGS) -nographic -cdrom $(ISO)

clean:
	rm -rf build iso_root $(ISO)

distclean: clean
	rm -rf third_party
