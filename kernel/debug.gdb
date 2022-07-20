set disassemble-next-line on
set confirm off
set arch aarch64
add-symbol-file build/scruffy.elf
target remote localhost:1234
layout regs
