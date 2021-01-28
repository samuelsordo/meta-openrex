Application act is SPI master and draws primitives (send from SPI slave) to framebuffer. Run as

openrex_spi_canvas -f /dev/fb0 -s /dev/spidevice2.0 -t /dev/tty1 -b 400000
