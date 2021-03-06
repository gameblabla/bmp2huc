#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <SDL2/SDL.h>

typedef struct tagBITMAP              /* the structure for a bitmap. */
{
  uint16_t width;
  uint16_t height;
  uint8_t *data;
  uint8_t *pdata[4];
  uint16_t sprite_width;
  uint16_t sprite_height;
  uint8_t bytespp;
} BITMAP;


uint32_t key_pressed = 0;
uint8_t str[64];
uint8_t tmp_str[64];

int spr = 0;

uint8_t VGA_8158_GAMEPAL[768];
uint16_t num_colors = 256;

typedef enum {
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE
} str2int_errno;

void fskip(FILE *fp, int num_bytes)
{
   int i;
   for (i=0; i<num_bytes; i++)
      fgetc(fp);
}


str2int_errno str2int(int *out, char *s, int base) {
    char *end;
    if (s[0] == '\0' || isspace(s[0]))
        return STR2INT_INCONVERTIBLE;
    errno = 0;
    long l = strtol(s, &end, base);
    /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return STR2INT_OVERFLOW;
    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return STR2INT_UNDERFLOW;
    if (*end != '\0')
        return STR2INT_INCONVERTIBLE;
    *out = l;
    return STR2INT_SUCCESS;
}

static char *removestr(char* myStr)
{
    char *retStr;
    char *lastExt;
    if (myStr == NULL) return NULL;
    if ((retStr = malloc (strlen (myStr) + 1)) == NULL) return NULL;
    strcpy (retStr, myStr);
    lastExt = strrchr (retStr, '.');
    if (lastExt != NULL)
        *lastExt = '\0';
    return retStr;
}

static void Load_bmp(const char *file, BITMAP *b, unsigned short s_width, unsigned short s_height)
{
	FILE *fp;
	long long index;
	long x;
  
	/* open the file */
	if ((fp = fopen(file,"rb")) == NULL)
	{
		printf("Error opening file %s.\n",file);
		exit(1);
	}
  
	/* check to see if it is a valid bitmap file */
	if (fgetc(fp)!='B' || fgetc(fp)!='M')
	{
		fclose(fp);
		printf("%s is not a bitmap file.\n",file);
		exit(1);
	}
	
	/* read in the width and height of the image, and the
	number of colors used; ignore the rest */
	fskip(fp,16);
	fread(&b->width, sizeof(uint16_t), 1, fp);
	fskip(fp,2);
	fread(&b->height,sizeof(uint16_t), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(uint16_t), 1, fp);
	fskip(fp,6);

	/* assume we are working with an 8-bit file */
	if (num_colors==0) num_colors=256;

	/* try to allocate memory */
	if ((b->data = malloc((b->width*b->height))) == NULL)
	{
		fclose(fp);
		printf("Error allocating memory for file %s.\n",file);
		exit(1);
	}
  
	fskip(fp,num_colors*4);
	
	/* read the bitmap */
	for(index = (b->height-1)*b->width; index >= 0;index-=b->width)
	{
		for(x = 0; x < b->width; x++)
		{
			b->data[(int)((index+x))]=fgetc(fp);
		}
	}
	fclose(fp);
  
	b->sprite_width = s_width;
	b->sprite_height = s_height;
}

static void Load_palette(const char *file)
{
	FILE *fp;
	long index;
	num_colors = 256;
  
	/* open the file */
	if ((fp = fopen(file,"rb")) == NULL)
	{
		printf("Error opening file %s.\n",file);
		exit(1);
	}
	/* check to see if it is a valid bitmap file */
	if (fgetc(fp)!='B' || fgetc(fp)!='M')
	{
		fclose(fp);
		printf("%s is not a bitmap file.\n",file);
		exit(1);
	}

	//fskip(fp,52);
	fseek(fp, 52 + 2, SEEK_SET);

	/* assume we are working with an 8-bit file */
	if (num_colors==0) num_colors=256;
  
  /* read the palette information */
	for(index=0;index<num_colors;index++)
	{
		VGA_8158_GAMEPAL[(int)(index*3+2)] = fgetc(fp);
		VGA_8158_GAMEPAL[(int)(index*3+1)] = fgetc(fp);
		VGA_8158_GAMEPAL[(int)(index*3+0)] = fgetc(fp);
		fgetc(fp);
	}
	
	fclose(fp);
}

uint16_t RGB2PAL_nofile(int r, int g, int b)
{
	uint16_t ui___P;
	float fl___R = (float) r; // 8-bit (0-255)
	float fl___G = (float) g; // 8-bit (0-255)
	float fl___B = (float) b; // 8-bit (0-255)

	float fl___Y = ( 0.2990f * fl___R) + (0.5870f * fl___G) + (0.1140f * fl___B);
	float fl___U = (-0.1686f * fl___R) - (0.3311f * fl___G) + (0.4997f * fl___B) + 128.0f;
	float fl___V = ( 0.4998f * fl___R) - (0.4185f * fl___G) - (0.0813f * fl___B) + 128.0f;

	if (fl___Y <   0.0f) fl___Y =   0.0f;
	if (fl___Y > 255.0f) fl___Y = 255.0f;
	if (fl___U <   0.0f) fl___U =   0.0f;
	if (fl___U > 255.0f) fl___U = 255.0f;
	if (fl___V <   0.0f) fl___V =   0.0f;
	if (fl___V > 255.0f) fl___V = 255.0f;
	
	//fprintf("R %d, G %d, B%d\n", r, g, b);

	unsigned ui___Y = (unsigned) fl___Y;
	unsigned ui___U = (unsigned) fl___U;
	unsigned ui___V = (unsigned) fl___V;

	ui___U = (ui___U + 8 ) >> 4;
	ui___V = (ui___V + 8 ) >> 4;
	ui___P = (ui___Y << 8 ) + (ui___U << 4) + ui___V;
        
	return (uint16_t) ui___P;
}

uint16_t RGB2PAL(int r, int g, int b, FILE* f)
{
	uint16_t ui___P;
	float fl___R = (float) r; // 8-bit (0-255)
	float fl___G = (float) g; // 8-bit (0-255)
	float fl___B = (float) b; // 8-bit (0-255)

	float fl___Y = ( 0.2990f * fl___R) + (0.5870f * fl___G) + (0.1140f * fl___B);
	float fl___U = (-0.1686f * fl___R) - (0.3311f * fl___G) + (0.4997f * fl___B) + 128.0f;
	float fl___V = ( 0.4998f * fl___R) - (0.4185f * fl___G) - (0.0813f * fl___B) + 128.0f;

	if (fl___Y <   0.0f) fl___Y =   0.0f;
	if (fl___Y > 255.0f) fl___Y = 255.0f;
	if (fl___U <   0.0f) fl___U =   0.0f;
	if (fl___U > 255.0f) fl___U = 255.0f;
	if (fl___V <   0.0f) fl___V =   0.0f;
	if (fl___V > 255.0f) fl___V = 255.0f;
	
	//fprintf("R %d, G %d, B%d\n", r, g, b);

	unsigned ui___Y = (unsigned) fl___Y;
	unsigned ui___U = (unsigned) fl___U;
	unsigned ui___V = (unsigned) fl___V;

	ui___U = (ui___U + 8 ) >> 4;
	ui___V = (ui___V + 8 ) >> 4;
	ui___P = (ui___Y << 8 ) + (ui___U << 4) + ui___V;	

	fwrite (&ui___P, sizeof(char), 2, f);
        
	return (uint16_t) ui___P;
}

uint16_t RGB2PAL_text(int r, int g, int b, FILE* f)
{
	uint16_t ui___P;
	float fl___R = (float) r; // 8-bit (0-255)
	float fl___G = (float) g; // 8-bit (0-255)
	float fl___B = (float) b; // 8-bit (0-255)

	float fl___Y = ( 0.2990f * fl___R) + (0.5870f * fl___G) + (0.1140f * fl___B);
	float fl___U = (-0.1686f * fl___R) - (0.3311f * fl___G) + (0.4997f * fl___B) + 128.0f;
	float fl___V = ( 0.4998f * fl___R) - (0.4185f * fl___G) - (0.0813f * fl___B) + 128.0f;

	if (fl___Y <   0.0f) fl___Y =   0.0f;
	if (fl___Y > 255.0f) fl___Y = 255.0f;
	if (fl___U <   0.0f) fl___U =   0.0f;
	if (fl___U > 255.0f) fl___U = 255.0f;
	if (fl___V <   0.0f) fl___V =   0.0f;
	if (fl___V > 255.0f) fl___V = 255.0f;
	
	//fprintf("R %d, G %d, B%d\n", r, g, b);

	unsigned ui___Y = (unsigned) fl___Y;
	unsigned ui___U = (unsigned) fl___U;
	unsigned ui___V = (unsigned) fl___V;

	ui___U = (ui___U + 8 ) >> 4;
	ui___V = (ui___V + 8 ) >> 4;
	ui___P = (ui___Y << 8 ) + (ui___U << 4) + ui___V;	
	
	fprintf(f, "0x%x,", ui___P);
        
	return (uint16_t) ui___P;
} 


unsigned short * loadBMP(const char *path)
{
	uint8_t r, g, b;
	int size, width, height, offset, i, j, padding;
	uint16_t *returnValue;
	FILE *temp = fopen(path, "rb");
	
	if(!temp)
	{
		printf("File does not exist or can't be opened\n");
		return NULL;
	}
	// Check if the file's 2 first char are BM (indicates bitmap)
	if(!(fgetc(temp) == 0x42 && fgetc(temp) == 0x4d))
	{
		printf("Image is not a bitmap\n");
		fclose(temp);
		return NULL;
	}
	
	// Check if the file is in 24 bpp
	fseek(temp, 0x1c, SEEK_SET);
	if(fgetc(temp) != 24)
	{
		printf("Wrong format : bitmap must use 24 bpp\n");
		fclose(temp);
		return NULL;
	}
	
	// Get the 4-bytes pixel width and height, situated respectively at 0x12 and 0x16
	fseek(temp, 0x12, SEEK_SET);
	width = fgetc(temp) | (fgetc(temp) << 8) | (fgetc(temp) << 16) | (fgetc(temp) << 24);
	//fseek(temp, 0x16, SEEK_SET);
	height = fgetc(temp) | (fgetc(temp) << 8) | (fgetc(temp) << 16) | (fgetc(temp) << 24);
	size = width * height;
	
	// Gets the 4-bytes offset to the start of the pixel table, situated at 0x0a
	fseek(temp, 0x0a, SEEK_SET);
	offset = fgetc(temp) | (fgetc(temp) << 8) | (fgetc(temp) << 16) | (fgetc(temp) << 24);
	
	fseek(temp, offset, SEEK_SET);
	
	returnValue = malloc(size * sizeof(unsigned short));
	if(!returnValue)
	{
		printf("Couldn't allocate memory\n");
		fclose(temp);
		return NULL;
	}

	padding = 4 - (width * 3 - (width * 3 / 4) * 4);
	padding %= 4;
	for(j = height - 1; j >= 0; j--)
	{
		for(i = 0; i < width; i++)
		{
			b = fgetc(temp);
			g = fgetc(temp);
			r = fgetc(temp);
			returnValue[(j * width) + i] = (uint16_t)RGB2PAL_nofile(r, g, b);
		}
		for(i = 0; i < padding; i++)
			fgetc(temp);
	}
		
	fclose(temp);
	return returnValue;
}


void usage()
{
	printf("BMP to Huc KING 8bpp/16bpp\n\n");
	printf("bmp2huc.elf in.bmp out.bin bitmap_width bitmap_height bitdepth\n");
	printf("bitdepth Accepted arguments :\n");
	printf("- 2 (16bpp)\n");
	printf("- 1 (8bpp)\n");
	printf("\n\n");
	printf("For 16bpp mode, a 24bpp BMP file is expected as the input.\n\n");
	printf("For 8bpp mode, a 8bpp BMP file with a color palette is expected as the input.\n");
	printf("In this case, a .PAL and .H file are also going to be exported\n");	
}

int main (int argc, char *argv[]) 
{
	FILE *fp;
	BITMAP bmp;
	unsigned short* imagetohold;
	uint16_t towrite;
	int spr_w;
	int spr_h;
	int i, index, counter = 0;
	int bitdepth;
	char str[128];
	
	if (argc < 5)
	{
		usage();
		return 0;
	}
	
	str2int(&spr_w, argv[3], 10);
	str2int(&spr_h, argv[4], 10);
	str2int(&bitdepth, argv[5], 10);
	
	printf("width %s\n", argv[3]);
	printf("height %s\n", argv[4]);
	printf("bitdepth %s\n", argv[5]);
	
	if (bitdepth == 1)
	{
		Load_bmp(argv[1], &bmp, spr_w, spr_h);
		bmp.bytespp = 1;
		Load_palette(argv[1]);
		
		/* Palette */
		snprintf(str, sizeof(str), "%s.PAL", removestr(basename(argv[1])));
		fp = fopen(str, "wb");
		for(index=0;index<256;index++)
		{
			RGB2PAL(VGA_8158_GAMEPAL[(index*3)+0], VGA_8158_GAMEPAL[(index*3)+1], VGA_8158_GAMEPAL[(index*3)+2], fp);
		}
		fclose(fp);
		
		/* Header */
		
		snprintf(str, sizeof(str), "%s.h", removestr(basename(argv[1])));
		fp = fopen(str, "w");
		fprintf(fp, "unsigned short mypal[%d] = {\n", num_colors);
		for(index=0;index<256;index++)
		{
			RGB2PAL_text(VGA_8158_GAMEPAL[(index*3)+0], VGA_8158_GAMEPAL[(index*3)+1], VGA_8158_GAMEPAL[(index*3)+2], fp);
			counter++;
			if (counter > 7) 
			{
				fprintf(fp, "\n");
				counter = 0;
			}
		}
		fprintf(fp, "\n};\n");
		fclose(fp);

		/* KING 8BPP needs to be byteswapped !*/
		fp = fopen(argv[2], "wb");
		if (!fp) 
		{
			printf("Cannot write to output file (read-only filesystem or lack of free space perhaps ?)\n");
			return 1;
		}
		
		for(i=0;i<(spr_w * spr_h);i+=2)
		{
			towrite = ((bmp.data[i] << 8) | (bmp.data[i+1] << 0));
			fwrite (&towrite, sizeof(char), 2, fp);
		}
		if (bmp.data) free(bmp.data);
	}
	else if (bitdepth == 2)
	{
		imagetohold = loadBMP(argv[1]);
		if (!imagetohold)
		{
			printf("Invalid format or can't open file\n");
			return 0;
		}
		printf("Loaded into memory !\n");
		
		fp = fopen(argv[2], "wb");
		if (!fp) 
		{
			printf("Cannot write to output file (read-only filesystem or lack of free space perhaps ?)\n");
			return 1;
		}
		/* No need for byteswapping in 64k color mode */
		fwrite(imagetohold, 1, ((spr_w * spr_h)*bitdepth), fp);
		if (imagetohold) free(imagetohold);
	}
	else
	{
		usage();
		return 0;
	}
	
	if (fp) fclose(fp);
	printf("File written\n");
	
	return 0;
}
