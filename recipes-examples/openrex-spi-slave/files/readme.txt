Ensure that 'spidev' is loaded by executing:

lsmod | grep spidev

If you see blank line, please insert spidev module by executing:

modprobe spidev

The module should be located at '/lib/modules/3.14.61-yocto+gc7f3526/kernel/drivers/spi/spidev.ko'
Your path might be different in the Kernel version part. Module creates '/dev/spidevX.X' devices.
In our case it's '/dev/spidev2.0' that allows userspace application to communicate with spi peripheral.
