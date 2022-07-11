// ****
// ****
#define VERSION1 "InfoBox v1.8 by GOst4711 and nit"
// ****
// ****
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <ft2build.h>
#include <jpeglib.h>
#include "shmE2.h"
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H
#include "readpng.h"

#if 0 //def MIPSEL
#define FBIO_BLIT 0x22
int codez = 0;
#else
#include <linux/stmfb.h>
#endif

#define alpha_composite(composite, fg, alpha, bg) {  \
	unsigned short temp = ((unsigned short)(fg)*(unsigned short)(alpha) +   \
	(unsigned short)(bg)*(unsigned short)(255 - (unsigned short)(alpha)) +  \
	(unsigned short)128);  \
	(composite) = (unsigned char)((temp + (temp >> 8)) >> 8);  \
}

// Schriftdatei setzen
#define FONT_DEF "/var/usr/share/fonts/default.ttf"
#define FONT_DEF1 "/tmp/default.ttf"
#define FONT_DEF2 "/usr/share/fonts/md_khmurabi_10.ttf"
#define FONT_DEF3 "/var/usr/share/fonts/neutrino.ttf"
char FONT[100];
//#define FONT "/usr/share/fonts/valis_enigma.ttf"

char *shm = NULL;

enum { LEFT, CENTER, RIGHT };
enum { VERY_SMALL, SMALL, BIG, USER, USER_SMALL };

// Schriftgrössen festlegen
#define FONTHEIGHT_VERY_SMALL 16
#define FONTHEIGHT_SMALL      32
#define FONTHEIGHT_BIG        40
int FONTHEIGHT_USER;
int FONTHEIGHT_USER_SMALL;

// Libfreetype Zeugs
FT_Error         error;
FT_Library       library;
FTC_Manager      manager;
FTC_SBitCache    cache;
FTC_SBit         sbit;
#if FREETYPE_MAJOR  == 2 && FREETYPE_MINOR == 0
FTC_Image_Desc   desc;
#else
FTC_ImageTypeRec desc;
#endif
FT_Face          face;
FT_UInt          prev_glyphindex;
FT_Bool          use_kerning;
FT_Error         MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface);

// Framebuffer Zeugs
unsigned char *lfb;
int StartX, StartY, fb, maxstringlen, linelen, boxx, boxy, boxx1, boxy1;
int maxlines, ende, waittime, balken, balkenlen, drawbox = 1, progressbar = 0, newprogressbar = 0, progressbarlen = 90, progressbarypos = 80;
int balkenproz, offsetx, offsety, linex, liney, pbar = 0, old_xres, old_yres, nostopSzapRC = 0, rueckzeile = 0, offsety_2;
double balkenx1 = 0, balken1count = 0, balken2count = 0;
char infoline[100] = "Information";
char line1[100] = "no text found";
char line2[100] = "no text found";
char line3[100] = "no text found";
char line4[100] = "no text found";
char line5[100] = "no text found";
char line6[100] = "no text found";
char line7[100] = "no text found";
char line8[100] = "no text found";
char line9[100] = "no text found";
char line10[100] = "no text found";
char line11[100] = "no text found";
char line12[100] = "no text found";
char line13[100] = "no text found";
char line14[100] = "no text found";
char line15[100] = "no text found";
char line16[100] = "no text found";
char line17[100] = "no text found";
char line18[100] = "no text found";
char lineall[18][100];
char kommando[100] = "kein Kommando vorhanden";
char bigestline[100] = "no text found";
char balkenout[20];
char progress_ch[4];
char cmd[100];
char *option1 = NULL;
char *jpgname = NULL;
struct fb_fix_screeninfo fix_screeninfo;
struct fb_var_screeninfo var_screeninfo;

static unsigned char *image_data, bg_red = 0, bg_green = 0, bg_blue = 0;
static unsigned long image_width, image_height, image_rowbytes;
static int image_channels;

int userprozx = 0;
int userprozy = 0;

void help()
{
	printf("\n");
	printf("Usage:\n");
	printf("infotext: infobox [-pos x%% y%%] <timeout> <label[#file.jpg]> <line1> <line2> ...\n");
	printf("infotext with timebar: infobox [-pos x%% y%%] <timeout + 10000> <label[#file.jpg]> <line1> <line2> ...\n");
	printf("infotext with bar (killall -4 infobox): infobox <percent + 15000> <label[#file.jpg]> <line1> <line2> ...\n\n");

	printf("GUI: infobox [-pos x%% y%%] GUI[#activeLine] <label[#file.jpg]> <Option1> <Option2> ...\n");

	printf("progressbar: infobox <endvalue read from /proc/progress 0-100> <progressbar[#file.jpg]> <len in percent> <y-pos in percent>\n");
	printf("only background: use text \"nobox\" as label\n\n");

	printf("textsaver: infobox <speed> saver <label> <line1> <line2> ...\n");
	printf("picturesaver (scroll): infobox <speed> saver1 <label> <png-file>\n");
	printf("picturesaver (jump): infobox <speed> saver2 <label> <png-file>\n");
	printf("\n");
}

#if 0 //def MIPSEL
unsigned char* scale(unsigned char* buf, int width, int height, int channels, int newwidth, int newheight, int free1)
{
	int h, w, pixel, nmatch;

	if (buf == NULL)
	{
		err("out -> NULL detect");
		return NULL;
	}

	unsigned char *newbuf = malloc(newwidth * newheight * channels);
	if (newbuf == NULL)
	{
		err("no mem");
		return NULL;
	}

	double scalewidth = (double)newwidth / (double)width;
	double scaleheight = (double)newheight / (double)height;

	for (h = 0; h < newheight; h++)
	{
		for (w = 0; w < newwidth; w++)
		{
			pixel = (h * (newwidth * channels)) + (w * channels);
			nmatch = (((int)(h / scaleheight) * (width * channels)) + ((int)(w / scalewidth) * channels));
			if (channels > 0)
			{
				newbuf[pixel] = buf[nmatch];
			}
			if (channels > 1)
			{
				newbuf[pixel + 1] = buf[nmatch + 1];
			}
			if (channels > 2)
			{
				newbuf[pixel + 2] = buf[nmatch + 2];
			}
			if (channels > 3)
			{
				newbuf[pixel + 3] = buf[nmatch + 3];
			}
		}
	}

	if (free1 == 1)
	{
		free(buf);
	}
	return newbuf;
}

int write_JPEG_file_to_FB(unsigned char *fb, int fb_width, int fb_height, char *filename)
{
	unsigned char *raw_image = NULL;
	unsigned char *help_fb = NULL;
	int width;
	int height;
	int bytes_per_pixel;
	int color_space;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPROW row_pointer[1];

	FILE *infile = fopen(filename, "rb");
	unsigned long location = 0;
	int i = 0;

	if (!infile)
	{
		printf("Error opening jpeg file %s\n!", filename);
		return -1;
	}
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);

 	printf("width:%d height%d\n", fb_width, fb_height);
	printf("JPEG File Information: \n");
	printf("Image width and height: %d pixels and %d pixels.\n", width=cinfo.image_width, height=cinfo.image_height);
	printf("Color components per pixel: %d.\n", bytes_per_pixel = cinfo.num_components);
	printf("Color space: %d.\n", cinfo.jpeg_color_space);

	jpeg_start_decompress(&cinfo);
	raw_image = (unsigned char*)malloc(cinfo.output_width*cinfo.output_height*cinfo.num_components);
	row_pointer[0] = (unsigned char *)malloc(cinfo.output_width*cinfo.num_components);

	while (cinfo.output_scanline < cinfo.image_height)
	{
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
		for (i = 0; i < cinfo.image_width * cinfo.num_components; i++)
		{
			raw_image[location++] = row_pointer[0][i];
		}
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(row_pointer[0]);
	fclose(infile);

	size_t helpb = 0;
	size_t help = 0;

	if (width != fb_width || height != fb_height)
	{
		char* buf = NULL;
		buf = scale(raw_image, width, height, 3, fb_width, fb_height, 0);
		free(raw_image);
		raw_image = NULL;
		raw_image = buf;
		buf = NULL;
	}

	while (help <= (fb_width * fb_height * 4))
	//while (help <= (width * height * 4))
	{
		fb[help + 0] = raw_image[helpb + 2];
		fb[help + 1] = raw_image[helpb + 1];
		fb[help + 2] = raw_image[helpb + 0];
		//fb[help + 3] = 0x00;
		fb[help + 3] = 0xFF;
		help = help + 4;
		helpb = helpb + 3;
	}
	free(raw_image);
	return 1;
}
#endif

int	blit(int srcx, int srcy)
{
#if 0 //def	MIPSEL
	ioctl(fb, FBIO_BLIT);
#else
	STMFBIO_BLT_DATA  bltData;
	memset(&bltData, 0, sizeof(STMFBIO_BLT_DATA));

	bltData.operation  = BLT_OP_COPY;
	bltData.srcOffset  = var_screeninfo.xres * var_screeninfo.yres * 4;
	bltData.srcPitch   = srcx * 4;
	bltData.src_top    = 0;
	bltData.src_left   = 0;
	bltData.src_right  = srcx;
	bltData.src_bottom = srcy;
	bltData.srcFormat  = SURF_BGRA8888;
	bltData.srcMemBase = STMFBGP_FRAMEBUFFER;
	bltData.dstOffset  = 0;
	bltData.dstPitch   = var_screeninfo.xres * 4;
	bltData.dst_top    = 0;
	bltData.dst_left   = 0;
	bltData.dst_right  = var_screeninfo.xres;
	bltData.dst_bottom = var_screeninfo.yres;
	bltData.dstFormat  = SURF_BGRA8888;
	bltData.dstMemBase = STMFBGP_FRAMEBUFFER;
	if (ioctl(fb, STMFBIO_BLT, &bltData) < 0)
	{
		perror("FBIO_BLIT");
	}
	return 0;
#endif
}

void readprogress(void)
{
	int progress_fd = open("/proc/progress", O_RDONLY);

	read(progress_fd, progress_ch, 4);
	close (progress_fd);
	
	progress_ch[3] = '\0';
	newprogressbar = atoi(progress_ch);
}

// open jpg for background
int openjpg(const char* Name)
{
	unsigned char r,g,b;
	int width, height, x, row_stride;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned char *pDummy;
	char strUseJpegBGx[20];
	char strUseJpegBGy[20];
	unsigned long color;

	FILE *infile;
	JSAMPARRAY pJpegBuffer;

	if ((infile = fopen(Name, "rb")) == NULL)
	{
		printf("cannot open %s\n", Name);
		return 1;
	}

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	width = cinfo.output_width;
	height = cinfo.output_height;

	pDummy = lfb + var_screeninfo.xres * var_screeninfo.yres * 4;

	row_stride = width * cinfo.output_components ;
	pJpegBuffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, pJpegBuffer, 1);
		for (x = 0; x < width; x++)
		{
			r = pJpegBuffer[0][cinfo.output_components * x];
			if (cinfo.output_components > 2)
			{
				g = pJpegBuffer[0][cinfo.output_components * x + 1];
				b = pJpegBuffer[0][cinfo.output_components * x + 2];
			}
			else
			{
				g = r;
				b = r;
			}
			color = (0 << 24) | (r << 16) | (g << 8) | b;
			memcpy(pDummy + x * 4 + (width * 4) * (cinfo.output_scanline - 1), &color, 4);
		}
	}
	fclose(infile);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	sprintf(strUseJpegBGx, "%s=%d", "useJpegBGx", width);
	setshmentry(shm, strUseJpegBGx);
	sprintf(strUseJpegBGy, "%s=%d", "useJpegBGy", height);
	setshmentry(shm, strUseJpegBGy);

	printf("jpg image hight = %ld\n", (long int)height);
	printf("jpg image width = %ld\n", (long int)width);

	blit(width, height);
	return 0;
}

// open png for screensaver
int openpng(void)
{
	FILE *infile;
	int rc;

	if (!(infile = fopen(line1, "rb")))
	{
		printf("cannot open PNG file [%s]\n", line1);
		return 1;
	}
	if ((rc = readpng_init(infile, &image_width, &image_height)) != 0)
	{
		switch (rc)
		{
			case 1:
			{
				printf("[%s] is not a PNG file:\n", line1);
				fclose(infile);
				return 1;
			}
			case 2:
			{
				printf("[%s] has bad IHDR (libpng longjmp)\n", line1);
				fclose(infile);
				return 1;
			}
			case 4:
			{
				printf("insufficient memory\n");
				fclose(infile);
				return 1;
			}
			default:
			{
				printf("unknown readpng_init() error\n");
				fclose(infile);
				return 1;
			}
		}
	}
	image_data = readpng_get_image(2.2 , &image_channels, &image_rowbytes);
	readpng_cleanup(0);
	fclose(infile);

	if (!image_data)
	{
		printf("unable to decode PNG image\n");
		return 1;
	}
	printf("png image height = %ld\n", image_height);
	printf("png image width  = %ld\n", image_width);
	printf("image channels   = %d\n", image_channels);
	printf("image rowbytes   = %ld\n", image_rowbytes);
	return 0;
}

// draw png for screensaver
void showpng(int posx, int posy, int del)
{
	unsigned char *src, r, g, b, a, red, green, blue;
	unsigned long color, i, row;

	for (row = 0; row < image_height; ++row)
	{
		src = image_data + row * image_rowbytes;
		for (i = 0; i < image_width; i++)
		{
			r = *src++;
			g = *src++;
			b = *src++;
			if (image_channels == 3)
			{
				a = 255;
			}
			else
			{
				a = *src++;
			}
			if (a == 255)
			{
				red = r;
				green = g;
				blue = b;
			}
			else if (a == 0)
			{
				red = bg_red;
				green = bg_green;
				blue = bg_blue;
			}
			else
			{
				alpha_composite(red, r, a, bg_red);
				alpha_composite(green, g, a, bg_green);
				alpha_composite(blue, b, a, bg_blue);
			}
		
			if (del == 1)
			{
				color = 0;
			}
			else
			{
				color = (0 << 24) | (red << 16) | (green << 8) | blue;
			}
			memcpy(lfb + i * 4 + posx * 4 + fix_screeninfo.line_length * (row + posy), &color, 4);
		}
	}
}

// Freetype - MyFaceRequester
FT_Error MyFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face *aface)
{
	FT_Error result;

	result = FT_New_Face(library, face_id, 0, aface);

	if (result)
	{
		printf("Freetype <Font \"%s\" failed>\n", (char*)face_id);
	}
	return result;
}

// Freetype - RenderChar
int RenderChar(FT_ULong currentchar, int sx, int sy, int ex, unsigned char *color)
{
	int test = 0, space = 0;
	int row, pitch, bit, x = 0, y = 0;
	FT_UInt glyphindex;
	FT_Vector kerning;
	FT_Error error;
//	int tmpcolor;

	if (currentchar == 32)
	{
		currentchar = 65;
		space = 1;
	}
	if (sx == -1)
	{
		sx = 1;
		sy = 1;
		ex = 100;
		test = 1;
	}

	if (!(glyphindex = FT_Get_Char_Index(face, currentchar)))
	{
		printf("Freetype <FT_Get_Char_Index> for character %x \"%c\" failed\n", (int)currentchar, (int)currentchar);
		return 0;
	}

#if FREETYPE_MAJOR == 2 && FREETYPE_MINOR == 0
	if ((error = FTC_SBit_Cache_Lookup(cache, &desc, glyphindex, &sbit)))
#else
	FTC_Node anode;
	if ((error = FTC_SBitCache_Lookup(cache, &desc, glyphindex, &sbit, &anode)))
#endif
	{
		printf("Freetype <FTC_SBitCache_Lookup> for character %x \"%c\" failed. Error: 0x%.2X>\n", (int)currentchar, (int)currentchar, error);
		return 0;
	}
	if (use_kerning)
	{
		FT_Get_Kerning(face, prev_glyphindex, glyphindex, ft_kerning_default, &kerning);
		prev_glyphindex = glyphindex;
		kerning.x >>= 6;
	}
	else
	{
		kerning.x = 0;
	}
	if (sx + sbit->xadvance > ex)
	{
		return -1;
	}
	for (row = 0; row < sbit->height; row++)
	{
		for (pitch = 0; pitch < sbit->pitch; pitch++)
		{
			for (bit = 7; bit >= 0; bit--)
			{
				if (pitch * 8 + 7 - bit >= sbit->width)
				{
					break; /* render needed bits only */
				}
				if ((sbit->buffer[row * sbit->pitch + pitch]) & 1 << bit)
				{
					if (test == 0 && space == 0)
					{
						memcpy(lfb + StartX * 4 + sx * 4 + (sbit->left + kerning.x + x) * 4 + fix_screeninfo.line_length * (StartY + sy - sbit->top + y), color, 4);
					}
				}
				x++;
			}
		}
		x = 0;
		y++;
	}
	return sbit->xadvance + kerning.x;
}

// Freetype - Stringlaenge
int GetStringLen(const char *string, int size)
{
	int stringlen = 0;
	FT_ULong help;
	const char helpc1 = 0xC0;
	const char helpc2 = 0xDF;
	const char helpc3 = 0x80;
	const char helpc4 = 0xBF;

	switch (size)
	{
		case VERY_SMALL:
		{
			desc.width = desc.height = FONTHEIGHT_VERY_SMALL;
			break;
		}
		case SMALL:
		{
			desc.width = desc.height = FONTHEIGHT_SMALL;
			break;
		}
		case BIG:
		{
			desc.width = desc.height = FONTHEIGHT_BIG;
			break;
		}
		case USER:
		{
			desc.width = desc.height = FONTHEIGHT_USER;
			break;
		}
		case USER_SMALL:
		{
			desc.width = desc.height = FONTHEIGHT_USER_SMALL;
			break;
		}
	}
	prev_glyphindex = 0;
	while (*string != '\0' && *string != '\n')
	{
		if ((int)*string < 0)
		{
			if ((*string >= helpc1) && (*string <= helpc2))
			{
			 	if ((*(string) + 1 > helpc3) && (*(string + 1) <= helpc4))
				{
					help = (*string - 0xc0) * 0x40;
					string++;
					help = help + (*string - 0x80);
					help = help - 0xFFFFbf00;
					stringlen += RenderChar(help, -1, -1, -1, (unsigned char *)"");
				}
				else
				{
					help = *string;
					help = help - 0xFFFFFF00;
					stringlen += RenderChar(help, -1, -1, -1, (unsigned char *)"");
				}
			}
			else
			{
				help = *string;
				help = help - 0xFFFFFF00;
				stringlen += RenderChar(help, -1, -1, -1, (unsigned char *)"");
			}
		}
		else
		{
			stringlen += RenderChar(*string, -1, -1, -1, (unsigned char *)"");
		}
		string++;
	}
	return stringlen;
}

// Freetype - Render String
void RenderString(const char *string, int sx, int sy, int maxwidth, int layout, int size, unsigned char *color)
{
	FT_ULong help;
	const char helpc1 = 0xC0;
	const char helpc2 = 0xDF;
	const char helpc3 = 0x80;
	const char helpc4 = 0xBF;
	int stringlen, ex = 0, charwidth;

	switch (size)
	{
		case VERY_SMALL:
		{
			desc.width = desc.height = FONTHEIGHT_VERY_SMALL;
			break;
		}
		case SMALL:
		{
			desc.width = desc.height = FONTHEIGHT_SMALL;
			break;
		}
		case BIG:
		{
			desc.width = desc.height = FONTHEIGHT_BIG;
			break;
		}
		case USER:
		{
			desc.width = desc.height = FONTHEIGHT_USER;
			break;
		}
		case USER_SMALL:
		{
			desc.width = desc.height = FONTHEIGHT_USER_SMALL;
			break;
		}
	}
	if (layout != LEFT)
	{
		stringlen = GetStringLen(string, size);
		switch(layout)
		{
			case CENTER:
			{
				if (stringlen < maxwidth)
				{
					sx += (maxwidth - stringlen) / 2;
				}
				break;
			}
			case RIGHT:
			{
				if (stringlen < maxwidth)
				{
					sx += maxwidth - stringlen;
				}
			}
		}
	}
	prev_glyphindex = 0;
	ex = sx + maxwidth;
	while (*string != '\0' && *string != '\n')
	{
		if ((int)*string < 0)
		{
			if ((*string >= helpc1) && (*string <= helpc2))
			{
			 	if ((*(string) + 1 > helpc3) && (*(string + 1) <= helpc4))
				{
					help = (*string - 0xc0) * 0x40;
					string++;
					help = help + (*string - 0x80);
					help = help - 0xFFFFbf00;
					if ((charwidth = RenderChar(help, sx, sy, ex, color)) == -1)
					{
						return; /* string > maxwidth */
					}
				}
				else
				{
					help = *string;
					help = help - 0xFFFFFF00;
					if ((charwidth = RenderChar(help, sx, sy, ex, color)) == -1)
					{
						return; /* string > maxwidth */
					}
				}
			}
			else
			{
				help = *string;
				help = help - 0xFFFFFF00;
				if ((charwidth = RenderChar(help, sx, sy, ex, color)) == -1)
				{
					return; /* string > maxwidth */
				}
			}
		}
		else
		{
			if ((charwidth = RenderChar(*string, sx, sy, ex, color)) == -1)
			{
				return; /* string > maxwidth */
			}
		}
		sx += charwidth;
		string++;
	}
}

// Render Box
void RenderBox(int sx, int sy, int ex, int ey, unsigned char *color)
{
//	int loop;
	int tx;

	for (; sy <= ey; sy++)
	{
		for (tx = 0; tx <= (ex-sx); tx++)
		{
			memcpy(lfb + StartX * 4 + sx * 4 + (tx * 4) + fix_screeninfo.line_length * (StartY + sy), color, 4);
		}
	}
}

void SetOffset()
{
	// 720 x 576
	// 1280 x 720
	// 1920 x 1080
	FONTHEIGHT_USER = 20 * var_screeninfo.yres / 576;  // schrifthoehe gross
	FONTHEIGHT_USER_SMALL = 14 * var_screeninfo.yres / 576;  // schrifthoehe klein
	offsetx = 5 * var_screeninfo.xres / 720;  // abstand zw. zeilen x
	offsety = 5 * var_screeninfo.yres / 576;  // abstand zw. zeilen y
	linex = 5 * var_screeninfo.xres / 720;  // linienbreite x
	liney = 5 * var_screeninfo.yres / 576;  // linienbreite y
}

void SetMaxStringLen()
{
	maxstringlen = GetStringLen(bigestline, USER);
}

// calculate Box Position
void SetBoxPos()
{
	boxx = (var_screeninfo.xres / 2) - ((maxstringlen+(offsetx * 2) + (linex * 2)) / 2);
	boxx1 = boxx + (maxstringlen + (offsetx * 2) + (linex * 2));
	boxy = (var_screeninfo.yres / 2) - (((maxlines + 1) * FONTHEIGHT_USER) + (pbar * FONTHEIGHT_USER_SMALL) + ((maxlines + 2 + pbar) * offsety) + ((3 + pbar) * liney)) / 2;
	boxy1 = boxy + (((maxlines + 1) * FONTHEIGHT_USER) + ((maxlines + 2) * offsety) + (3 * liney));
	if (userprozx > -2)
	{
		int userposx = var_screeninfo.xres * userprozx / 100;
		int userposy = var_screeninfo.yres * userprozy / 100;
	
		if (userprozx > -1)
		{
			if (boxx1 - boxx + userposx <= var_screeninfo.xres)
			{
				boxx = userposx;
				boxx1 =  boxx + (maxstringlen + (offsetx * 2) + (linex * 2));
				boxx = userposx;
				boxx1 = boxx + (maxstringlen + (offsetx * 2) + (linex * 2));
			}
			else
			{
				boxx = var_screeninfo.xres - (boxx1 - boxx);
				boxx1 = boxx + (maxstringlen + (offsetx * 2) + (linex * 2));
			}
		}
		if (userprozy > -1)
		{
			if (boxy1 - boxy + userposy <= var_screeninfo.yres)
			{
				boxy = userposy;
				boxy1 =  boxy + (((maxlines + 1) * FONTHEIGHT_USER) + ((maxlines + 2) * offsety) + (3 * liney));
			}
			else
			{
				boxy = var_screeninfo.yres - (boxy1 - boxy);
				boxy1 = boxy + (((maxlines + 1) * FONTHEIGHT_USER) + ((maxlines + 2) * offsety) + (3 * liney));
			}
		}
	}
}

// calculate Balken Position
void SetBalkenPos()
{
	if (progressbar == 1)
	{
		balkenlen = maxstringlen = ((var_screeninfo.xres / 100) * progressbarlen);
		boxx = (var_screeninfo.xres / 2) - ((balkenlen + (linex * 2)) / 2);
		boxx1 = boxx + (balkenlen + (linex * 2));
		boxy = ((var_screeninfo.yres / 100) * progressbarypos);
		boxy1 = boxy + FONTHEIGHT_USER + (liney * 2);
	}
	else
	{
		balkenlen = (boxx1 - boxx) - (linex * 2);
	}
}

// Draw Box
void DrawBox(void)
{
	if (drawbox)
	{
		offsety_2 = offsety / 2;
		RenderBox(boxx, boxy, boxx1, boxy1, (unsigned char *)"\x44\x2E\x24\xFF"); // hintergrundbox
		RenderBox(boxx + linex, boxy + liney, boxx1 - linex, boxy + liney + FONTHEIGHT_USER + offsety, (unsigned char *)"\x1c\x02\x04\xFF"); //Labelbox
		RenderBox(boxx + linex, boxy + liney * 2 + FONTHEIGHT_USER + offsety, boxx1 - linex, boxy1 - liney, (unsigned char *)"\x26\x20\x1F\xFF"); //Textbox
	
		RenderString(infoline, boxx + linex + offsetx, boxy + liney + FONTHEIGHT_USER * 1 + offsety_2, maxstringlen, CENTER, USER, (unsigned char *)"\x00\xD8\xFF\xFF");
		RenderString(line1, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 2 + offsety * 1 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		if (maxlines > 1)
		{
			RenderString(line2, boxx + linex + offsetx, boxy+liney * 2 + FONTHEIGHT_USER * 3 + offsety * 2 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 2)
		{
			RenderString(line3, boxx + linex+offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 4 + offsety * 3 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 3)
		{
			RenderString(line4, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 5 + offsety * 4 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 4)
		{
			RenderString(line5, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 6 +offsety * 5 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 5)
		{
			RenderString(line6, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 7 + offsety * 6 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 6)
		{
			RenderString(line7, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 8 + offsety * 7 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 7)
		{
			RenderString(line8, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 9 + offsety * 8 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 8)
		{
			RenderString(line9, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 10 + offsety * 9 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 9)
		{
			RenderString(line10, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 11 + offsety * 10 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 10)
		{
			RenderString(line11, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 12 + offsety * 11 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 11)
		{
			RenderString(line12, boxx + linex+offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 13 + offsety * 12 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 12)
		{
			RenderString(line13, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 14 + offsety * 13 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 13)
		{
			RenderString(line14, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 15 + offsety * 14 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 14)
		{
			RenderString(line15, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 16 + offsety * 15 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 15)
		{
			RenderString(line16, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 17 + offsety * 16 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 16)
		{
			RenderString(line17, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 18 + offsety * 17 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
		if (maxlines > 17)
		{
			RenderString(line18, boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * 19 + offsety * 18 + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
		}
	}
	
	if (balken > 0 && progressbar == 0)
	{
		RenderBox(boxx, boxy1 + offsety, boxx1, boxy1 + liney * 2 + offsety + FONTHEIGHT_USER_SMALL, (unsigned char *)"\x44\x2E\x24\xFF");
		RenderBox(boxx + linex, boxy1 + liney + offsety, boxx + linex + balkenlen, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, (unsigned char *)"\x26\x20\x1F\xFF");
	}
}

// detect video mode change
int videomodechange()
{
	int ret = 0;

	ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo);
	ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo);

	if (var_screeninfo.xres != old_xres || var_screeninfo.yres != old_yres)
	{
		old_xres = var_screeninfo.xres;
		old_yres = var_screeninfo.yres;
		sleep(1.5);
	
		if (jpgname != NULL && progressbar == 0)
		{
			memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);
#if 0 //def MIPSEL
			write_JPEG_file_to_FB(lfb, var_screeninfo.xres, var_screeninfo.yres, jpgname);
#else
			openjpg(jpgname);
#endif
		}
		else
		{
			memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);
		}
		SetOffset();
		SetMaxStringLen();
		SetBoxPos();
		ret = 1;
	}
	return ret;
}

int file_ttyAS1;

void SIG_Handler(int sig)
{
	switch(sig)
	{
		case 4:
		{
			if (balken == 2)
			 {
				balken2count = balken2count + balkenproz;
				if (balken2count < 100)
				{
					RenderString(balkenout, boxx + offsetx + linex, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, maxstringlen, CENTER, USER_SMALL, (unsigned char *)"\x26\x20\x1F\xFF");
					balkenx1 = (balkenlen / 100.00) * (float)balken2count;
					RenderBox(boxx + linex, boxy1 + liney + offsety, boxx + linex + (int)balkenx1, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, (unsigned char *)"\xFF\x80\x00\xFF");
					sprintf(balkenout, "%0.2f%s", (float)balken2count, "%");
					RenderString(balkenout, boxx + offsetx + linex, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, maxstringlen, CENTER, USER_SMALL, (unsigned char *)"\xFF\xFF\xFF\xFF");
				}
				else
				{
					sprintf(balkenout, "%0.2f%s", 100.00, "%");
					RenderBox(boxx + linex, boxy1 + liney + offsety, boxx + linex + balkenlen, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, (unsigned char *)"\xFF\x80\x00\xFF");
					RenderString(balkenout, boxx + offsetx + linex, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, maxstringlen, CENTER, USER_SMALL, (unsigned char *)"\xFF\xFF\xFF\xFF");
				}
			}
			break;
		}
		case 9:
		case 15:
		{
			if (strcmp(kommando, "GUI") == 0)
			{
				close (file_ttyAS1);
			}
			ende = 1;
			break;
		}
	}
}

#define TRUE    1
#define FALSE   0

#define EXIT    1
#define OK      28
#define UP      24
#define DOWN    25
#define RED     30
#define KEY_ESC 1


int key;
int RCtrl;     /* Remote Control file drescriptor */
int RCtrlCode; /* Remote Control KeyPressed */
int	anzKeyHoch = 0;
int	anzKeyRunter = 0;
int	anzKeyOK = 0;

int GetRCtrlCode()
{
	int ret_val;
//	int value;
	int nbytes;
	int mbytes;
//	char *ReturnCode;
//	unsigned char data[100];
	unsigned char data[32];

	char *dataptr;

	printf ("GetRCtrlCode\n");
	ret_val = 0;

	while (ret_val == 0)
	{
		dataptr = (char*)data;
		nbytes = 0;
		mbytes = 0;

		while ((nbytes = read(file_ttyAS1, dataptr, 32 - mbytes)) > 0)
		{
			dataptr += (char)nbytes;
			mbytes += nbytes;
			if (mbytes >= 32)
			{
				break;
			}
#if 0
			if (dataptr[-1] == '\n' || dataptr[-1] == '\r')
			{
				break;
			}
#endif
		}
#if 0
		nbytes = read(file_ttyAS1, dataptr, 16);
		ReturnCode = fgets(data, 32, file_ttyAS1);

		fseek (file_ttyAS1, 1, SEEK_SET);
		nbytes = read(file_ttyAS1, dataptr, 32);
#endif
		if (ende == 1)
		{
			ret_val = EXIT;
		}
		else
		{
#if 0 //def MIPSEL
			if (codez != 0)
			{
				codez = 0;
				return 0;
			}
			else
			{
				codez = 1;
			}
#endif
/*
			printf("data %x\n",data);
			printf("0-15:\t0=%x\t1=%x\t2=%x\t3=%x\t4=%x\t5=%x\t6=%x\t7=%x\t8=%x\t9=%x 10=%x\t11=%x\t12=%x\t13=%x\t14=%x\t15=%x\n" ,data[0] ,data[1] ,data[2] ,data[3] ,data[4] ,data[5] ,data[6], data[7], data[8], data[9], data[10], data[11],data[12],data[13],data[14],data[15]);
			printf("16-31:\t16=%x\t17=%x\t18=%x\t19=%x\t20=%x\t21=%x\t22=%x\t23=%x\t24=%x\t25=%x\t26=%x\t27=%x\t28=%x\t29=%x\t30=%x\t31=%x\n",data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25],data[26], data[27],data[28],data[29],data[30],data[31]);
			printf("32-47:\t32=%x\t33=%x\t34=%x\t35=%x\t36=%x\t37=%x\t38=%x\t39=%x\t40=%x\t41=%x\t42=%x\t43=%x\t44=%x\t45=%x\t46=%x\t47=%x\n",data[32],data[33],data[34],data[35],data[36],data[37],data[38],data[39],data[40],data[41],data[42], data[43],data[44],data[45],data[46],data[47]);
*/
			switch (data[10])
			{
				case 0x67:
				{
					anzKeyHoch = anzKeyHoch + 1;
					anzKeyOK = 0;
					anzKeyRunter = 0;
					ret_val = UP;
					printf("KeyUP data[10]\n");
					break;
				}
				case 0x6c:
				{
					anzKeyRunter = anzKeyRunter + 1;
					anzKeyOK = 0;
					anzKeyHoch = 0;
					ret_val = DOWN;
					printf("KeyDown data[10]\n");
					break;
				}
				case 0x60:
				{
					anzKeyOK = anzKeyOK + 1;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					ret_val = OK;
					printf("KeyOK data[10]\n");
					break;
				}
		   		case 0x8e:
				{
					anzKeyOK = 0;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					ret_val = RED;
					printf("KeyRED data[10]\n");
					break;
				}
				case 0x66:
				case 0x9e: //ipbox exit code
				{
					anzKeyOK = 0;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					ret_val = EXIT;
					printf("KeyEXIT data[10]\n");
					break;
				}
				case 0xae: //Marvel exit code
				{
					anzKeyOK = 0;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					ret_val = EXIT;
					printf("KeyEXIT data[10]\n");
					break;				 		
				}
	 			default:
				{
					anzKeyOK = 0;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					break;
				}
 		 	}
#ifdef DOUBLE
			switch (data[26])
			{
				case 0x67:
				{
					anzKeyHoch = anzKeyHoch + 1;
					anzKeyOK = 0;
					anzKeyRunter = 0;
					ret_val = UP;
					printf("KeyUP data[26]\n");
					break;
				}
				case 0x6c:
				{
					anzKeyRunter = anzKeyRunter + 1;
					anzKeyOK = 0;
					anzKeyHoch = 0;
					ret_val = DOWN;
					printf("KeyDown data[26]\n");
					break;
				}
				case 0x60:
				{
					anzKeyOK = anzKeyOK + 1;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					ret_val = OK;
					printf("KeyOK data[26]\n");
					break;
				}
				case 0x8e:
				{
					anzKeyOK = 0;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					ret_val = RED;
					printf("KeyRED data[26]\n");
					break;
				}
				case 0x66:
				case 0x9e: //ipbox exit code
				{
					anzKeyOK = 0;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					ret_val = EXIT;
					printf("KeyEXIT data[26]\n");
					break;				 		
				}
				case 0xae: //Marvel exit code
				{
					anzKeyOK = 0;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					ret_val = EXIT;
					printf("KeyEXIT data[26]\n");
					break;				 		
				}
				default:
				{
					anzKeyOK = 0;
					anzKeyHoch = 0;
					anzKeyRunter = 0;
					break;
				}
 		 	}
#endif
 		}	 	 	
		usleep(50000);
	}
	return ret_val;
}


// Hauptroutine

int main(argc, argv)
int argc;
char *argv[];
{
	int useJpegBGx = 0, useJpegBGy = 0;
	char *shmbuf = NULL;
	int actlineset = 0;
	int pa = 0;

	printf("*** %s ***\n", VERSION1);

	if (argc == 2 && strcmp(argv[1], "-h") == 0)
	{
		help();
		exit(0);
	}

	//open shm to E2
	shm = findshm();

	ende = 0;
	signal(15, SIG_Handler);
	signal(4, SIG_Handler);

	// Setze Offsets (Startpunkt links oben), der Framebuffer hat 720x576pixel,
	// danke Overscan der TV Geräte meist nur ca. 640x480 nutzbar
	StartX = 0;
	StartY = 0;
	FONTHEIGHT_USER = 20;
	FONTHEIGHT_USER_SMALL = 14;

	FILE *datei;
	sprintf(FONT, "%s", FONT_DEF);
	datei = fopen(FONT, "r");
	if (datei == NULL)
	{
		sprintf(FONT, "%s", FONT_DEF1);
		datei=fopen(FONT, "r");
		if (datei == NULL)
		{
			sprintf(FONT, "%s" ,FONT_DEF2);
			datei=fopen(FONT, "r");
			if (datei == NULL)
			{
				sprintf(FONT, "%s" ,FONT_DEF3);
			}
			else
			{
				fclose(datei);
			}
		}
		else
		{
			fclose(datei);
		}
	}
	else
	{
		fclose(datei);
	}
	char tmpbuf[255];
	FILE *fil;
	fil = fopen("/var/etc/infobox.ini", "r");
	if (fil != NULL)
	{
		while (fgets(tmpbuf, sizeof(tmpbuf), fil))
		{
			if (strncmp(tmpbuf, "font=", 5) == 0)
			{
				char *opt = &tmpbuf[5];
				sprintf(FONT, "%s", opt);
			}
			if (strncmp(tmpbuf, "fontcolor=", 10) == 0)
			{
//				char *opt = &tmpbuf[10];
				//sprintf(fontcolor, "%s", opt);
			}
			if (strncmp(tmpbuf, "fontheight=", 11) == 0)
			{
//				char *opt = &tmpbuf[11];
				//sprintf(fontcolor, "%s", opt);
			}
			if (strncmp(tmpbuf, "smallfontheight=", 16) == 0)
			{
//				char *opt = &tmpbuf[16];
				//sprintf(fontcolor, "%s", opt);
			}
			if (strncmp(tmpbuf, "linecolor=", 10) == 0)
			{
//				char *opt = &tmpbuf[10];
				//sprintf(fontcolor, "%s", opt);
			}
			if (strncmp(tmpbuf, "linexwidth=", 11) == 0)
			{
//				char *opt = &tmpbuf[11];
				//sprintf(fontcolor, "%s", opt);
			}
			if (strncmp(tmpbuf, "linexwidth=", 11) == 0)
			{
//				char *opt = &tmpbuf[11];
				//sprintf(fontcolor, "%s", opt);
			}
			if (strncmp(tmpbuf, "offset=", 7) == 0)
			{
//				char *opt = &tmpbuf[7];
				//sprintf(fontcolor, "%s", opt);
			}
			if (strncmp(tmpbuf, "backcolor=", 10) == 0)
			{
//				char *opt = &tmpbuf[10];
				//sprintf(fontcolor, "%s", opt);
			}
		}
		fclose (fil);
	}

	// Framebuffer oeffnen
	fb = open("/dev/fb0", O_RDWR);
	if (fb == -1)
	{
		printf("Framebuffer failed\n");
		return -1;
	}

	// Framebuffer initialisieren und Infos auslesen
	if (ioctl(fb, FBIOGET_FSCREENINFO, &fix_screeninfo) == -1)
	{
		printf("Framebuffer: <FBIOGET_FSCREENINFO failed>\n");
		return -1;
	}

	if (ioctl(fb, FBIOGET_VSCREENINFO, &var_screeninfo) == -1)
	{
		printf("Framebuffer: <FBIOGET_VSCREENINFO failed>\n");
		return -1;
	}

	printf("%dx%d, %dbpp\n", var_screeninfo.xres, var_screeninfo.yres, var_screeninfo.bits_per_pixel);

	// Framebuffer in Speicher mappen
	if (!(lfb = (unsigned char*)mmap(0, fix_screeninfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0)))
	{
		printf("Framebuffer: <Memory mapping failed>\n");
		return -1;
	}
	old_xres = var_screeninfo.xres;
	old_yres = var_screeninfo.yres;

#ifdef USE_BLIT
#ifndef MIPSEL
	lfb += 1920 * 1080 * 4;
#endif
#endif

	// Freetype initialisieren
	if ((error = FT_Init_FreeType(&library)))
	{
		printf("Freetype <FT_Init_FreeType failed. Error: 0x%.2X>", error);
		munmap(lfb, fix_screeninfo.smem_len);
		return -1;
	}

	if ((error = FTC_Manager_New(library, 1, 2, 0, &MyFaceRequester, NULL, &manager)))
	{
		printf("Freetype <FTC_Manager_New failed. Error: 0x%.2X>\n", error);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		return -1;
	}

	if ((error = FTC_SBitCache_New(manager, &cache)))
	{
		printf("Freetype <FTC_SBitCache_New failed. Error: 0x%.2X>\n", error);
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		return -1;
	}

	if ((error = FTC_Manager_LookupFace(manager, FONT, &face)))
	{
		printf("Freetype <FTC_Manager_Lookup_Face failed. Error: 0x%.2X>\n", error);
		FTC_Manager_Done(manager);
		FT_Done_FreeType(library);
		munmap(lfb, fix_screeninfo.smem_len);
		return -1;
	}
	else
	{
		desc.face_id = FONT;
	}
	use_kerning = FT_HAS_KERNING(face);

#if FREETYPE_MAJOR  == 2 && FREETYPE_MINOR == 0
	desc.image_type = ftc_image_mono;
#else
	desc.flags = FT_LOAD_MONOCHROME;
#endif
	SetOffset();
	balken = 0;
	waittime = 3;
	sprintf(line1, "No data");
	sprintf(infoline, "Information");

	if (argc > 1)
	{
		if (strcmp(argv[1], "-pos") == 0)
		{
			userprozx = atoi(argv[2]);
			userprozy = atoi(argv[3]);
			pa = 3;
			argc = argc - 3;
		}
		else
		{
			userprozx = -2;
			userprozy = -2;
			pa = 0;
		}
	}

	if (argc > 1)
	{
		sprintf(kommando, "%s", argv[pa + 1]);
		if (strstr(kommando, "GUI") == NULL)
		{
			waittime = atoi(argv[pa + 1]);

			if ((waittime >= 10000) & (waittime < 15000))
			{
				balken = 1;
				waittime = waittime - 10000;
				pbar = 1;
			}
			if (waittime >= 15000)
			{
				balken = 2;
				balkenproz = waittime - 15000;
				waittime = 1000;
				pbar = 1;
			}
			if (waittime < 1)
			{
				waittime = 1;
			}
		}
		else
		{
			if (strstr(kommando, "#") != NULL)
			{
				strtok(kommando, "#");
				option1 = strtok(NULL, "#");
				actlineset = atoi(option1);
			}
		}
	}
	if (argc > 2)
	{
		sprintf(infoline, "%s", argv[pa + 2]);
	}
	
	shmbuf = malloc(6);
	getshmentry(shm, "useJpegBGx=", shmbuf, 6);
	useJpegBGx = atoi(shmbuf);
	getshmentry(shm, "useJpegBGy=", shmbuf, 6);
	useJpegBGy = atoi(shmbuf);
	free(shmbuf);

	strtok(infoline, "#");
	option1=strtok(NULL, "#");

	if (option1 != NULL && strcmp(option1, "nostopSzapRC") == 0)
	{
		nostopSzapRC = 1;
		option1 = NULL;
	}

	if (option1 != NULL)
	{
		jpgname = option1;
#if 0 //def MIPSEL
		write_JPEG_file_to_FB(lfb, var_screeninfo.xres, var_screeninfo.yres, jpgname);
#else
		openjpg(jpgname);
#endif
	}
	else
	{
		if (useJpegBGx == 0 && useJpegBGy == 0)
		{
			memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);
		}
	}

	if (strcmp(infoline, "nobox") == 0)
	{
		drawbox = 0;
	}
	maxstringlen = GetStringLen(infoline, USER);
	sprintf(bigestline, "%s", infoline);

	maxlines = 1;
	if (argc > 3)
	{
		maxlines = argc - 3;
		sprintf(line1, "%s", argv[pa + 3]);
	}
	linelen = GetStringLen(line1, USER);
	if (linelen > maxstringlen)
	{
		maxstringlen = linelen;
		sprintf(bigestline, "%s", line1);
	}
	sprintf(lineall[1], "%s", line1);

	if (maxlines > 1)
	{
		sprintf(line2, "%s", argv[pa + 4]);
		linelen = GetStringLen(line2, USER);
		sprintf(lineall[2], "%s", line2);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line2);
		}
	}
	if (maxlines > 2)
	{
		sprintf(line3, "%s", argv[pa + 5]);
		linelen = GetStringLen(line3, USER);
		sprintf(lineall[3], "%s", line3);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line3);
		}
	}
	if (maxlines > 3)
	{
		sprintf(line4, "%s", argv[pa + 6]);
		linelen = GetStringLen(line4, USER);
		sprintf(lineall[4], "%s", line4);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line4);
		}
	}
	if (maxlines > 4)
	{
		sprintf(line5, "%s", argv[pa + 7]);
		linelen = GetStringLen(line5, USER);
		sprintf(lineall[5], "%s", line5);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line5);
		}
	}
	if (maxlines > 5)
	{
		sprintf(line6, "%s", argv[pa + 8]);
		linelen = GetStringLen(line6, USER);
		sprintf(lineall[6], "%s", line6);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line6);
		}
	}
	if (maxlines > 6)
	{
		sprintf(line7, "%s", argv[pa + 9]);
		linelen = GetStringLen(line7, USER);
		sprintf(lineall[7], "%s", line7);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line7);
		}
	}
	if (maxlines > 7)
	{
		sprintf(line8, "%s", argv[pa + 10]);
		linelen = GetStringLen(line8, USER);
		sprintf(lineall[8], "%s", line8);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line8);
		}
	}
	if (maxlines > 8)
	{
		sprintf(line9, "%s", argv[pa + 11]);
		linelen = GetStringLen(line9, USER);
		sprintf(lineall[9], "%s", line9);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line9);
		}
	}
	if (maxlines > 9)
	{
		sprintf(line10, "%s", argv[pa + 12]);
		linelen = GetStringLen(line10, USER);
		sprintf(lineall[10], "%s", line10);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line10);
		}
	}
	if (maxlines > 10)
	{
		sprintf(line11, "%s", argv[pa + 13]);
		linelen = GetStringLen(line11, USER);
		sprintf(lineall[11], "%s", line11);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line11);
		}
	}
	if (maxlines > 11)
	{
		sprintf(line12, "%s", argv[pa + 14]);
		linelen = GetStringLen(line12, USER);
		sprintf(lineall[12], "%s", line12);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line12);
		}
	}
	if (maxlines > 12)
	{
		sprintf(line13, "%s", argv[pa + 15]);
		linelen = GetStringLen(line13, USER);
		sprintf(lineall[13], "%s", line13);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line13);
		}
	}
	if (maxlines > 13)
	{
		sprintf(line14, "%s", argv[pa + 16]);
		linelen = GetStringLen(line14, USER);
		sprintf(lineall[14], "%s", line14);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line14);
		}
	}
	if (maxlines > 14)
	{
		sprintf(line15, "%s", argv[pa + 17]);
		linelen = GetStringLen(line15, USER);
		sprintf(lineall[15], "%s", line15);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line15);
		}
	}
	if (maxlines > 15)
	{
		sprintf(line16, "%s", argv[pa + 18]);
		linelen = GetStringLen(line16, USER);
		sprintf(lineall[16], "%s", line16);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line16);
		}
	}
	if (maxlines > 16)
	{
		sprintf(line17, "%s", argv[pa + 19]);
		linelen = GetStringLen(line17, USER);
		sprintf(lineall[17], "%s", line17);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line17);
		}
	}
	if (maxlines > 17)
	{
		sprintf(line18, "%s", argv[pa + 20]);
		linelen = GetStringLen(line18, USER);
		sprintf(lineall[18], "%s", line18);
		if (linelen > maxstringlen)
		{
			maxstringlen = linelen;
			sprintf(bigestline, "%s", line18);
		}
	}
	if (strcmp(infoline, "progressbar") == 0)
	{
		progressbar = 1;
		progressbarlen = atoi(line1);
		progressbarypos = atoi(line2);
		drawbox = 0;
	}
	SetBoxPos();

	//deaktivate E2 framebuffer output
	incshmentry(shm, "stopGUI=");
	incshmentry(shm, "stopSpinner=");
	incshmentry(shm, "stopRC=");
	if (nostopSzapRC == 0 && progressbar == 0)
	{
		incshmentry(shm, "stopSzapRC=");
	}

	//screensaver
	if (strcmp(infoline, "saver") == 0)
	{
		decshmentry(shm, "stopRC=");
		if (nostopSzapRC == 0 && progressbar == 0)
		{
			decshmentry(shm, "stopSzapRC=");
		}
		int overscanx = 60, overscany = 60;
		int oldposx = overscanx, oldposy = overscany;
		while (1)
		{
			if (videomodechange() == 1)
			{
				oldposx = overscanx, oldposy = overscany;
			}
			int newposx = rand() % (var_screeninfo.xres - (overscanx * 2));
			int newposy = rand() % (var_screeninfo.yres - (overscany * 2));
			newposx -= (newposx % 4);
			newposy -= (newposy % 4);
			memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);

			while (newposx != oldposx || newposy != oldposy)
			{
				if (ende == 1)
				{
					memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);
#ifdef USE_BLIT
					blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
					//aktivate E2 framebuffer output
					decshmentry(shm, "stopGUI=");
					decshmentry(shm, "stopSpinner=");
					exit(0);
				}
				if (newposx > oldposx)
				{
					oldposx += 4;
				}
				if (newposx < oldposx)
				{
					oldposx -= 4;
				}
				if (newposy > oldposy)
				{
					oldposy += 4;
				}
				if (newposy < oldposy)
				{
					oldposy -= 4;
				}
				RenderString(line1, oldposx + overscanx - maxstringlen,
					oldposy + overscany + FONTHEIGHT_USER, maxstringlen,
					CENTER, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");

				if (maxlines > 1)
				{
					RenderString(line2, oldposx + overscanx - maxstringlen,
						oldposy + overscany + (FONTHEIGHT_USER * 2), maxstringlen,
						CENTER, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
				}
#ifdef USE_BLIT
				blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
				usleep(waittime * 100);

				RenderString(line1, oldposx + overscanx - maxstringlen,
					oldposy + overscany + FONTHEIGHT_USER, maxstringlen,
					CENTER, USER, (unsigned char *)"\x00\x00\x00\x00");

				if (maxlines > 1)
				{
					RenderString(line2, oldposx + overscanx - maxstringlen,
						oldposy + overscany + (FONTHEIGHT_USER * 2), maxstringlen,
						CENTER, USER, (unsigned char *)"\x00\x00\x00\x00");
				}
#ifdef USE_BLIT
				blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
			}
		}
	}

	if (strcmp(infoline, "saver1") == 0)
	{
		decshmentry(shm, "stopRC=");
		if (nostopSzapRC == 0 && progressbar == 0)
		{
			decshmentry(shm, "stopSzapRC=");
		}
		int overscanx = 60, overscany = 60;
		int oldposx = overscanx, oldposy = overscany;
		if (openpng())
		{
			//aktivate E2 framebuffer output
			decshmentry(shm, "stopGUI=");
			decshmentry(shm, "stopSpinner=");
			exit(1);
		}

		while (1)
		{
			if (videomodechange() == 1)
			{
				oldposx = overscanx, oldposy = overscany;
			}
			int newposx = rand() % (var_screeninfo.xres - image_width - (overscanx * 2));
			int newposy = rand() % (var_screeninfo.yres - image_height - (overscany * 2));
			newposx -= (newposx % 4);
			newposy -= (newposy % 4);
			memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);

			while (newposx != oldposx || newposy != oldposy)
			{
				if (ende == 1)
				{
					memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);
#ifdef USE_BLIT
					blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
					free(image_data);
					//aktivate E2 framebuffer output
					decshmentry(shm, "stopGUI=");
					decshmentry(shm, "stopSpinner=");
					exit(0);
				}
				if (newposx > oldposx)
				{
					oldposx += 4;
				}
				if (newposx < oldposx)
				{
					oldposx -= 4;
				}
				if (newposy > oldposy)
				{
					oldposy += 4;
				}
				if (newposy < oldposy)
				{
					oldposy -= 4;
				}
				showpng(oldposx + overscanx, oldposy + overscany, 0);
#ifdef USE_BLIT
				blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
				usleep(waittime * 100);
				showpng(oldposx + overscanx, oldposy + overscany, 1);
#ifdef USE_BLIT
				blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
			}
		}
	}

	if (strcmp(infoline, "saver2") == 0)
	{
		decshmentry(shm, "stopRC=");
		if (nostopSzapRC == 0 && progressbar == 0)
		{
			decshmentry(shm, "stopSzapRC=");
		}
		int overscanx = 60, overscany = 60;
		if (openpng())
		{
			//aktivate E2 framebuffer output
			decshmentry(shm, "stopGUI=");
			decshmentry(shm, "stopSpinner=");
			exit(1);
		}

		while (1)
		{
			videomodechange();
			int newposx = rand() % (var_screeninfo.xres - image_width - (overscanx * 2));
			int newposy = rand() % (var_screeninfo.yres - image_height - (overscany * 2));
			memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);

			if (ende == 1)
			{
				memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);
#ifdef USE_BLIT
				blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
				//aktivate E2 framebuffer output
				decshmentry(shm, "stopGUI=");
				decshmentry(shm, "stopSpinner=");
				free(image_data);
				exit(0);
			}
			showpng(newposx + overscanx, newposy + overscany, 0);
#ifdef USE_BLIT
			blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
			usleep(waittime * 10000);
			showpng(newposx + overscanx, newposy + overscany, 1);
#ifdef USE_BLIT
			blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
		}
	}
	SetBalkenPos();
	DrawBox();

	sprintf(balkenout, "%0.2f%s", 0.00, "%");

	if (strcmp(kommando, "GUI") != 0)
	{
		while (1)
		{
			if (videomodechange() == 1)
			{
				SetBalkenPos();
				DrawBox();
				if (balken == 1)
				{
					balkenx1 = (balkenlen / (float)waittime) * (float)balken1count;
				}
				if (balken == 2)
				{
					 balkenx1 = (balkenlen / 100.00) * (float)balken2count;
				}
				if (balken == 1 || balken == 2)
				{
					RenderBox(boxx + linex, boxy1 + liney + offsety, boxx + linex + (int)balkenx1, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, (unsigned char *)"\xFF\x80\x00\xFF");
					RenderString(balkenout, boxx + linex + offsetx, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, maxstringlen, CENTER, USER_SMALL, (unsigned char *)"\xFF\xFF\xFF\xFF");
				}
			}
	
			if (balken == 1 || (balken == 2 && balken1count == 0))
			{
				RenderString(balkenout, boxx + linex + offsetx, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, maxstringlen, CENTER, USER_SMALL, (unsigned char *)"\xFF\xFF\xFF\xFF");
			}
			if (balken1count >= waittime)
			{
				if (balken == 1)
				{
					sprintf(balkenout, "%0.2f%s", 100.00, "%");
					RenderBox(boxx + linex, boxy1 + liney + offsety, boxx + linex+balkenlen, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, (unsigned char *)"\xFF\x80\x00\xFF");
					RenderString(balkenout, boxx + linex + offsetx, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, maxstringlen, CENTER, USER_SMALL, (unsigned char *)"\xFF\xFF\xFF\xFF");
					sleep(1);
				}
				break;
			}
			if (progressbar == 1)
			{
				readprogress();
				balken = 0;

				if (balken1count < newprogressbar)
				{
					balken1count = newprogressbar;
					balken = 1;
				}
 			}

			if (ende == 1)
			{
				break;
			}
			sleep(1);

			if (progressbar == 0)
			{
				balken1count++;
			}
			if (balken == 1)
			{
				RenderString(balkenout, boxx + linex+offsetx, boxy1 + liney +offsety + FONTHEIGHT_USER_SMALL, maxstringlen, CENTER, USER_SMALL, (unsigned char *)"\x26\x20\x1F\xFF");
				balkenx1 = (balkenlen / (float)waittime) * (float)balken1count;
				RenderBox(boxx + linex, boxy1 + liney + offsety, boxx + linex + (int)balkenx1, boxy1 + liney + offsety + FONTHEIGHT_USER_SMALL, (unsigned char *)"\xFF\x80\x00\xFF");
				sprintf(balkenout, "%0.2f%s", (float)(100.00 / waittime) * balken1count, "%");
			}
#ifdef USE_BLIT
			blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
		}
	}
	else
	{
		int actline = 1;

		if (actlineset > 0 && actlineset <= maxlines)
		{
			actline = actlineset;
		}
		while (strlen(lineall[actline]) == 0)
		{
			actline = actline + 1;
			if (actline > maxlines)
			{
				actline = 1;
			}
		}
		int endew = 0;
		rueckzeile = 0;
		//file_ttyAS1 = open("/dev/input/event0", O_RDWR);
#if 0 //def MIPSEL
#ifdef EVENT0
		file_ttyAS1 = open("/dev/input/event0", O_RDONLY);
#else
		file_ttyAS1 = open("/dev/input/event1", O_RDONLY);
#endif
#else
		file_ttyAS1 = open("/dev/input/event0", O_RDONLY);
#endif	
		if (file_ttyAS1 == -1)
		{
			printf("Error - cannot open /dev/rc\n");
		}
		else
		{
			RenderString(lineall[actline], boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * (actline + 1) + offsety * actline + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\x00\xFF\xFF\xFF");
//#ifdef MIPSEL
//			sprintf(cmd, "echo %s > /dev/oled0", lineall[actline]);
//#else
			sprintf(cmd, "echo %s > /dev/vfd", lineall[actline]);
//#endif
			system(cmd);
			while (RCtrlCode != KEY_ESC)
			{
#ifdef USE_BLIT
				blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
				RCtrlCode = GetRCtrlCode();
				switch (RCtrlCode)
				{
					case DOWN:
					{
						RenderString(lineall[actline], boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * (actline + 1) + offsety * actline + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
						actline = actline + 1;
						while (strlen(lineall[actline]) == 0)
						{
							actline = actline + 1;
							if (actline > maxlines)
							{
								actline = 1;
							}
						}
						if (actline > maxlines)
						{
							actline = 1;
						}
						RenderString(lineall[actline], boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * (actline + 1) + offsety * actline + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\x00\xFF\xFF\xFF");
//#ifdef MIPSEL
//						sprintf(cmd,"echo %s > /dev/oled0",lineall[actline]);
//#else
						sprintf(cmd,"echo %s > /dev/vfd",lineall[actline]);
//#endif
						system(cmd);
						break;
					}
					case UP:
					{
						RenderString(lineall[actline], boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * (actline + 1) + offsety * actline + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\xFF\xFF\xFF\xFF");
						actline = actline - 1;
						while (strlen(lineall[actline]) == 0)
						{
							actline = actline - 1;
							if (actline < 1)
							{
								actline = maxlines;
							}
						}
						if (actline < 1)
						{
							actline = maxlines;
						}
						RenderString(lineall[actline], boxx + linex + offsetx, boxy + liney * 2 + FONTHEIGHT_USER * (actline + 1) + offsety * actline + offsety_2, maxstringlen, LEFT, USER, (unsigned char *)"\x00\xFF\xFF\xFF");
//#ifdef MIPSEL
//						sprintf(cmd, "echo %s > /dev/oled0", lineall[actline]);
//#else
						sprintf(cmd, "echo %s > /dev/vfd", lineall[actline]);
//#endif
						system(cmd);
						break;
					}
					case OK:
					case RED:
					{
						if (RCtrlCode == OK)
						{
							rueckzeile = actline;
						}
						if (RCtrlCode == RED)
						{
							rueckzeile = actline + 20;
						}
						close (file_ttyAS1);
						endew = 1;
						break;
					}
				}
				if (endew == 1)
				{
					break;
				}
			}
		}
	}
	printf("[infobox] end\n");
	// Programmende

	// Libfreetype aufräumen
	FTC_Manager_Done(manager);
	FT_Done_FreeType(library);

	// Framebuffer wieder leeren (Transparent machen), Speicher freigeben,
	// Framebufefr Device schliessen
	if (checkshmentry(shm, "stopFBClear=") != 1)
	{
		readprogress();
		if (useJpegBGx != 0 && useJpegBGy != 0 && newprogressbar < 95)
		{
			blit(useJpegBGx, useJpegBGy);
		}
		else
		{
			memset(lfb, 0, var_screeninfo.yres * fix_screeninfo.line_length);
		}
#ifdef USE_BLIT
		blit(var_screeninfo.xres, var_screeninfo.yres);
#endif
	}
	munmap(lfb, fix_screeninfo.smem_len);
	close(fb);

	//aktivate E2 framebuffer output
	decshmentry(shm, "stopGUI=");
	decshmentry(shm, "stopSpinner=");
	decshmentry(shm, "stopRC=");
	if (nostopSzapRC == 0 && progressbar == 0)
	{
		decshmentry(shm, "stopSzapRC=");
	}
	if (strcmp(kommando, "GUI") == 0)
	{
		exit(rueckzeile);
	}
	exit(0);
}
// vim:ts=4
