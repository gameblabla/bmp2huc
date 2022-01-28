# BMP2HUC

Simple tool for converting BMP files to the PC-FX's KING 8bpp or 16bpp format.


# Usage

bmp2huc.elf in.bmp out.bin bitmap_width bitmap_height [bitdepth]

bitdepth :
- 2 (16bpp)
- 1 (8bpp)

For 8bpp, make sure your BMP file is actually 8bpp with a 256 colors palette.
A PAL and C Header file will also be saved in the same directory.

For 16bpp, a 24bpp BMP file is expected.
16bpp BMP files won't work and are probably not preferable due to the YUV nature of the PC-FX.
So for best image quality, 24bpp should be used instead.

# Technical details

So 8bpp on the PC-FX is quite unusual because it expects a big endian framebuffer, even though the V810 is little endian.
This means that the bytes are swapped and so it makes it extra annoying to work with.

16bpp on the PC-FX is quite straight forward on the other hand, and is little endian.

All color modes expect Y8U4V4 pixels instead of RGB ones, so this can be a little difficult to work with.
