PROJECT := oak-os
BUILD := build
ISO_DIR := $(BUILD)/iso
KERNEL := $(BUILD)/acorn.elf
ISO := $(BUILD)/oak-os.iso
DISK := $(BUILD)/oak-disk.img
USER_ELF := $(BUILD)/user/init.elf
USER_OBJ := $(BUILD)/user/init.o $(BUILD)/user/libc.o
USER_BINARY_OBJ := $(BUILD)/user/init-elf.o

CC := gcc
LD := ld
OBJCOPY := objcopy

CFLAGS := -std=c11 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -Wall -Wextra -Werror -O2 -g -Ikernel/include
ASFLAGS := -g
LDFLAGS := -nostdlib -z max-page-size=0x1000 -z noexecstack -T linker.ld

C_SOURCES := kernel/kernel.c kernel/serial.c kernel/vga.c kernel/framebuffer.c kernel/gui.c kernel/mouse.c kernel/interrupts.c kernel/memory.c kernel/pic.c kernel/timer.c kernel/keyboard.c kernel/scheduler.c kernel/process.c kernel/syscall.c kernel/vm.c kernel/elf.c kernel/fs.c kernel/ata.c
ASM_SOURCES := boot/boot.S kernel/idt.S kernel/context.S
OBJECTS := $(C_SOURCES:%.c=$(BUILD)/%.o) $(ASM_SOURCES:%.S=$(BUILD)/%.o) $(USER_BINARY_OBJ)

.PHONY: all kernel iso run run-gui run-vnc stop-vnc debug test clean

all: iso

kernel: $(KERNEL)

$(KERNEL): $(OBJECTS) $(USER_ELF) linker.ld
	@mkdir -p $(@D)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
	$(OBJCOPY) -O binary $@ $(BUILD)/acorn.bin

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/user/init.o: user/init.S
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/user/libc.o: user/libc.S
	@mkdir -p $(@D)
	$(CC) $(ASFLAGS) -c $< -o $@

$(USER_ELF): $(USER_OBJ) user.ld
	@mkdir -p $(@D)
	$(LD) -nostdlib -T user.ld -o $@ $(USER_OBJ)

$(USER_BINARY_OBJ): $(USER_ELF)
	@mkdir -p $(@D)
	$(LD) -r -b binary $< -o $@

iso: $(KERNEL) grub.cfg
	@command -v grub-mkrescue >/dev/null || { echo "erro: grub-mkrescue nao esta instalado" >&2; exit 1; }
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/acorn.elf
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)

$(DISK):
	@mkdir -p $(@D)
	dd if=/dev/zero of=$@ bs=1M count=16 status=none

run: iso $(DISK)
	@command -v qemu-system-x86_64 >/dev/null || { echo "erro: qemu-system-x86_64 nao esta instalado" >&2; exit 1; }
	qemu-system-x86_64 -cdrom $(ISO) -drive file=$(DISK),format=raw,if=ide -nographic -no-reboot -no-shutdown

run-gui: iso $(DISK)
	@command -v qemu-system-x86_64 >/dev/null || { echo "erro: qemu-system-x86_64 nao esta instalado" >&2; exit 1; }
	@test -n "$(DISPLAY)$(WAYLAND_DISPLAY)" || { echo "erro: este Codespace nao possui DISPLAY; use 'make run-vnc' e um cliente VNC" >&2; exit 1; }
	qemu-system-x86_64 -vga std -cdrom $(ISO) -drive file=$(DISK),format=raw,if=ide -serial stdio -no-reboot -no-shutdown

run-vnc: iso $(DISK)
	@command -v qemu-system-x86_64 >/dev/null || { echo "erro: qemu-system-x86_64 nao esta instalado" >&2; exit 1; }
	@command -v websockify >/dev/null || { echo "erro: websockify nao esta instalado" >&2; exit 1; }
	@echo "noVNC disponivel em http://127.0.0.1:6080/vnc.html?autoconnect=true&show_dot=true"
	@$(MAKE) -s stop-vnc
	@qemu-system-x86_64 -machine pc -vga std -cdrom $(ISO) -drive file=$(DISK),format=raw,if=ide -serial stdio -display vnc=127.0.0.1:1 -no-reboot -no-shutdown & qemu_pid=$$!; \
	websockify --web=/usr/share/novnc 6080 127.0.0.1:5901 & proxy_pid=$$!; \
	cleanup() { kill $$proxy_pid $$qemu_pid 2>/dev/null || true; pkill -P $$proxy_pid 2>/dev/null || true; }; \
	trap cleanup EXIT INT TERM; \
	wait $$proxy_pid

stop-vnc:
	@session_pids=$$(ps -eo pid=,comm=,args= | awk '($$2 == "qemu-system-x86" && $$0 ~ /oak-disk.img/) || ($$2 == "websockify" && $$0 ~ /6080/) || ($$2 == "python3" && $$0 ~ /websockify/ && $$0 ~ /6080/) {print $$1}'); \
	for pid in $$session_pids; do kill $$pid 2>/dev/null || true; done

debug: iso $(DISK)
	@command -v qemu-system-x86_64 >/dev/null || { echo "erro: qemu-system-x86_64 nao esta instalado" >&2; exit 1; }
	qemu-system-x86_64 -cdrom $(ISO) -drive file=$(DISK),format=raw,if=ide -serial stdio -S -s

test: kernel
	@test "$$(readelf -h $(KERNEL) | awk '/Class:/ { print $$2 }')" = ELF64
	@readelf -S $(KERNEL) | grep -q '.text'
	@echo "testes de imagem: OK (ELF64 e secao .text presentes)"

clean:
	rm -rf $(BUILD)