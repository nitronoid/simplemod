# Makefile

This module is designed only to be build out-of-tree.

### Build
```
KERNEL_SOURCE=/path/to/kernel make
# or
KERNEL_SOURCE=/path/to/kernel make mod
```

### Clean
```
KERNEL_SOURCE=/path/to/kernel make clean
```

### Format
```
make format
```

# Usage

The expectation is that you have root access.
```
# install the module
insmod simple_mod.ko

# write to the procfs node
echo hello > /proc/simple
echo world > /proc/simple

# read from the procfs node
cat /proc/simple
# Expected output:
# hello
# world

# remove the module
rmmod simple_mod
```
