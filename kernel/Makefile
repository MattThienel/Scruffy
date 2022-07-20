PREFIX = aarch64-linux-gnu

CC = $(PREFIX)-gcc
ASM = $(PREFIX)-as
OBJ_COPY = $(PREFIX)-objcopy
LD = $(PREFIX)-ld

PROJECT = scruffy
ELF = $(PROJECT).elf
BIN = $(PROJECT).bin

LINKER_FILE = linker.ld

BUILD_DIR = build

ASSEMBLY_FILES = $(wildcard ./*.S)
ASSEMBLY_OBJS = $(addprefix $(BUILD_DIR)/obj/,$(notdir $(ASSEMBLY_FILES:.S=.o)))

C_FILES = $(wildcard ./*.c)
C_FILES += $(wildcard mm/*.c)
C_OBJS = $(addprefix $(BUILD_DIR)/obj/,$(notdir $(C_FILES:.c=.o)))

INCLUDES = -I./

CFLAGS = $(INCLUDES) -ffreestanding -g -O2 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -mgeneral-regs-only 
LDFLAGS = -ffreestanding -nostdlib -lgcc

all: clean prekernel bin

$(BUILD_DIR)/obj/%.o: ./%.S obj
		$(CC) $(INCLUDES) -g -c $< -o $@

$(BUILD_DIR)/obj/%.o: ./%.c obj
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/%.o: mm/%.c obj
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(ELF): $(ASSEMBLY_OBJS) $(C_OBJS) build
		$(CC) -T $(LINKER_FILE) -o $@ build/obj/*.o $(ARM_COMPILER_OPTIONS) $(LDFLAGS) 

obj:
		@mkdir -p $(BUILD_DIR)/obj
	
build:
		@mkdir -p $(BUILD_DIR)

clean:
		rm -rf $(BUILD_DIR)

prekernel: obj
		$(MAKE) -C ./prekernel/
		cp ./prekernel/build/obj/*.o build/obj/

bin: | prekernel $(BUILD_DIR)/$(ELF)
		$(OBJ_COPY) -O binary $(BUILD_DIR)/$(ELF) $(BUILD_DIR)/$(BIN)

dump:
		$(PREFIX)-objdump -D $(BUILD_DIR)/$(ELF)

dump-source:
		$(PREFIX)-objdump -S $(BUILD_DIR)/$(ELF)

emulate:
		qemu-system-aarch64 -machine virt -cpu cortex-a53 -m 4G -kernel $(BUILD_DIR)/$(ELF) -nographic
#		qemu-system-aarch64 -machine virt -cpu cortex-a53 -kernel $(BUILD_DIR)/$(ELF) -nographic -machine dumpdtb=qemu.dtb 

emulate-int:
		qemu-system-aarch64 -machine virt,virtualization=on -cpu cortex-a53 -m 4G -kernel $(BUILD_DIR)/$(ELF) -nographic -d int

# Debugging uses gdb-multiarch
# Run: gdb-multiarch $(BUILD_DIR)/$(ELF)
# Then target remote localhost:1234
debug:
		qemu-system-aarch64 -machine virt -cpu cortex-a53 -m 4G -kernel $(BUILD_DIR)/$(ELF) -nographic -s -S -d int

