# BMP2HUC

Simple tool for converting 8bpp BMP files to the PC-FX's KING 8bpp format.

The strange thing about this color mode is that it is linear but the bytes are swapped
even though the V810 is little endian by nature.

# Usage

bmp2huc.elf in.bmp out.bin bitmap_width bitmap_height
