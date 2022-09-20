## Operating System Kernel

Small operating system kernel targeting ARM architecture. Currently only runs on QEMU emulator. 

### Required packages
```
sudo apt install gcc-aarch64-linux-gnu

sudo apt install qemu-system-arm

sudo apt install gdb-multiarch
```

### Build and Run

```
make

make emulate
```

### Debugging

```
make debug

gdb-multiarch -x debug.gdb
```
