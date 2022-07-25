PREFIX = aarch64-linux-gnu

CC = $(PREFIX)-gcc
ASM = $(PREFIX)-as
OBJ_COPY = $(PREFIX)-objcopy
LD = $(PREFIX)-ld

PROJECT = scruffy_boot
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

CFLAGS = $(INCLUDES) -ffreestanding -g -O0 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-array-bounds -mgeneral-regs-only -fno-asynchronous-unwind-tables -fomit-frame-pointer -fno-unwind-tables #-fPIC
LDFLAGS = -ffreestanding -nostdlib -lgcc

all: | clean build

$(BUILD_DIR)/obj/%.o: ./%.S obj
		$(CC) $(INCLUDES) -g -c $< -o $@

$(BUILD_DIR)/obj/%.o: ./%.c obj
		$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/obj/%.o: mm/%.c obj
		$(CC) $(CFLAGS) -c $< -o $@

#$(BUILD_DIR)/$(ELF): $(ASSEMBLY_OBJS) $(C_OBJS) build
#		$(CC) -T $(LINKER_FILE) -o $@ $(ASSEMBLY_OBJS) $(C_OBJS) $(ARM_COMPILER_OPTIONS) $(LDFLAGS) 

obj:
		@mkdir -p $(BUILD_DIR)/obj
	
build: | obj $(ASSEMBLY_OBJS) $(C_OBJS)

clean:
		rm -rf $(BUILD_DIR)

#bin: | $(BUILD_DIR)/$(ELF)
#		$(OBJ_COPY) -O binary $(BUILD_DIR)/$(ELF) $(BUILD_DIR)/$(BIN)

