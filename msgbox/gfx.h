#ifndef __GFX_H__
#define __GFX_H__

void RenderBox(int sx, int sy, int ex, int ey, int mode, int color);
int paintIcon(const char *const fname, int xstart, int ystart, int xsize, int ysize, int *iw, int *ih);
void scale_pic(unsigned char **buffer, int x1, int y1, int xstart, int ystart, int xsize, int ysize,
			   int *imx, int *imy, int *dxp, int *dyp, int *dxo, int *dyo, int alpha);

#if defined(HAVE_SPARK_HARDWARE) || defined(HAVE_DUCKBOX_HARDWARE)
void FillRect(int sx, int sy, int ex, int ey, uint32_t color);
#endif

#endif
